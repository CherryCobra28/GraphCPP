/**
 * NOTE TO SELF:
 * FIND OUT WHY IGRAPH_NEW<IGRAPH_T> CAUSES A MEMORY LEAK IF IT IS NOT
 * STD::MOVED\
 * I.E.: NO LEAK:\
 * igraph_t s;\
 * auto s_p = igraph_make_uni_ptr(&s);\
 * NO LEAK (but compiler warning):
 * auto s_p =std::move(igraph_new<igraph_t>());\
 * LEAK:\
 * auto s_p = igraph_new<igraph_t>();\
 *
 *
 */
#include "igraph_interface.h"
#include "igraph_iterators.h"
#include "igraph_types.h"
#include <igraph.h>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>

// Please dont use this
#define IGRAPH_NEW(X)                                                          \
                                                                               \
  auto __x = igraph_new<igraph_t>();                                           \
  auto X = std::move(__x);

template <typename T> struct always_false {
  enum { value = false };
};

template <typename T> void destroy(T *t) {
  static_assert(always_false<T>::value, "DESTROY NOT IMPLEMENETED FOR TYPE!");
};
void destroy(igraph_vector_int_t *v) { igraph_vector_int_destroy(v); }
void destroy(igraph_t *g) { igraph_destroy(g); }
template <typename T> struct Destroyer {
  void operator()(T *t) { destroy(t); }
};
template <typename T>
std::unique_ptr<T, Destroyer<T>> igraph_make_uni_ptr(T *t) {
  return std::unique_ptr<T, Destroyer<T>>(t);
}

template <typename T> std::unique_ptr<T, Destroyer<T>> igraph_new() {
  T v;
  auto v_ptr = igraph_make_uni_ptr(&v);
  return v_ptr;
}

// std::unique_ptr<igraph_t, Destroyer<igraph_t>> igraph_new() {
//   igraph_t v;
//   auto v_ptr = igraph_make_uni_ptr(&v);
//   auto r_ptr = std::move(v_ptr);
//   return r_ptr;
// }

double r_number() {
  std::random_device red;
  std::default_random_engine gen(red());
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  double r = distribution(gen);
  return r;
}

void set_iota(std::unordered_set<igraph_integer_t> *s, igraph_integer_t n) {
  for (igraph_integer_t i = 0; i < n; i++) {
    s->insert(s->end(), i);
  }
}

class InfectGraph {
public:
  igraph_t *graph;
  std::unordered_set<igraph_integer_t> nodes;
  std::unordered_set<igraph_integer_t> infected;
  std::unordered_set<igraph_integer_t> recovered;
  std::unordered_map<igraph_integer_t, int> days_inf;
  std::map<igraph_integer_t, igraph_integer_t> nodes_to_id;
  std::map<igraph_integer_t, igraph_integer_t> id_to_nodes;
  InfectGraph(igraph_t *g)
      : graph{g}, nodes{}, infected{}, recovered{}, days_inf{}, nodes_to_id{},
        id_to_nodes{} {
    size_t buffer = igraph_vcount(g);
    infected.reserve(buffer);
    recovered.reserve(buffer);
    nodes.reserve(buffer);
    infected.insert(0);

    // std::iota(nodes.begin(), nodes.end(), 0);
    set_iota(&nodes, buffer);
    for (auto node : nodes) {
      days_inf[node] = 0;
      nodes_to_id[node] = node;
      id_to_nodes[node] = node;
    }
  }
  ~InfectGraph() { igraph_destroy(graph); }
  void remove_infected(igraph_integer_t x) {
    auto pos = infected.find(x);
    infected.erase(pos);
  }

  void remove_node(igraph_integer_t x) {
    auto pos = nodes.find(x);
    nodes.erase(pos);
  }
  void update_node_id(const igraph_vector_int_t *v) {
    igraph_integer_t i = 0;
    for (auto [node, id] : nodes_to_id) {
      if (i < igraph_vector_int_size(v)) {
        if (id != i) {
          nodes_to_id.at(node) = i;
        }
      } else {
        break;
      }
      i++;
    }
  }
  void update_id_node() {
    for (auto [node, id] : nodes_to_id) {
      id_to_nodes.at(id) = node;
    }
  }
};

void print_set(const std::unordered_set<igraph_integer_t> &set) {
  for (auto x : set) {
    std::cout << x << " ";
  }
  std::cout << "\n";
}

void infect(InfectGraph *G, double p_i) {
  auto neigh_p = igraph_new<igraph_vector_int_t>();
  igraph_vector_int_init(neigh_p.get(), 0);
  for (auto inode : G->infected) {
    std::cout << "Node: " << inode << "\n";
    std::cout << "Infected: \n";
    print_set(G->infected);
    std::cout << "Nodes: \n";
    print_set(G->nodes);
    std::cout << inode << "\n";
    igraph_neighbors(G->graph, neigh_p.get(), G->nodes_to_id.at(inode),
                     IGRAPH_ALL);
    igraph_integer_t n_size = igraph_vector_int_size(neigh_p.get());
    for (igraph_integer_t i = 0; i < n_size; i++) {
      igraph_integer_t id = VECTOR(*neigh_p)[i];
      igraph_integer_t node = G->id_to_nodes.at(id);
      if (G->recovered.find(node) == G->recovered.end()) {
        double r = r_number();
        if (r < p_i) {
          G->infected.insert(node);
        }
      }
    }
  }
  // igraph_vector_int_destroy(&neigh);
}

