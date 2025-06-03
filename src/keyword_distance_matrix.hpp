#ifndef EVA_KEYWORD_DISTANCE_MATRIX
#define EVA_KEYWORD_DISTANCE_MATRIX

#include "graph.hpp"
#include "shader_util.hpp"

/*! This class is used to generate a WxV matrix where each cell represents the distance
   between a vertex v_i and a keyword w_j, stored as a pair (v_j, Dist(v_i, v_j)) where
   v_j is the predecessor of the closest vertex containing keyword w_j 
*/

struct alignas(16) Pair {
    int pred;             //!< ID of predecessor vertex
    int dist;             //!< Distance to closest vertex
};

class KeywordDistanceMatrix {
public:
    KeywordDistanceMatrix(int W, int V); 
    ~KeywordDistanceMatrix();

    Pair* operator()(int w, int v); 
    void calculate_matrix_cpu(SparseGraph<int>* graph);
    void calculate_matrix_gpu(SparseGraph<int>* graph);

    Pair get_size();

private:
    Pair** matrix;      //!< WxV matrix
    int W;              //!< Number of keywords
    int V;              //!< Number of vertices
};

#endif 
