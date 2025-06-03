#ifndef EVA_GRAPH_CSV_WRITER
#define EVA_GRAPH_CSV_WRITER

#include "graph.hpp"
#include "keyword_distance_matrix.hpp"

class CSVWriter {
public:
    CSVWriter();

    void write(std::string filepath, const KeywordDistanceMatrix& matrix);
};

#endif
