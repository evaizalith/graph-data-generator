#ifndef EVA_GRAPH
#define EVA_GRAPH

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <memory>
#include <stdexcept>
#include <imgui.h>

// Max keyword count should equal 2^N - 1 where N is some integer to ensure that the Vertex struct is packed properly for memory purposes
#define MAX_KEYWORD_COUNT 15

// Basic graph parameter class with some default values
struct GraphParameters {
    int n_vertices = 30;
    int n_keywords = 10;
    int min_degree = 1;
    int max_degree = 5;
    int min_keywords = 1;
    int max_keywords = 5;
    int min_weight = 1;
    int max_weight = 10;
    ImVec4 vertex_color = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 edge_color = ImVec4(0.0f, 1.0f, 0.0f, 1.00f);
};

// We make use of templates for the Vertex struct and the SparseGraph class primarily so that the user may select the most memory efficient data type. If you don't need more than 256 unique vertices, you don't need more than a uint8_t, otherwise you might need a uint16_t or a uint32_t, etc.
template <typename T>
struct Vertex {
    T id;
    std::set<T> keywords;
};

template <typename T>
struct Edge {
    T end;
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

    void            add_vertex(Vertex<T>*);
    void            add_vertex(T id);
    void            add_edge(Vertex<T>* start, Vertex<T>* end, T weight);
    void            add_edge(T start, T end, T weight);
    void            add_keyword(Vertex<T>*, T word);
    void            add_keyword(T id, T word);

    
    // Deletes memory associated with vertices
    void            remove_vertex(Vertex<T>*);
    void            remove_vertex(T id);
    // Remove edges
    void            remove_edge(Vertex<T>* start, Vertex<T>* end);
    void            remove_edge(T start, T end);
    
    // Get all vertices connected by an edge
    std::vector<Edge<T>>  get_adjacent(Vertex<T>*);
    std::vector<Edge<T>>  get_adjacent(T id);

    bool            vertex_exists(T id);

    template <class U>friend std::ostream& operator<<(std::ostream& os, SparseGraph<U>&);

    T                                                   n_vertices;
    std::vector<Vertex<T>*, std::allocator<Vertex<T>*>> vertices;
    std::multimap<T, Edge<T>>                           adjacency_list; 
    std::multimap<T, T>                                 reverse_index;  // Stores a list of every vertex with a given keyword 
};

template <typename T>
SparseGraph<T>::SparseGraph() {
    n_vertices = 0;
}


template <typename T>
SparseGraph<T>::~SparseGraph() {
    adjacency_list.clear();

    for (Vertex<T>* vert : vertices) {
        delete vert;
    }

    vertices.clear();
    vertices.shrink_to_fit();
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
void SparseGraph<T>::add_vertex(Vertex<T>* vert) {
    if (vertex_exists(vert->id)) throw std::runtime_error("Vertex already exists");
    vertices.insert(vertices.begin() + vert->id, vert);
    ++n_vertices;
}

template <typename T>
void SparseGraph<T>::add_vertex(T id) {
    if (vertex_exists(id)) throw std::runtime_error("Vertex already exists");
    Vertex<T>* vert = new Vertex<T>;
    vert->id = id;
    vertices.insert(vertices.begin() + id, vert);
    ++n_vertices;
}

template <typename T>
void SparseGraph<T>::add_keyword(Vertex<T>* vert, T word) {
    vert->keywords.insert(word);
   
    // Check for duplicates in reverse_index, return early if found
    auto range = reverse_index.equal_range(word);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == vert->id) {
            return;
        }
    }

    reverse_index.insert({word, vert->id});
}

template <typename T>
void SparseGraph<T>::add_keyword(T id, T word) {
    add_keyword(&vertices[id], word);
}

template <typename T>
void SparseGraph<T>::add_edge(Vertex<T>* start, Vertex<T>* end, T weight) {
    add_edge(start->id, end->id, weight);
}

template <typename T>
void SparseGraph<T>::add_edge(T start, T end, T weight) {
    Edge<T> edge = {end, weight};
    adjacency_list.insert({start, edge});
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
            if (it->second.end == end) {
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
std::vector<Edge<T>>  SparseGraph<T>::get_adjacent(Vertex<T>* vert) {
    return get_adjacent(vert->id);
}

template <typename T>
std::vector<Edge<T>>  SparseGraph<T>::get_adjacent(T id) {
    std::vector<Edge<T>> vec;

    auto range = adjacency_list.equal_range(id);
    for (auto it = range.first; it != range.second;) {
        vec.push_back((*it).second);
        ++it;
    }

    return vec;
}

template <typename T>
bool SparseGraph<T>::vertex_exists(T id) {
    try {
        return vertices.at(id) != NULL;
    } catch (const std::out_of_range& e) {
        return false;
    }
}

template <class T>
std::ostream& operator<<(std::ostream& os, SparseGraph<T>& graph) {
        for (Vertex<T>* vert : graph.vertices) {
            os << "(id: " << vert->id << ", adj: <";
            for (Edge<T> adj : graph.get_adjacent(vert->id)) {
                os << "(" << adj.end << ", " << adj.weight << ")" << " ";
            }
            os << ">" << ", keywords: ";
            for (const auto& word : vert->keywords) {
                os << word << " ";
            }
            os << ");" << std::endl;
        }

        return os;
    }

#endif
