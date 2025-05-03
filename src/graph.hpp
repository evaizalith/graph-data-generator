#ifndef EVA_GRAPH
#define EVA_GRAPH

#include <map>
#include <vector>
#include <memory>
#include <stdexcept>

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

    Vertex<T>*      operator[](T);
    Vertex<T>*      operator[](int);

    void            add_vertex(Vertex<T>*);
    void            add_vertex(T id);
    void            add_edge(Vertex<T>* start, Vertex<T>* end);
    void            add_edge(T start, T end);
    
    // Deletes memory associated with vertices
    void            remove_vertex(Vertex<T>*);
    void            remove_vertex(T id);
    // Remove edges
    void            remove_edge(Vertex<T>* start, Vertex<T>* end);
    void            remove_edge(T start, T end);
    
    // Get all vertices connected by an edge
    std::vector<T>  get_adjacent(Vertex<T>*);
    std::vector<T>  get_adjacent(T id);

    bool            vertex_exists(T id);

private:
    T n_vertices;
    std::vector<Vertex<T>*, std::allocator<T>> vertices;
    std::multimap<T, T> adjacency_list; 
};

template <typename T>
SparseGraph<T>::SparseGraph() {
    n_vertices = 0;
}


template <typename T>
SparseGraph<T>::~SparseGraph() {
    for (auto it = vertices.begin(); it != vertices.end(); it++) {
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
    return vertices[i];
}

template <typename T>
Vertex<T>* SparseGraph<T>::operator[](int i) {
    return vertices[dynamic_cast<T>(i)];
}

template <typename T>
void SparseGraph<T>::add_vertex(Vertex<T>* vert) {
    if (vertex_exists(vert->id)) throw std::runtime_error("Vertex already exists");
    vertices[vert->id] = vert;
    ++n_vertices;
}

template <typename T>
void SparseGraph<T>::add_vertex(T id) {
    if (vertex_exists(id)) throw std::runtime_error("Vertex already exists");
    Vertex<T>* vert = new Vertex<T>;
    vert->id = id;
    vertices[id] = vert;
    ++n_vertices;
}

template <typename T>
void SparseGraph<T>::add_edge(Vertex<T>* start, Vertex<T>* end) {
    adjacency_list.insert(start->id, end->id);
}

template <typename T>
void SparseGraph<T>::add_edge(T start, T end) {
    adjacency_list.insert(start, end);
}

template <typename T>
void SparseGraph<T>::remove_vertex(Vertex<T>* vert) {
    try {
        remove_vertex(vert->id);
    } catch (const std::exception e) {
        throw e;
    }
}

template <typename T>
void SparseGraph<T>::remove_vertex(T id) {
    try {
        adjacency_list.erase(id);
        delete vertices[id];
        --n_vertices;
    } catch (const std::exception e) {
        throw e;
    }
}

template <typename T>
void SparseGraph<T>::remove_edge(Vertex<T>* start, Vertex<T>* end) {
    try {
        remove_edge(start->id, end->id);
    } catch (const std::exception e) {
        throw e;
    }
}

template <typename T>
void SparseGraph<T>::remove_edge(T start, T end) {
    try {
        auto range = adjacency_list.equal_range(start);
        for (auto it = range.first; it != range.second;) {
            if (it->second == end) {
                it = adjacency_list.erase(it);
            } else {
                ++it;
            }
        }
    } catch (const std::exception e) {
        throw e;
    }
}

template <typename T>
std::vector<T>  SparseGraph<T>::get_adjacent(Vertex<T>* vert) {
    return get_adjacent(vert->id);
}

template <typename T>
std::vector<T>  SparseGraph<T>::get_adjacent(T id) {
    std::vector<T> vec;

    auto range = adjacency_list.equal_range(id);
    for (auto it = range.first; it != range.second;) {
        vec.push(*it);
        ++it;
    }
}

template <typename T>
bool SparseGraph<T>::vertex_exists(T id) {
    return vertices[id] != NULL;
}

#endif
