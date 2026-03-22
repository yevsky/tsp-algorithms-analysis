#ifndef TSP_PROJECT_TSP_ALGORITHMS_H
#define TSP_PROJECT_TSP_ALGORITHMS_H

#include "graph.h"

// Random algorithm: shuffles permutations for time_limit_sec seconds (or iterations, whichever comes first)
int randTSP(const Matrix& g, int iterations, int time_limit_sec = 1800);

// Nearest Neighbour from a single start vertex
int nearestNeighbor(const Matrix& g, int start);

// Repeated Nearest Neighbour: tries every start vertex, explores tie-breaking branches via DFS
int RNN(const Matrix& g, int time_limit_sec = 900);

// Brute-force: generates all (n-1)! permutations, stops after time_limit_sec seconds
int bruteForce(const Matrix& g, int time_limit_sec = 1800);

// Relative error in percent: 100 * (approx - optimum) / optimum
double relativeError(int approx, int optimum);

#endif // TSP_PROJECT_TSP_ALGORITHMS_H