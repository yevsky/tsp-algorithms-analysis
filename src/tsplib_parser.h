#ifndef TSP_PROJECT_TSPLIB_PARSER_H
#define TSP_PROJECT_TSPLIB_PARSER_H

#include "graph.h"
#include <string>

// Load a TSP or ATSP instance from a TSPLIB-format file.
// Supports:
//   EDGE_WEIGHT_TYPE: EUC_2D, ATT, CEIL_2D, GEO (coordinate sections)
//   EDGE_WEIGHT_TYPE: EXPLICIT  with EDGE_WEIGHT_FORMAT: FULL_MATRIX / UPPER_ROW / LOWER_DIAG_ROW
// Returns an empty matrix on failure.
Matrix loadTSPLIB(const std::string& filename);

#endif // TSP_PROJECT_TSPLIB_PARSER_H