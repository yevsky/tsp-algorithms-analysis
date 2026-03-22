#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
#include <filesystem>
#include <climits>

#include "config.h"
#include "graph.h"
#include "tsp_algorithms.h"
#include "tsplib_parser.h"

// ---------------------------------------------------------------------------
// Known optimal values for TSPLIB / VLSI instances
// ---------------------------------------------------------------------------
static const std::map<std::string, int> KNOWN_OPTIMA = {
    // TSPLIB symmetric
    {"berlin52.tsp", 7542},
    {"eil51.tsp", 426},
    {"eil76.tsp", 538},
    {"eil101.tsp", 629},
    {"kroA100.tsp", 21282},
    {"kroB100.tsp", 22141},
    {"pr76.tsp", 108159},
    {"pr1002.tsp", 259045},
    // TSPLIB asymmetric
    {"br17.atsp", 39},
    {"ft53.atsp", 6905},
    {"ft70.atsp", 38673},
    {"ftv70.atsp", 1950},
    {"kro124p.atsp", 36230},
    {"rbg443.atsp", 2720},
    // VLSI
    {"xqf131.tsp", 564},
    {"pma343.tsp", 1368},
    {"dhb3386.tsp", 11137},
    {"lap7454.tsp", 19535},
    {"dga9698.tsp", 27724},
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
        if (!item.empty()) elems.push_back(item);
    return elems;
}

static std::string basename(const std::string &path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

static void writeRow(std::ofstream &csv,
                     const std::string &instance,
                     const std::string &algo,
                     int n,
                     long long time_ms,
                     int cost,
                     double rel_error) {
    csv << instance << ","
            << algo << ","
            << n << ","
            << time_ms << ","
            << cost << ","
            << rel_error << "\n";
    csv.flush();
}

static int runAlgo(const std::string &algo,
                   const Matrix &g,
                   const Config &cfg,
                   long long &elapsed_ms) {
    int cost = 0;
    auto t0 = std::chrono::high_resolution_clock::now();

    if (algo == "BRUTE") {
        cost = bruteForce(g, cfg.timeout_brute * 60);
    } else if (algo == "RAND") {
        cost = randTSP(g, cfg.rand_iterations, cfg.timeout_rand * 60);
    } else if (algo == "NN") {
        // Run from every start vertex, keep best
        int best = INT_MAX;
        for (int s = 0; s < (int) g.size(); s++) {
            int c = nearestNeighbor(g, s);
            if (c < best) best = c;
        }
        cost = best;
    } else if (algo == "RNN") {
        cost = RNN(g, cfg.timeout_rnn * 60);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    return cost;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    Config cfg = loadConfig("config.txt");

    // Ensure output directory exists
    {
        std::filesystem::path outPath(cfg.output);
        if (outPath.has_parent_path())
            std::filesystem::create_directories(outPath.parent_path());
    }

    std::ofstream csv(cfg.output);
    if (!csv.is_open()) {
        std::cerr << "[MAIN] Błąd CSV: " << cfg.output << std::endl;
        return 1;
    }
    csv << "instance,algorithm,n,time_ms,cost,rel_error\n";

    std::vector<std::string> algorithms = split(cfg.algorithms, ',');

    // -------------------------------------------------------------------------
    // Part A: TSPLIB instances – RAND, NN, RNN only (BRUTE always skipped)
    // -------------------------------------------------------------------------
    if (!cfg.instances.empty()) {
        std::cout << "\n==============================" << std::endl;
        std::cout << "  Czesc A: Instancje TSPLIB" << std::endl;
        std::cout << "==============================" << std::endl;

        std::vector<std::string> instanceFiles = split(cfg.instances, ',');

        for (auto &file: instanceFiles) {
            std::cout << "\n=== Plik: " << file << " ===" << std::endl;
            Matrix g = loadTSPLIB(file);
            if (g.empty()) {
                std::cerr << "[MAIN] Zła instancja: " << file << std::endl;
                continue;
            }
            int n = (int) g.size();

            std::string bname = basename(file);
            int optimum = -1;
            if (KNOWN_OPTIMA.count(bname))
                optimum = KNOWN_OPTIMA.at(bname);

            for (auto &algo: algorithms) {
                // BRUTE is never run on TSPLIB files
                if (algo == "BRUTE") {
                    std::cout << "  [BRUTE] Ignorujemy (TSPLIB instance)" << std::endl;
                    continue;
                }

                std::cout << "  [" << algo << "] n=" << n << " ... " << std::flush;

                long long elapsed_ms = 0;
                int cost = runAlgo(algo, g, cfg, elapsed_ms);

                double rel = (optimum > 0) ? relativeError(cost, optimum) : -1.0;
                writeRow(csv, bname, algo, n, elapsed_ms, cost, rel);

                std::cout << "cost=" << cost << " time=" << elapsed_ms << "ms";
                if (optimum > 0) std::cout << " err=" << rel << "%";
                std::cout << std::endl;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Part B: Generated random instances – BRUTE FORCE only
    // Generates cfg.instances_count symmetric (TSP) and asymmetric (ATSP)
    // graphs for each n in [n_start, n_end], runs only BRUTE on each.
    // -------------------------------------------------------------------------
    if (cfg.generate_random) {
        std::cout << "\n==============================" << std::endl;
        std::cout << "  Czesc B: Generowane instancje (Wyłącznie BRUTE)" << std::endl;
        std::cout << "==============================" << std::endl;

        for (int pass = 0; pass < 2; pass++) {
            bool asym = (pass == 1);
            std::string typeLabel = asym ? "ATSP" : "TSP";

            std::cout << "\n--- " << typeLabel << " Instancje ---" << std::endl;

            for (int n = cfg.n_start; n <= cfg.n_end; n += cfg.step) {
                for (int inst = 1; inst <= cfg.instances_count; inst++) {
                    Matrix g = generateGraph(n, asym);
                    std::string instName = typeLabel
                                           + "_n" + std::to_string(n)
                                           + "_inst" + std::to_string(inst);

                    std::cout << "  [BRUTE] " << instName << " ... " << std::flush;

                    long long elapsed_ms = 0;
                    int cost = runAlgo("BRUTE", g, cfg, elapsed_ms);

                    // rel_error = 0.0 because brute-force IS the optimum
                    writeRow(csv, instName, "BRUTE", n, elapsed_ms, cost, 0.0);

                    std::cout << "cost=" << cost
                            << " time=" << elapsed_ms << "ms" << std::endl;
                }
            }
        }
    }

    csv.close();
    std::cout << "\nAll done. Results saved to: " << cfg.output << std::endl;
    return 0;
}
