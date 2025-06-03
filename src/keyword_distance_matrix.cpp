#include "keyword_distance_matrix.hpp"
#include <climits>
#include <omp.h>
#include <iostream>

KeywordDistanceMatrix::KeywordDistanceMatrix(int n_W, int n_V) {
    W = n_W;
    V = n_V;
    matrix = new Pair*[W];
    for (int i = 0; i < W; i++) {
        matrix[i] = new Pair[V];
    }
}

KeywordDistanceMatrix::~KeywordDistanceMatrix() {
    if (matrix) {
        for (int i = 0; i < W; i++) {
            delete[] matrix[i];
        }

        delete[] matrix;
    }
}

Pair* KeywordDistanceMatrix::operator()(int w, int v) {
    if (!matrix) return nullptr;

    return &matrix[w][v];
}

Pair KeywordDistanceMatrix::get_size() {
    Pair p = {W, V};
    return p;
}

void KeywordDistanceMatrix::calculate_matrix_cpu(SparseGraph<int>* graph) {
    std::vector<VerboseEdge<int>> list = graph->get_edge_list();

    #pragma omp parallel 
    {
        #pragma omp for
        for (int w = 0; w < W; w++) {
            int dist[V];
            int pred[V];

            for (int v = 0; v < V; v++) {
                dist[v] = INT_MAX;
                pred[v] = -1;

                if (graph->keyword_is_in(w, v)) {
                    dist[v] = 0;
                    pred[v] = v;
                }
            }

            // This part of the algorithm is the most run-time intensive and potentially has the most
            // opportunity for optimization
            for (int counter = 0; counter < V; counter++) {
                for (auto& edge: list) {
                    if (dist[edge.start] + edge.weight < dist[edge.end]) {
                        dist[edge.end] = dist[edge.start] + edge.weight;
                        pred[edge.end] = edge.start;
                    }
                }   
            }

            for (int v = 0; v < V; v++) {
                matrix[w][v].dist = dist[v];
                matrix[w][v].pred = pred[v];
            }
        int tid = omp_get_thread_num();
        std::cout << "Thread " << tid << " completed keyword " << w << std::endl;
        }
    }
}
