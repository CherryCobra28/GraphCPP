#include <igraph.h>
#include <iostream>
#include <random>
#include <unordered_set>
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
  igraph_vector_int_init(&neigh, 0);
  for (auto node : G->infected) {
    igraph_neighbors(G->graph, &neigh, node, IGRAPH_ALL);
    igraph_integer_t n_size = igraph_vector_int_size(&neigh);
    for (igraph_integer_t i = 0; i < n_size; i++) {
      double r = r_number();
      if (r < p_i) {
        G->infected.insert(VECTOR(neigh)[i]);
      }
    }
  }
  igraph_vector_int_destroy(&neigh);
}

int main() {
  igraph_t seed;
  igraph_integer_t n{5};
  igraph_full(&seed, n, NULL, NULL);
  igraph_t g;
  igraph_barabasi_game(&g, 10, 1.0, 3, NULL, true, 0, false,
                       IGRAPH_BARABASI_PSUMTREE, &seed);
  std::unordered_set<igraph_integer_t> infected{9};
  InfectGraph G{&g, infected};
  infect(&G, 0.5);
  for (auto x : G.infected) {
    printf("%" IGRAPH_PRId " ", x);
  }
  igraph_destroy(&seed);
  return 0;
}
