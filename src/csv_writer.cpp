#include <iostream>
#include <fstream>
#include <stdexcept>
#include "csv_writer.hpp"

CSVWriter::CSVWriter() {

}

void CSVWriter::write(std::string filepath, const KeywordDistanceMatrix& mat) {
    Pair p = mat.get_size();
    int W = p.pred;
    int V = p.dist; 

    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create file " + filepath);
        return;
    }

    for (int i = 0; i < W; i++) {
        for (int j = 0; j < V; j++) {
           p = mat(i, j);
           file << p.dist << ";" << p.pred << ",";
        }
        file << "\n";
    }

    file.close();
    std::cout << "Successfully wrote " << filepath << std::endl;
}
