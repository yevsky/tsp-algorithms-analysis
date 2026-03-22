#ifndef TSP_PROJECT_GRAPH_H
#define TSP_PROJECT_GRAPH_H

#include <vector>
#include <string>

using Matrix = std::vector<std::vector<int>>;

// Generate a random complete graph (symmetric or asymmetric), weights in [1,100]
Matrix generateGraph(int n, bool asymmetric);

// Calculate total cycle cost for a given path (returns to path[0] at the end)
int pathCost(const Matrix& g, const std::vector<int>& path);

#endif // TSP_PROJECT_GRAPH_H