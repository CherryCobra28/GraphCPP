#include <igraph.h>
#include <iostream>
#include <memory>
#include <random>
#include <unordered_set>

template <typename T> void destroy(T *t);
void destroy(igraph_vector_int_t *v) { igraph_vector_int_destroy(v); }
void destroy(igraph_t *g) { igraph_destroy(g); }
template <typename T> struct Destroyer {
  void operator()(T *t) { destroy(t); }
};
template <typename T>
std::unique_ptr<T, Destroyer<T>> igraph_make_uni_ptr(T *t) {
  return std::unique_ptr<T, Destroyer<T>>(t);
}

double r_number() {
  std::random_device red;
  std::default_random_engine gen(red());
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  double r = distribution(gen);
  return r;
}

class InfectGraph {
public:
  igraph_t *graph;
  std::unordered_set<igraph_integer_t> infected;
  ~InfectGraph() { igraph_destroy(graph); }
};

void infect(InfectGraph *G, double p_i) {
  igraph_vector_int_t neigh;
  auto neigh_p = igraph_make_uni_ptr(&neigh);
  igraph_vector_int_init(neigh_p.get(), 0);
  for (auto node : G->infected) {
    igraph_neighbors(G->graph, neigh_p.get(), node, IGRAPH_ALL);
    igraph_integer_t n_size = igraph_vector_int_size(neigh_p.get());
    for (igraph_integer_t i = 0; i < n_size; i++) {
      double r = r_number();
      if (r < p_i) {
        G->infected.insert(VECTOR(*neigh_p)[i]);
      }
    }
  }
  // igraph_vector_int_destroy(&neigh);
}

int main() {
  igraph_t seed;
  auto seed_p = igraph_make_uni_ptr(&seed);
  igraph_integer_t n{5};
  igraph_full(seed_p.get(), n, NULL, NULL);
  igraph_t g;
  igraph_barabasi_game(&g, 10, 1.0, 3, NULL, true, 0, false,
                       IGRAPH_BARABASI_PSUMTREE, &seed);
  std::unordered_set<igraph_integer_t> infected{9};
  InfectGraph G{&g, infected};
  infect(&G, 0.5);
  for (auto x : G.infected) {
    printf("%" IGRAPH_PRId " ", x);
  }
  return 0;
}
