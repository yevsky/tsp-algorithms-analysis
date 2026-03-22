#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    std::string algorithms;
    std::string instances;
    int rand_iterations = 100000;
    bool generate_random = false;
    bool asymmetric = false;
    int instances_count = 10;
    int n_start = 6;
    int n_end = 15;
    int step = 1;
    std::string output = "results/results.csv";
    int timeout_brute = 30;  // minutes
    int timeout_rnn = 15;    // minutes
    int timeout_rand = 30;   // minutes
};

Config loadConfig(const std::string& filename);

#endif // CONFIG_H