#include "keyword_distance_matrix.hpp"
#include <climits>
#include <omp.h>
#include <iostream>

KeywordDistanceMatrix::KeywordDistanceMatrix(int n_W, int n_V, int max_weight) {
    W = n_W;
    V = n_V;
    MAX_WEIGHT = max_weight + 1;
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

Pair KeywordDistanceMatrix::operator()(int w, int v) const {
    return matrix[w][v];
}

Pair KeywordDistanceMatrix::get_size() const {
    Pair p = {W, V};
    return p;
}

void KeywordDistanceMatrix::calculate_matrix_cpu(SparseGraph<int>* graph) {
    std::vector<VerboseEdge<int>> list = graph->get_edge_list();

    #pragma omp parallel 
    {
        #pragma omp for
        for (int w = 0; w < W; w++) {
            unsigned dist[V];
            int pred[V];

            for (int v = 0; v < V; v++) {
                dist[v] = INT_MAX - MAX_WEIGHT;
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

void KeywordDistanceMatrix::calculate_matrix_gpu(SparseGraph<int>* graph) {
    GLuint computeProgram = createShaderProgram("keyword_matrix.comp");
    if (computeProgram == 0) {
        std::cerr << "Unable to compile shaders for " << __func__ << std::endl;
        return;
    }

    std::vector<VerboseEdge<int>> edges = graph->get_edge_list();
    int E = edges.size();
    if (E == 0) {
        std::cerr << "No edges found for " << __func__ << std::endl;
        return;
    }

    std::vector<uint32_t> hasKeywordData(W * V, 0);
    // Efficiently fill using graph's keyword data
    #pragma omp parallel for
    for (int w = 0; w < W; w++) {
        auto vertices = graph->get_vertices_with_keyword(w);
        for (int v : vertices) {
            hasKeywordData[w * V + v] = 1;
        }
    }

    // Create and bind buffers
    GLuint ssbos[8]; // EdgeList, HasKeyword, Dist0, Dist1, Pred0, Pred1, OutputDist, OutputPred
    glGenBuffers(8, ssbos);

    // Buffer 0: Edge List
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VerboseEdge<int>) * E, edges.data(), GL_STATIC_DRAW);

    // Buffer 1: HasKeyword (W*V matrix)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint) * W * V, hasKeywordData.data(), GL_STATIC_DRAW);

    // Buffers 2-7: Initialize with appropriate sizes (W*V elements)
    for (int i = 2; i < 8; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint) * W * V, nullptr, GL_DYNAMIC_DRAW);
    }

    // Set uniform values
    glUniform1ui(glGetUniformLocation(computeProgram, "V"), V);
    glUniform1ui(glGetUniformLocation(computeProgram, "E"), E);
    glUniform1ui(glGetUniformLocation(computeProgram, "W"), W);

    // Dispatch compute shader
    glUseProgram(computeProgram);
    glDispatchCompute(W, 1, 1); // One work group per keyword
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Retrieve results from OutputDist/OutputPred
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[6]);
    uint* distResults = (uint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[7]);
    int* predResults = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    // Copy data to matrix
    for (int w = 0; w < W; ++w) {
        for (int v = 0; v < V; ++v) {
            const int index = w * V + v;
            matrix[w][v] = {
                (int)predResults[index],
                (int)distResults[index]
            };
        }
    }

    // Unmap buffers
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
