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
    KeywordDistanceMatrix(int W, int V, int max_weight); 
    ~KeywordDistanceMatrix();

    Pair& operator()(int w, int v) const; 
    void calculate_matrix_cpu(SparseGraph<int>* graph);
    void calculate_matrix_gpu(SparseGraph<int>* graph);

    Pair get_size() const;

    void set_batch_cutoff(int i)      { dynamicBatchSizeCutoff = i; }; //!< Set optimization option 
    void set_vertex_chunk_size(int i) { vertexChunkSize = i;        }; //!< Set optimization option
    void set_min_batch_size(int i)    { minBatchSize = i;           }; //!< Set optimization option


private:
    Pair** matrix;      //!< WxV matrix
    int W;              //!< Number of keywords
    int V;              //!< Number of vertices
    int MAX_WEIGHT;

    // Optimization options
    int dynamicBatchSize;               //!< Number of keywords to process at once
    int dynamicBatchSizeCutoff = 1000;  //!< if V > dynamicBatchSizeCutoff then batchSize = minBatchSize
    int vertexChunkSize = 50000;        //!< Maximum number of vertices to process at once
    int minBatchSize = 1;               //!< Minimum keywords to process at once
};

#endif 
