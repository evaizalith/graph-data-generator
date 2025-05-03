#ifndef EVA_GRAPH_GEN
#define EVA_GRAPH_GEN

#include "graph.hpp"
#include <random>

// This class randomly generates a graph according to user parameters
// !! THE GRAPH CAN OUTLIVE THE GENERATOR !!
template <typename T = int>
class GraphGenerator {
public:
    GraphGenerator(int seed, float mean, float sigma);
    ~GraphGenerator();

    SparseGraph<T> generate(T n_vertices, T n_keywords, T min_keywords, T max_keywords, T min_degree, T max_degree); 

private:
    std::mt19937* gen;
    std::normal_distribution<T>* distribution;
};

template <typename T>
GraphGenerator<T>::GraphGenerator(int seed, float mean, float sigma) {
    gen = new std::mt19937(seed);
    distribution = new std::normal_distribution<T>(mean, sigma);
}

template <typename T>
GraphGenerator<T>::~GraphGenerator() {
    delete gen;
    delete distribution;
}

template <typename T>
SparseGraph<T> GraphGenerator<T>::generate(T n_vertices, T n_keywords, T min_keywords, T max_keywords, T min_degree, T max_degree) {
    SparseGraph<T>* graph = new SparseGraph<T>();

    // Generate vertices and populate with keywords
    for (T i = 0; i < n_vertices; ++i) {
        Vertex<T>* vert = new Vertex<T>;
        vert->id = i;
        
        T vert_n_keywords = distribution(min_keywords, max_keywords); 
        for (T j = 0; j < vert_n_keywords; ++j) {
            vert->keywords[j] = distribution(dynamic_cast<T>(1), n_keywords);
        }

        graph->add_vertex(vert);
    }

    // Generate edges
    for (T i = 0; i < n_vertices; ++i) {
        T n_edges = distribution(min_degree, max_degree);
        
        for (T j = 0; j < n_edges; ++j) {
            T end = distribution(dynamic_cast<T>(0), n_vertices);
            graph->add_edge(i, end);
        }
    }

    return graph;
}

#endif
