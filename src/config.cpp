#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Trim whitespace from both ends of a string
static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if(start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

Config loadConfig(const std::string& filename)
{
    Config cfg;
    std::ifstream file(filename);
    if(!file.is_open())
    {
        std::cerr << "[CONFIG] Cannot open config file: " << filename << std::endl;
        return cfg;
    }

    std::string line;
    while(std::getline(file, line))
    {
        line = trim(line);
        // Skip empty lines and comments
        if(line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if(pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, pos));
        std::string val = trim(line.substr(pos + 1));

        // Strip inline comments (anything after ' #')
        auto comment = val.find(" #");
        if(comment != std::string::npos) val = trim(val.substr(0, comment));

        if(key == "algorithms")        cfg.algorithms = val;
        if(key == "instances")         cfg.instances  = val;
        if(key == "rand_iterations")   cfg.rand_iterations  = stoi(val);
        if(key == "generate_random")   cfg.generate_random  = (val == "true");
        if(key == "asymmetric")        cfg.asymmetric       = (val == "true");
        if(key == "instances_count")   cfg.instances_count  = stoi(val);
        if(key == "n_start")           cfg.n_start          = stoi(val);
        if(key == "n_end")             cfg.n_end            = stoi(val);
        if(key == "step")              cfg.step             = stoi(val);
        if(key == "output")            cfg.output           = val;
        if(key == "timeout_brute")     cfg.timeout_brute    = stoi(val);
        if(key == "timeout_rnn")       cfg.timeout_rnn      = stoi(val);
        if(key == "timeout_rand")      cfg.timeout_rand     = stoi(val);
    }
    return cfg;
}