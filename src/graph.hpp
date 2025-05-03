#ifndef EVA_GRAPH
#define EVA_GRAPH

#include <iostream>
#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <memory>

// Max keyword count should equal 2^N - 1 where N is some integer to ensure that the Vertex struct is packed properly for memory purposes
#define MAX_KEYWORD_COUNT 15

// We make use of templates for the Vertex struct and the SparseGraph class primarily so that the user may select the most memory efficient data type. If you don't need more than 256 unique vertices, you don't need more than a uint8_t, otherwise you might need a uint16_t or a uint32_t, etc.
template <typename T>
struct Vertex {
    T id;
    T keywords[MAX_KEYWORD_COUNT]; 
};

template <typename T>
struct WeightedEdge {
    Vertex<T>* end;
    T weight;
};

// As long as |E| < |V| / 2, this data structure is the most efficient way to store graph data (especially for digraphs), making use of an adjacency list. 
template <typename T>
class SparseGraph {
public:
    SparseGraph();
    SparseGraph(const SparseGraph<T>&);
    ~SparseGraph();

    Vertex<T>*  operator[](T);
    Vertex<T>*  operator[](int);

    friend std::ostream& operator<<(std::ostream&, const SparseGraph<T>&);

    void        push(Vertex<T>*);

private:
    T n_vertices;
    std::vector<Vertex<T>*, std::allocator<T>> vectors;
    std::unordered_map<T, T> adjacency_list; 
};

template <typename T>
SparseGraph<T>::SparseGraph() {
    n_vertices = 0;
}


template <typename T>
SparseGraph<T>::~SparseGraph() {
    for (auto it = vectors.begin(); it != vectors.end(); it++) {
        delete *it;
    }
}

template <typename T>
SparseGraph<T>::SparseGraph(const SparseGraph<T>& rhs) {
    n_vertices = rhs.n_vertices;
    adjacency_list = rhs.adjacency_list;
}

template <typename T>
Vertex<T>* SparseGraph<T>::operator[](T i) {
    return vectors[i];
}

template <typename T>
Vertex<T>* SparseGraph<T>::operator[](int i) {
    return vectors[i];
}

template <typename T>
void SparseGraph<T>::push(Vertex<T>* vertex) {
    vectors.insert(vertex->id, vertex);
}

#endif
