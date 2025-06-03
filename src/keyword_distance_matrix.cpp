#include "keyword_distance_matrix.hpp"
#include <omp.h>
#include <iostream>

const int BIG_NUMBER = 0x7FFFFFFF;

const int BATCH_SIZE = 100; // keywords to process per batch
const int LOCAL_SIZE = 512; // threads per work group

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
        return;
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
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, edges.size() * sizeof(VerboseEdge<int>), edges.data(), GL_STATIC_DRAW);

    // Process in batches
    for (int batchStart = 0; batchStart < W; batchStart += BATCH_SIZE) {
        const int batchSize = std::min(BATCH_SIZE, W - batchStart);
        std::cout << "Processing batch: " << batchStart << " to " 
                  << (batchStart + batchSize - 1) << std::endl;

        // Buffer 1: HasKeyword (batchSize × V)
        std::vector<uint32_t> hasKeywordData(batchSize * V, 0);
        for (int b = 0; b < batchSize; b++) {
            int w = batchStart + b;
            auto vertices = graph->get_vertices_with_keyword(w);
            for (int v : vertices) {
                hasKeywordData[b * V + v] = 1;
            }
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                    hasKeywordData.size() * sizeof(uint32_t),
                    hasKeywordData.data(), GL_DYNAMIC_DRAW);

        // Buffers 2-7: Batch-sized buffers
        const GLsizeiptr batchMatrixSize = batchSize * V * sizeof(uint32_t);
        std::vector<uint32_t> initDist(batchSize * V, 0x7FFFFFFF);
        std::vector<int> initPred(batchSize * V, -1);
        
        for (int i = 2; i <= 5; i++) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER, batchMatrixSize, 
                        initDist.data(), GL_DYNAMIC_DRAW);
        }
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[6]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, batchMatrixSize, 
                    initDist.data(), GL_DYNAMIC_COPY);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[7]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, batchSize * V * sizeof(int), 
                    initPred.data(), GL_DYNAMIC_COPY);

        // Bind all buffers
        for (int i = 0; i < 8; i++) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, ssbos[i]);
        }

        // Set uniforms
        glUseProgram(computeProgram);
        glUniform1ui(glGetUniformLocation(computeProgram, "V"), V);
        glUniform1ui(glGetUniformLocation(computeProgram, "E"), E);
        glUniform1ui(glGetUniformLocation(computeProgram, "W"), batchSize);

        // Dispatch compute shader
        GLuint workGroupsX = (batchSize + LOCAL_SIZE - 1) / LOCAL_SIZE;
        glDispatchCompute(workGroupsX, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Read results for this batch
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[6]);
        uint32_t* distData = (uint32_t*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[7]);
        int* predData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

        for (int b = 0; b < batchSize; b++) {
            int w = batchStart + b;
            for (int v = 0; v < V; v++) {
                matrix[w][v].dist = distData[b * V + v];
                matrix[w][v].pred = predData[b * V + v];
            }
        }

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        std::cout << "Completed batch: " << batchStart << " to "
                  << (batchStart + batchSize - 1) << std::endl;
    }

    glDeleteBuffers(10, ssbos);
    glDeleteProgram(computeProgram);

}
