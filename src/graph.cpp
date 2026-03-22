#include "graph.h"
#include <random>
#include <numeric>

Matrix generateGraph(int n, bool asymmetric)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);

    Matrix g(n, std::vector<int>(n, 0));

    for(int i = 0; i < n; i++)
    {
        for(int j = 0; j < n; j++)
        {
            if(i == j)
            {
                g[i][j] = 0;
            }
            else if(asymmetric)
            {
                g[i][j] = dist(gen);
            }
            else
            {
                // Symmetric: fill upper triangle first, mirror below
                if(j > i)
                    g[i][j] = dist(gen);
                else
                    g[i][j] = g[j][i];
            }
        }
    }
    return g;
}

int pathCost(const Matrix& g, const std::vector<int>& path)
{
    int n = (int)path.size();
    int cost = 0;
    for(int i = 0; i < n - 1; i++)
        cost += g[path[i]][path[i + 1]];
    cost += g[path[n - 1]][path[0]];
    return cost;
}