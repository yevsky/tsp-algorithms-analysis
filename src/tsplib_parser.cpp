#include "tsplib_parser.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <algorithm>

// Trim whitespace
static std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if(a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

// Uppercase a string
static std::string toUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// EUC_2D distance (TSPLIB standard rounding)
static int euc2d(double x1, double y1, double x2, double y2)
{
    double dx = x1 - x2, dy = y1 - y2;
    return (int)(std::sqrt(dx*dx + dy*dy) + 0.5);
}

// CEIL_2D distance
static int ceil2d(double x1, double y1, double x2, double y2)
{
    double dx = x1 - x2, dy = y1 - y2;
    return (int)std::ceil(std::sqrt(dx*dx + dy*dy));
}

// ATT pseudo-Euclidean distance
static int att(double x1, double y1, double x2, double y2)
{
    double dx = x1 - x2, dy = y1 - y2;
    double r = std::sqrt((dx*dx + dy*dy) / 10.0);
    int t = (int)r;
    return (t < r) ? t + 1 : t;
}

// GEO distance (geographic coordinates in degrees)
static double toRad(double x)
{
    int deg = (int)x;
    double min = x - deg;
    return M_PI * (deg + 5.0 * min / 3.0) / 180.0;
}
static int geo(double lat1, double lon1, double lat2, double lon2)
{
    const double RRR = 6378.388;
    double q1 = std::cos(toRad(lon1) - toRad(lon2));
    double q2 = std::cos(toRad(lat1) - toRad(lat2));
    double q3 = std::cos(toRad(lat1) + toRad(lat2));
    return (int)(RRR * std::acos(0.5*((1.0+q1)*q2 - (1.0-q1)*q3)) + 1.0);
}

Matrix loadTSPLIB(const std::string& filename)
{
    std::ifstream file(filename);
    if(!file.is_open())
    {
        std::cerr << "[PARSER] Cannot open file: " << filename << std::endl;
        return {};
    }

    int dimension = 0;
    std::string edgeWeightType   = "EUC_2D";
    std::string edgeWeightFormat = "FULL_MATRIX";

    std::vector<std::pair<double,double>> coords;
    Matrix g;

    bool inCoordSection  = false;
    bool inWeightSection = false;

    // --- Variables used while reading EDGE_WEIGHT_SECTION ---
    // (local, NOT static – fixes the critical bug from the original code)
    int row = 0, col = 0;

    std::string line;
    while(std::getline(file, line))
    {
        line = trim(line);
        if(line.empty() || line == "EOF") break;

        std::string lineUp = toUpper(line);

        // ---- Header keywords ----
        if(lineUp.find("DIMENSION") != std::string::npos)
        {
            auto pos = line.find(':');
            if(pos != std::string::npos)
                dimension = std::stoi(trim(line.substr(pos + 1)));
            continue;
        }
        if(lineUp.find("EDGE_WEIGHT_FORMAT") != std::string::npos)
        {
            auto pos = line.find(':');
            if(pos != std::string::npos)
                edgeWeightFormat = toUpper(trim(line.substr(pos + 1)));
            continue;
        }
        if(lineUp.find("EDGE_WEIGHT_TYPE") != std::string::npos)
        {
            auto pos = line.find(':');
            if(pos != std::string::npos)
                edgeWeightType = toUpper(trim(line.substr(pos + 1)));
            continue;
        }

        // ---- Section markers ----
        if(lineUp == "NODE_COORD_SECTION")
        {
            inCoordSection  = true;
            inWeightSection = false;
            continue;
        }
        if(lineUp == "EDGE_WEIGHT_SECTION")
        {
            inWeightSection = true;
            inCoordSection  = false;
            if(dimension > 0)
                g.assign(dimension, std::vector<int>(dimension, 0));
            row = 0; col = 0; // IMPORTANT: reset here, not as static
            continue;
        }
        if(lineUp == "DISPLAY_DATA_SECTION" ||
           lineUp == "TOUR_SECTION"         ||
           lineUp == "DEMAND_SECTION")
        {
            inCoordSection  = false;
            inWeightSection = false;
            continue;
        }

        // ---- Read coordinate section ----
        if(inCoordSection)
        {
            std::stringstream ss(line);
            int id; double x, y;
            if(ss >> id >> x >> y)
                coords.push_back({x, y});
            continue;
        }

        // ---- Read explicit weight matrix ----
        if(inWeightSection && dimension > 0)
        {
            std::stringstream ss(line);
            int val;
            while(ss >> val)
            {
                if(edgeWeightFormat == "FULL_MATRIX")
                {
                    g[row][col] = val;
                    col++;
                    if(col == dimension) { col = 0; row++; }
                }
                else if(edgeWeightFormat == "UPPER_ROW")
                {
                    // row < col  (diagonal = 0)
                    g[row][col] = val;
                    g[col][row] = val;
                    col++;
                    if(col == dimension) { row++; col = row + 1; }
                }
                else if(edgeWeightFormat == "LOWER_DIAG_ROW")
                {
                    g[row][col] = val;
                    if(row != col) g[col][row] = val;
                    col++;
                    if(col > row) { row++; col = 0; }
                }
            }
            continue;
        }
    }

    // ---- Build distance matrix from coordinates ----
    if(!coords.empty() && dimension > 0)
    {
        g.assign(dimension, std::vector<int>(dimension, 0));
        for(int i = 0; i < dimension; i++)
        {
            for(int j = 0; j < dimension; j++)
            {
                if(i == j) { g[i][j] = 0; continue; }

                double xi = coords[i].first,  yi = coords[i].second;
                double xj = coords[j].first,  yj = coords[j].second;

                if(edgeWeightType == "EUC_2D")
                    g[i][j] = euc2d(xi, yi, xj, yj);
                else if(edgeWeightType == "CEIL_2D")
                    g[i][j] = ceil2d(xi, yi, xj, yj);
                else if(edgeWeightType == "ATT")
                    g[i][j] = att(xi, yi, xj, yj);
                else if(edgeWeightType == "GEO")
                    g[i][j] = geo(xi, yi, xj, yj);
                else
                    g[i][j] = euc2d(xi, yi, xj, yj); // fallback
            }
        }
    }

    if(g.empty())
        std::cerr << "[PARSER] Warning: empty matrix for " << filename << std::endl;

    return g;
}