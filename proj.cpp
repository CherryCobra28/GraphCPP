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
#include <igraph.h>
#include <iostream>
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

void set_iota(std::set<igraph_integer_t> *s, igraph_integer_t n) {
  for (igraph_integer_t i = 0; i < n; i++) {
    s->insert(s->end(), i);
  }
}

class InfectGraph {
public:
  igraph_t *graph;
  std::set<igraph_integer_t> nodes;
  std::unordered_set<igraph_integer_t> infected;
  std::unordered_set<igraph_integer_t> recovered;
  std::unordered_map<igraph_integer_t, int> days_inf;
  InfectGraph(igraph_t *g)
      : graph{g}, nodes{}, infected{}, recovered{}, days_inf{} {
    size_t buffer = igraph_vcount(g);
    infected.reserve(buffer);
    recovered.reserve(buffer);
    infected.insert(0);

    // std::iota(nodes.begin(), nodes.end(), 0);
    set_iota(&nodes, buffer);
    for (auto node : nodes) {
      days_inf[node] = 0;
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
};

void infect(InfectGraph *G, double p_i) {
  auto neigh_p = igraph_new<igraph_vector_int_t>();
  igraph_vector_int_init(neigh_p.get(), 0);
  for (auto node : G->infected) {
    igraph_neighbors(G->graph, neigh_p.get(), node, IGRAPH_ALL);
    igraph_integer_t n_size = igraph_vector_int_size(neigh_p.get());
    for (igraph_integer_t i = 0; i < n_size; i++) {
      igraph_integer_t node = VECTOR(*neigh_p)[i];
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
    if (days > time_until_recover) {
      double r = r_number();
      if (r < p_r) {
        G->recovered.insert(node);
      } else {
        igraph_delete_vertices(G->graph, igraph_vss_1(node));
        G->remove_node(node);
      }
      G->days_inf.at(node) = 0;
      G->remove_infected(node);
    }
  }
}

int main() {
  // IGRAPH_NEW(seed_p);
  igraph_t seed;
  auto seed_p = igraph_make_uni_ptr(&seed);
  igraph_integer_t n{5};
  igraph_full(seed_p.get(), n, false, false);
  igraph_t g;
  igraph_barabasi_game(&g, 10, 1.0, 3, NULL, true, 0, false,
                       IGRAPH_BARABASI_PSUMTREE, seed_p.get());
  // igraph_full(&g, 10, false, false);
  InfectGraph G{&g};
  G.recovered.insert(3);
  infect(&G, 1);
  for (auto x : G.infected) {
    std::cout << x << " ";
  }
  G.infected.insert(6);
  G.days_inf.at(6) = 10;
  printf("\n");
  printf("----------------- \n");
  for (auto [node, day] : G.days_inf) {
    std::cout << node << " => " << day << "\n";
  }
  die_recover(&G, 0.0, 5);
  printf("----------\n");
  for (auto x : G.nodes) {
    std::cout << x << " ";
  }
  printf("\n");
  printf("---------------\n");

  return 0;
}
