#include "keyword_distance_matrix.hpp"
#include <omp.h>
#include <iostream>

const int BIG_NUMBER = 0x7FFFFFFF;

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
                dist[v] = BIG_NUMBER;
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

void setUniforms(GLuint computeProgram, int V, int E, int W) {
    glUseProgram(computeProgram);

    auto setUniformChecked = [&](const char* name, GLuint value) {
        GLint loc = glGetUniformLocation(computeProgram, name);
        if (loc != -1) {
            glUniform1ui(loc, value);
            std::cout << "Set uniform " << name << " = " << value << std::endl;
        } else {
            std::cerr << "Warning: Uniform " << name << " not found (may be optimized out)" << std::endl;
        }
    };

    setUniformChecked("V", V);
    setUniformChecked("E", E);
    setUniformChecked("W", W);
}

void KeywordDistanceMatrix::calculate_matrix_gpu(SparseGraph<int>* graph) {
    GLuint computeProgram = createShaderProgram("keyword_matrix.comp");
    if (computeProgram == 0) {
        std::cerr << "Unable to compile shaders for " << __func__ << std::endl;
        return;
    }

    GLint status;
    glGetProgramiv(computeProgram, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLchar log[1024];
        glGetProgramInfoLog(computeProgram, sizeof(log), nullptr, log);
        std::cerr << "Program link failed:\n" << log << std::endl;
    }

    std::vector<VerboseEdge<int>> edges = graph->get_edge_list();
    int E = edges.size();
    if (E == 0) {
        std::cerr << "No edges found for " << __func__ << std::endl;
        return;
    }

    // Create and bind buffers
    GLuint ssbos[10]; // EdgeList, HasKeyword, Dist0, Dist1, Pred0, Pred1, OutputDist, OutputPred
    glGenBuffers(10, ssbos);

    // Buffer 0: Edge List
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VerboseEdge<int>) * E, edges.data(), GL_STATIC_DRAW);

    std::vector<uint32_t> hasKeywordData(W * V, 0);
    #pragma omp parallel for
    for (int w = 0; w < W; w++) {
        auto vertices = graph->get_vertices_with_keyword(w);
        for (int v : vertices) {
            hasKeywordData[w * V + v] = 1;
        }
    }

    // Buffer 1: HasKeyword (W*V matrix)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint) * hasKeywordData.size(), hasKeywordData.data(), GL_STATIC_DRAW);

    // Buffers 2-5: Double buffering (initialize to max distance)
    GLsizeiptr matrixSize = W * V * sizeof(uint32_t);
    for (int i = 2; i <= 5; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, matrixSize, nullptr, GL_DYNAMIC_DRAW);
    }
    
    // Buffers 6-7: Output buffers
    std::vector<uint32_t> initDist(W * V, 0xCCCCCCCC);
    std::vector<int> initPred(W * V, -999);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[6]);  // OutputDist
    glBufferData(GL_SHADER_STORAGE_BUFFER, matrixSize, initDist.data(), GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[7]);  // OutputPred
    glBufferData(GL_SHADER_STORAGE_BUFFER, W * V * sizeof(int), initPred.data(), GL_DYNAMIC_COPY);

    
    // Buffer 8-9: Debug buffers
    std::vector<uint32_t> debugData(3 * W, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[8]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * W * sizeof(uint32_t), 
                debugData.data(), GL_DYNAMIC_COPY);

    for (int i = 0; i < 9; i++) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, ssbos[i]);
    }

    glUseProgram(computeProgram);

    setUniforms(computeProgram, V, E, W);

    // Dispatch compute shader
    std::cout << "Dispatching " << W << " working groups" << std::endl;
    glDispatchCompute(W, 1, 1); // One work group per keyword
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read debug buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[8]);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 
                      3 * W * sizeof(uint32_t), debugData.data());
    
    std::cout << "\nDebug Output:\n";
    for (int w = 0; w < W; w++) {
        std::cout << "Keyword " << w << ": "
                  << "Init=" << std::hex << debugData[w] << ", "
                  << "EdgeWeight=" << debugData[W + w] << ", "
                  << "FinalDist=" << debugData[2*W + w] << std::dec << "\n";
    }

    // Retrieve results from OutputDist/OutputPred
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[6]);
    uint* distResults = (uint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[7]);
    int* predResults = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    // Copy data to matrix
    for (int w = 0; w < W; ++w) {
        for (int v = 0; v < V; ++v) {
            const int index = w * V + v;
            matrix[w][v].pred = (int)predResults[index];
            matrix[w][v].dist = (int)distResults[index];
        }
    }

    // Unmap buffers
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glDeleteBuffers(10, ssbos);
    glDeleteProgram(computeProgram);
}
