#ifndef EVA_GRAPH_GEN
#define EVA_GRAPH_GEN

#include "graph.hpp"
#include "simd_random.hpp"

// This class randomly generates a graph according to user parameters
// !! THE GRAPH CAN OUTLIVE THE GENERATOR !!
template <typename T = int>
class GraphGenerator {
public:
    GraphGenerator(unsigned seed, float mean, float sigma);
    ~GraphGenerator();

    SparseGraph<T>* generate(T n_vertices, T n_keywords, T min_keywords, T max_keywords, T min_degree, T max_degree, T min_weight, T max_weight); 
    T               distribution(T min, T max);

private:
    unsigned seed;
    float mean;
    float sigma;
    CachedPhiloxAVX2* gen;
};

template <typename T>
GraphGenerator<T>::GraphGenerator(unsigned u_seed, float m, float s) {
    seed = u_seed;
    mean = m;
    sigma = s;
    gen = new CachedPhiloxAVX2(seed);
}


template <typename T>
GraphGenerator<T>::~GraphGenerator() {
    delete gen;
}

template <typename T>
SparseGraph<T>* GraphGenerator<T>::generate(T n_vertices, T n_keywords, T min_keywords, T max_keywords, T min_degree, T max_degree, T min_weight, T max_weight) {
    SparseGraph<T>* graph = new SparseGraph<T>();

    // Generate vertices and populate with keywords
    for (T i = 0; i < n_vertices; ++i) {
        Vertex<T>* vert = new Vertex<T>;
        vert->id = i;
     
        graph->add_vertex(vert);

        T vert_n_keywords = distribution(min_keywords, max_keywords); 
        for (T j = 0; j < vert_n_keywords; ++j) {
            graph->add_keyword(vert, distribution(0, n_keywords));
        }

        T n_edges = distribution(min_degree, max_degree);
        
        for (T j = 0; j < n_edges; ++j) {
            T end = distribution(reinterpret_cast<T>(0), n_vertices);
            T weight = distribution(min_weight, max_weight);
            graph->add_edge(i, end, weight);
        }
    }

    graph->process_keyword_additions();

    return graph;
}

template <typename T>
T GraphGenerator<T>::distribution(T min, T max) {
    T val;
    do {
        uint32_t result = (*gen)(max + 1);
        val = static_cast<T>(result);
    } while (val < min);
    return val;
}

#endif
