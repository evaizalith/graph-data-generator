#ifndef EVA_GRAPH_GEN
#define EVA_GRAPH_GEN

#include "graph.hpp"
#include <random>
#include <cmath>

// This class randomly generates a graph according to user parameters
// !! THE GRAPH CAN OUTLIVE THE GENERATOR !!
template <typename T = int>
class GraphGenerator {
public:
    GraphGenerator(unsigned seed, float mean, float sigma);
   // ~GraphGenerator();

    SparseGraph<T>* generate(T n_vertices, T n_keywords, T min_keywords, T max_keywords, T min_degree, T max_degree, T min_weight, T max_weight); 
    T               distribution(T min, T max);

private:
    unsigned seed;
    float mean;
    float sigma;
};

template <typename T>
GraphGenerator<T>::GraphGenerator(unsigned u_seed, float m, float s) {
    seed = u_seed;
    mean = m;
    sigma = s;
}

/*
template <typename T>
GraphGenerator<T>::~GraphGenerator() {
    delete gen;
}*/

template <typename T>
SparseGraph<T>* GraphGenerator<T>::generate(T n_vertices, T n_keywords, T min_keywords, T max_keywords, T min_degree, T max_degree, T min_weight, T max_weight) {
    SparseGraph<T>* graph = new SparseGraph<T>();

    // Generate vertices and populate with keywords
    for (T i = 0; i < n_vertices; ++i) {
        Vertex<T>* vert = new Vertex<T>;
        vert->id = i;
     
        // We use -1 to represent null values of keywords here
        for (T j = 0; j < MAX_KEYWORD_COUNT; ++j) {
            vert->keywords[j] = -1;
        }

        T vert_n_keywords = distribution(min_keywords, max_keywords); 
        for (T j = 0; j < vert_n_keywords; ++j) {
            vert->keywords[j] = distribution(0, n_keywords);
        }

        graph->add_vertex(vert);
    }

    // Generate edges
    for (T i = 0; i < n_vertices; ++i) {
        T n_edges = distribution(min_degree, max_degree);
        
        for (T j = 0; j < n_edges; ++j) {
            T end = distribution(reinterpret_cast<T>(0), n_vertices);
            T weight = distribution(min_weight, max_weight);
            graph->add_edge(i, end, weight);
        }
    }

    return graph;
}

// Quick implementation. Later this needs to be changed to a Strategy pattern to allow for
// different distributions to be used
template <typename T>
T GraphGenerator<T>::distribution(T min, T max) {
    std::mt19937 gen(seed++);
    std::normal_distribution<float> norm(mean, sigma);
    T value;
    do {
        value = static_cast<T>(std::lround(norm(gen)));
    } while (value < min || value > max);
    return value;
}

#endif
