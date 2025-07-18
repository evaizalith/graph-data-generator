#include "keyword_distance_matrix.hpp"
#include <omp.h>
#include <iostream>
#include "percent_tracker.hpp"

const int BIG_NUMBER = 0x7FFFFFFF;

const int BATCH_SIZE = 50; // keywords to process per batch
const int LOCAL_SIZE = 1024; // threads per work group

KeywordDistanceMatrix::KeywordDistanceMatrix(int n_W, int n_V, int max_weight) {
    W = n_W;
    V = n_V;
    MAX_WEIGHT = max_weight + 1;
    matrix = new std::atomic<Pair>*[W];
    for (int i = 0; i < W; i++) {
        matrix[i] = new std::atomic<Pair>[V];
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
    return (matrix[w][v]).load();
}

Pair KeywordDistanceMatrix::get_size() const {
    Pair p = {W, V};
    return p;
}

void KeywordDistanceMatrix::calculate_matrix_cpu(SparseGraph<int>* graph) {
    std::vector<VerboseEdge<int>> list = graph->get_edge_list();
    if (list.size() == 0) {
        std::cerr << "No edges found for " << __func__ << std::endl;
        return;
    }

    ProgressTracker tracker("calculate_matrix_cpu", "All keywords processed.", W);
    tracker.begin();


    #pragma omp parallel num_threads(10) 
    {
       #pragma omp for 
        for (int w = 0; w < W; w++) {
            unsigned* dist = new unsigned[V];
            int* pred = new int[V];

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
                (matrix[w][v]).store({(int)dist[v], pred[v]});
            }
            
            tracker.increment_and_print();
            delete[] dist;
            delete[] pred;
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
    GLuint ssbos[8]; // EdgeList, HasKeyword, Dist0, Dist1, Pred0, Pred1, OutputDist, OutputPred
    glGenBuffers(8, ssbos);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, edges.size() * sizeof(VerboseEdge<int>), edges.data(), GL_STATIC_DRAW);

    dynamicBatchSize = (V > dynamicBatchSizeCutoff) ? minBatchSize : BATCH_SIZE;

    ProgressTracker tracker("calculate_matrix_gpu", "All keywords processed.", W / dynamicBatchSize);
    tracker.begin();

    // Process in batches
    for (int batchStart = 0; batchStart < W; batchStart += dynamicBatchSize) {
        const int batchSize = std::min(dynamicBatchSize, W - batchStart);

        // Buffer 1: HasKeyword (batchSize × V)
        std::vector<uint32_t> hasKeywordData(batchSize * V, 0);
        for (int b = 0; b < batchSize; b++) {
            int w = batchStart + b;
            auto vertices = graph->get_vertices_with_keyword(w);
            for (int v : vertices) {
                if (v < V)
                    hasKeywordData[b * V + v] = 1;
            }
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                    hasKeywordData.size() * sizeof(uint32_t),
                    hasKeywordData.data(), GL_DYNAMIC_DRAW);

        const GLsizeiptr batchMatrixSize = batchSize * V * sizeof(uint32_t);
        std::vector<uint32_t> initDist(batchSize * V, 0x7FFFFFFF);
        std::vector<int> initPred(batchSize * V, -1);
        
        // Output pred buffers
        for (int i = 2; i <= 3; i++) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER, batchMatrixSize, 
                        initDist.data(), GL_DYNAMIC_DRAW);
        }

        // Input pred buffers
        for (int i = 4; i <= 5; i++) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER, batchMatrixSize, 
                        initPred.data(), GL_DYNAMIC_DRAW);
        }
        
        // Output dist buffers
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[6]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, batchMatrixSize, 
                    initDist.data(), GL_DYNAMIC_COPY);
        
        // Output pred buffers
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
        GLuint workGroupsX = (batchSize * V + LOCAL_SIZE - 1) / LOCAL_SIZE;
        glDispatchCompute(workGroupsX, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glFinish();

        // Read results for this batch
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[6]);
        uint32_t* distData = (uint32_t*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
       
        if (!distData) {
            std::cerr << "ERROR: Failed to map distance buffer in " << __func__ << std::endl;
            glDeleteBuffers(8, ssbos);
            glDeleteProgram(computeProgram);
            return;
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[7]);
        int* predData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

        if (!predData) {
            std::cerr << "ERROR: Failed to map pred buffer in " << __func__ << std::endl;
            glDeleteBuffers(8, ssbos);
            glDeleteProgram(computeProgram);
            return;
        }

        for (int b = 0; b < batchSize; b++) {
            int w = batchStart + b;
            for (int v = 0; v < V; v++) {
                (matrix[w][v]).store({(int)distData[b * V + v], predData[b * V + v]});
            }
        }

        for (int i = 6; i < 7; i++) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[i]);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        
        tracker.increment_and_print();
    }

    glDeleteBuffers(8, ssbos);
    glDeleteProgram(computeProgram);

}