void die_recover(InfectGraph *G, double p_r, int time_until_recover) {
  for (auto [node, days] : G->days_inf) {

    igraph_vector_int_t invidx;
    igraph_vector_int_init(&invidx, 0);
    if (days > time_until_recover) {
      double r = r_number();
      if (r < p_r) {
        G->recovered.insert(node);
      } else {

        igraph_delete_vertices_idx(
            G->graph, igraph_vss_1(G->nodes_to_id.at(node)), NULL, &invidx);
        G->nodes_to_id.erase(node);
        G->update_node_id(&invidx);
        G->update_id_node();
        G->remove_node(node);
      }
      G->days_inf.at(node) = 0;
      G->remove_infected(node);
    } else {
      if (G->infected.find(node) != G->infected.end())
        G->days_inf.at(node) += 1;
    }

    igraph_vector_int_destroy(&invidx);
  }
}

int test() {
  // IGRAPH_NEW(seed_p);
  igraph_t seed;
  auto seed_p = igraph_make_uni_ptr(&seed);
  igraph_integer_t n{5};
  igraph_full(seed_p.get(), n, false, false);
  igraph_t g;
  igraph_barabasi_game(&g, 1000000, 1.0, 3, NULL, true, 0, false,
                       IGRAPH_BARABASI_PSUMTREE, seed_p.get());
  // igraph_full(&g, 10, false, false);
  InfectGraph G{&g};
  G.recovered.insert(3);
  infect(&G, 1);
  // for (auto x : G.infected) {
  //   std::cout << x << " ";
  // }
  G.infected.insert(6);
  G.days_inf.at(6) = 10;
  // printf("\n");
  // printf("----------------- \n");
  // for (auto [node, day] : G.days_inf) {
  //  std::cout << node << " => " << day << "\n";
  //}
  die_recover(&G, 0.0, 5);

  // printf("----------\n");
  // for (auto x : G.nodes) {
  // std::cout << x << " ";
  //}
  // printf("\n");
  // printf("---------------\n");

  return 0;
}

void vec_print(const igraph_vector_int_t *v) {
  igraph_integer_t v_size = igraph_vector_int_size(v);
  for (igraph_integer_t i = 0; i < v_size; ++i) {
    std::cout << VECTOR(*v)[i] << " ";
  }
  std::cout << "\n";
}

int main_3() {
  igraph_t g;
  igraph_full(&g, 10, false, false);
  igraph_vs_t vs;
  igraph_vs_all(&vs);
  igraph_vit_t vit;
  igraph_vit_create(&g, vs, &vit);
  while (!IGRAPH_VIT_END(vit)) {
    std::cout << IGRAPH_VIT_GET(vit) << " ";
    IGRAPH_VIT_NEXT(vit);
  }

  printf("\n");
  igraph_delete_vertices(&g, igraph_vss_1(3));
  igraph_vit_t vit2;
  igraph_vit_create(&g, vs, &vit2);
  while (!IGRAPH_VIT_END(vit2)) {
    std::cout << IGRAPH_VIT_GET(vit2) << " ";
    IGRAPH_VIT_NEXT(vit2);
  }
  return 0;
}

int main_6() {
  igraph_t g;
  igraph_full(&g, 10, false, false);
  // igraph_vector_int_t idx;
  // igraph_vector_int_t invidx;
  // igraph_vector_int_init(&idx, 0);
  // igraph_vector_int_init(&invidx, 0);
  // igraph_delete_vertices_idx(&g, igraph_vss_1(3), &idx, &invidx);
  // std::cout << "Idx: \n";
  // vec_print(&idx);
  // std::cout << "Invidx: \n";
  // vec_print(&invidx);
  InfectGraph G{&g};
  for (auto [node, id] : G.nodes_to_id) {
    std::cout << node << " => " << id << "\n";
  }
  printf("\n");
  G.infected.insert(3);
  G.days_inf.at(3) = 10;
  die_recover(&G, 0.0, 3);
  for (auto [node, id] : G.nodes_to_id) {
    std::cout << node << " => " << id << "\n";
  }

  printf("\n");
  G.infected.insert(6);
  G.days_inf.at(6) = 10;
  die_recover(&G, 0.0, 3);
  for (auto [node, id] : G.nodes_to_id) {
    std::cout << node << " => " << id << "\n";
  }

  printf("\n");
  G.infected.insert(8);
  G.days_inf.at(8) = 10;
  die_recover(&G, 0.0, 3);
  for (auto [node, id] : G.nodes_to_id) {
    std::cout << node << " => " << id << "\n";
  }
}

int main() {
  igraph_t seed;
  auto seed_p = igraph_make_uni_ptr(&seed);
  igraph_integer_t n{5};
  igraph_full(seed_p.get(), n, false, false);
  igraph_t g;
  igraph_barabasi_game(&g, 10000, 1.0, 3, NULL, true, 0, false,
                       IGRAPH_BARABASI_PSUMTREE, seed_p.get());
  InfectGraph G{&g};
  const double p_i = 0.6;
  const double p_r = 0.2;
  const int time_recover = 10;
  int counter{0};
  size_t size{};
  do {
    infect(&G, p_i);
    die_recover(&G, p_r, time_recover);
    counter++;
    size = G.infected.size();
  } while (size != 0);
  if (G.nodes.size() == 0) {
    std::cout << "OOPS ALL DEAD" << "\n"
              << "Only Took: " << counter << " Days";
  } else {
    std::cout << G.nodes.size() << " Survived" << "\n"
              << "Only Took: " << counter << " Days";
  }
  return 0;
}
