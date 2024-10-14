#include "igraph_interface.h"
#include "igraph_iterators.h"
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
class vit_t {
public:
  igraph_vs_t vit;
  ~vit_t() { igraph_vs_destroy(&vit); };
};
void infect(InfectGraph *G, double p_i) {
  igraph_vit_t neighbours;
  igraph_vs_t vs;
  for (auto node : G->infected) {
    igraph_vs_adj(&vs, node, IGRAPH_ALL);
    igraph_vit_create(G->graph, vs, &neighbours);
    while (!IGRAPH_VIT_END(neighbours)) {
      igraph_integer_t n{IGRAPH_VIT_GET(neighbours)};
      // printf("%" IGRAPH_PRId " ", n);
      double r = r_number();
      // printf("%g\n", r);
      if (r < p_i) {
        G->infected.insert(n);
      }
      IGRAPH_VIT_NEXT(neighbours);
    }
  }
  igraph_vit_destroy(&neighbours);
  igraph_vs_destroy(&vs);
}

int main() {
  igraph_t g;
  igraph_integer_t n{5};
  igraph_full(&g, n, NULL, NULL);
  std::unordered_set<igraph_integer_t> infected{0};
  InfectGraph G{&g, infected};
  infect(&G, 0.5);
  for (auto x : G.infected) {
    printf("%" IGRAPH_PRId " ", x);
  }
  return 0;
}
