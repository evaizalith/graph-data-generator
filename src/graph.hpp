#ifndef EVA_GRAPH
#define EVA_GRAPH

#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <stdexcept>
#include <imgui.h>
#include <queue> 
#include <new>
#include <memory>

// Max keyword count should equal 2^N - 1 where N is some integer to ensure that the Vertex struct is packed properly for memory purposes
#define MAX_KEYWORD_COUNT 15

//! Basic graph parameter class with some default values
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

//! We make use of templates for the Vertex struct and the SparseGraph class primarily so that the user may select the most memory efficient data type. If you don't need more than 256 unique vertices, you don't need more than a uint8_t, otherwise you might need a uint16_t or a uint32_t, etc.
template <typename T>
struct alignas(sizeof(T)) Vertex {
    T id;
};

template <typename T>
struct alignas(2 * sizeof(T)) Edge {
    T end;
    T weight;
};

template <typename T>
struct VerboseEdge {
    T start;
    T end;
    T weight; 
};

template <typename T>
struct alignas(2 * sizeof(T)) KeywordPair {
    T vert;
    T keyword;
};

/*! As long as |E| < |V| / 2, this data structure is the most efficient way to store graph data (especially for digraphs), making use of an adjacency list. 
 *This data structure has been implemented to best make use of the CPU's cache, sometimes at the
 *expense of usability, such that it's extremely efficient. 
 */
template <typename T>
class SparseGraph {
public:
    SparseGraph();
    SparseGraph(const SparseGraph<T>&);
    ~SparseGraph();

    Vertex<T>*      operator[](T);                                 //!< Get vertex by id

    void            add_vertex(Vertex<T>*);
    void            add_vertex(T id);
    void            add_edge(Vertex<T>* start, Vertex<T>* end, T weight);
    void            add_edge(T start, T end, T weight);
    void            add_keyword(Vertex<T>*, T word);               //!< Adds keywords to a queue to be added to the graph
    void            add_keyword(T id, T word);
    void            process_keyword_additions();                   //!< Register keyword additions

    void            remove_vertex(Vertex<T>*);                     //!< Deletes memory associated with vertices
    void            remove_vertex(T id);
    void            remove_edge(Vertex<T>* start, Vertex<T>* end); //!< Remove edge between two vertices 
    void            remove_edge(T start, T end);                   
    std::vector<Edge<T>>  get_adjacent(Vertex<T>*);                //!< Get all vertices connected by an edge
    std::vector<Edge<T>>  get_adjacent(T id);
    std::vector<T>        get_keywords(T id);

    std::vector<VerboseEdge<T>> get_edge_list();                   //!< Produce a new adjacency list  
    std::vector<T>              get_vertices_with_keyword(int w) const;  //!< Gets all vertices containing a keyword

    bool            vertex_exists(T id);
    bool            keyword_is_in(T w, T v);

    template <class U>friend std::ostream& operator<<(std::ostream& os, SparseGraph<U>&);

    T                                                   n_vertices;
    std::vector<Vertex<T>*, std::allocator<Vertex<T>*>> vertices;
    std::multimap<T, Edge<T>>                           adjacency_list;
    std::multimap<T, T>                                 keyword_index;
    std::multimap<T, T>                                 reverse_index;  //!< Stores a list of every vertex with a given keyword
    std::queue<KeywordPair<T>>                          keyword_add_queue;
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
    // Check for duplicates in reverse_index, return early if found
    KeywordPair<T> pair = {vert->id, word};
    keyword_add_queue.push(pair);
}

template <typename T>
void SparseGraph<T>::add_keyword(T id, T word) {
    add_keyword(&vertices[id], word);
}

template <typename T>
void SparseGraph<T>::process_keyword_additions() {
    T i = 0;
    while (!keyword_add_queue.empty()) {
        KeywordPair<T> pair = keyword_add_queue.front();
        keyword_add_queue.pop();
        auto range = reverse_index.equal_range(pair.keyword);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == pair.vert) {
                continue;
            }
        }

        keyword_index.insert({pair.vert, pair.keyword});
        reverse_index.insert({pair.keyword, pair.vert});

        ++i;
    }

    std::queue<KeywordPair<T>> emptyQueue;
    keyword_add_queue.swap(emptyQueue);
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
std::vector<T> SparseGraph<T>::get_keywords(T id) {
    std::vector<T> result;
    auto range = keyword_index.equal_range(id);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

template <typename T>
std::vector<VerboseEdge<T>> SparseGraph<T>::get_edge_list() {
    std::vector<VerboseEdge<T>> list;

    for (auto vert : vertices) {
        auto range = adjacency_list.equal_range(vert->id);
        for(auto it = range.first; it != range.second; ++it) {
            VerboseEdge<T> edge = { vert->id, (*it).second.end, (*it).second.weight };
            list.push_back(edge);
        }
    }

    return list; 
}

template <typename T>
std::vector<T> SparseGraph<T>::get_vertices_with_keyword(int w) const {
    std::vector<T> result;
    auto range = reverse_index.equal_range(w);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }

    return result;
}


template <typename T>
bool SparseGraph<T>::vertex_exists(T id) {
    try {
        return vertices.at(id) != NULL;
    } catch (const std::out_of_range& e) {
        return false;
    }
}

template <typename T>
bool SparseGraph<T>::keyword_is_in(T w, T v) {
    auto range = keyword_index.equal_range(v);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == w) {
            return true;
        }
    } 

    return false;
}

template <class T>
std::ostream& operator<<(std::ostream& os, SparseGraph<T>& graph) {
        for (Vertex<T>* vert : graph.vertices) {
            os << "(id: " << vert->id << ", adj: <";
            for (Edge<T> adj : graph.get_adjacent(vert->id)) {
                os << "(" << adj.end << ", " << adj.weight << ")" << " ";
            }
            os << ">" << ", keywords: ";
            std::vector<T> keywords = graph.get_keywords(vert->id);
            for (const auto& word : keywords) {
                os << word << " ";
            }
            os << ");" << std::endl;
        }

        return os;
    }

#endif
