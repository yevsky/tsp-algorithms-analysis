#include "tsp_algorithms.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <climits>
#include <numeric>

static const int INF = INT_MAX / 2;

// ---------------------------------------------------------------------------
// RAND
// ---------------------------------------------------------------------------
int randTSP(const Matrix& g, int iterations, int time_limit_sec)
{
    int n = (int)g.size();
    std::vector<int> path(n);
    std::iota(path.begin(), path.end(), 0);

    std::random_device rd;
    std::mt19937 gen(rd());

    int best = INF;
    auto startTime = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < iterations; i++)
    {
        // Keep city 0 fixed, shuffle the rest
        std::shuffle(path.begin() + 1, path.end(), gen);
        int cost = pathCost(g, path);
        if(cost < best) best = cost;

        // Check time limit
        auto now = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() >= time_limit_sec)
            break;
    }
    return best;
}

// ---------------------------------------------------------------------------
// NN (single start)
// ---------------------------------------------------------------------------
int nearestNeighbor(const Matrix& g, int start)
{
    int n = (int)g.size();
    std::vector<bool> visited(n, false);
    int current = start;
    visited[current] = true;
    int cost = 0;

    for(int step = 1; step < n; step++)
    {
        int bestDist = INF, next = -1;
        for(int j = 0; j < n; j++)
        {
            if(!visited[j] && g[current][j] > 0 && g[current][j] < bestDist)
            {
                bestDist = g[current][j];
                next = j;
            }
        }
        if(next == -1) break; // disconnected graph (shouldn't happen for full graphs)
        cost += bestDist;
        visited[next] = true;
        current = next;
    }
    cost += g[current][start];
    return cost;
}

// ---------------------------------------------------------------------------
// RNN – DFS with tie-breaking
// ---------------------------------------------------------------------------
static void rnnDFS(
    const Matrix& g,
    std::vector<bool>& visited,
    int current,
    int depth,
    int cost,
    int& best,
    int startNode,
    const std::chrono::high_resolution_clock::time_point& startTime,
    int time_limit_sec)
{
    // Time guard
    auto now = std::chrono::high_resolution_clock::now();
    if(std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() >= time_limit_sec)
        return;

    int n = (int)g.size();

    if(depth == n)
    {
        int total = cost + g[current][startNode];
        if(total < best) best = total;
        return;
    }

    // Find minimum outgoing distance to an unvisited node
    int minDist = INF;
    for(int j = 0; j < n; j++)
        if(!visited[j] && g[current][j] > 0 && g[current][j] < minDist)
            minDist = g[current][j];

    if(minDist == INF) return; // no unvisited neighbour reachable

    // Explore ALL edges with that minimum distance (tie-breaking)
    for(int j = 0; j < n; j++)
    {
        if(!visited[j] && g[current][j] == minDist)
        {
            visited[j] = true;
            rnnDFS(g, visited, j, depth + 1, cost + minDist, best, startNode, startTime, time_limit_sec);
            visited[j] = false;
        }
    }
}

int RNN(const Matrix& g, int time_limit_sec)
{
    int n = (int)g.size();
    int best = INF;
    auto startTime = std::chrono::high_resolution_clock::now();

    for(int start = 0; start < n; start++)
    {
        // Check overall time before starting a new root
        auto now = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() >= time_limit_sec)
            break;

        std::vector<bool> visited(n, false);
        visited[start] = true;
        rnnDFS(g, visited, start, 1, 0, best, start, startTime, time_limit_sec);
    }
    return best;
}

// ---------------------------------------------------------------------------
// BRUTE-FORCE  –  fixes vertex 0, permutes [1..n-1]
// ---------------------------------------------------------------------------
int bruteForce(const Matrix& g, int time_limit_sec)
{
    int n = (int)g.size();
    std::vector<int> perm(n - 1);
    std::iota(perm.begin(), perm.end(), 1); // [1, 2, ..., n-1]

    int best = INF;
    auto startTime = std::chrono::high_resolution_clock::now();

    do {
        // Build full path: 0 + permutation
        int cost = g[0][perm[0]];
        for(int i = 0; i < (int)perm.size() - 1; i++)
            cost += g[perm[i]][perm[i + 1]];
        cost += g[perm.back()][0];

        if(cost < best) best = cost;

        auto now = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() >= time_limit_sec)
            break;

    } while(std::next_permutation(perm.begin(), perm.end()));

    return best;
}

// ---------------------------------------------------------------------------
// Relative error [%]
// ---------------------------------------------------------------------------
double relativeError(int approx, int optimum)
{
    if(optimum <= 0) return 0.0;
    return 100.0 * (double)(approx - optimum) / (double)optimum;
}