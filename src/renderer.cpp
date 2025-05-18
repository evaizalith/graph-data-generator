#include <vector>
#include "renderer.hpp"
#include "shader_util.hpp"

void GPUGraph::createBuffers() {
    glGenBuffers(2, nodeSSBOs);
    glGenBuffers(1, &edgeSSBO);
    updateEdgeBuffer();
    updateNodeBuffers();
}

void GPUGraph::loadShaders() {
        forceSimProgram = createShaderProgram("force_directed.comp");
        nodeShader = createShaderProgram("node.vert", "node.frag");
        edgeShader = createShaderProgram("edge.vert", "edge.frag");
}

void GPUGraph::updateEdgeBuffer() {
        // Convert adjacency list to edge array
        std::vector<GPUEdge> edges;
        for(auto& [from, edge] : graph.adjacency_list) {
            edges.push_back({static_cast<int>(from), 
                           static_cast<int>(edge.end),
                           static_cast<float>(edge.weight)});
        }

        // Upload to GPU
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                    edges.size() * sizeof(Edge<int>),
                    edges.data(), GL_DYNAMIC_DRAW);
}

void GPUGraph::updateNodeBuffers() {
        if (graph.vertices.empty()) return;

        // Convert vertices to node data
        std::vector<NodeData> nodes;
        for(auto* vert : graph.vertices) {
            nodes.push_back({
                glm::vec2(rand() % screenWidth, rand() % screenHeight), // Random init
                glm::vec2(0.0f)
            });
        }

        if (nodes.empty()) {
            std::cerr << "Graph is empty. No rendering can be done. Discovered in: " << __func__ << std::endl;
            return; 
        }

        // Initialize both SSBOs
        for(int i = 0; i < 2; i++) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBOs[i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER, 
                        nodes.size() * sizeof(NodeData),
                        nodes.data(), GL_DYNAMIC_DRAW);
        }
}

void GPUGraph::simulate(float dt) {
        glUseProgram(forceSimProgram);
        
        // Bind buffers
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, nodeSSBOs[currentBuffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nodeSSBOs[1 - currentBuffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, edgeSSBO);

        // Set uniforms
        glUniform1f(glGetUniformLocation(forceSimProgram, "deltaTime"), dt);
        
        // Dispatch compute
        glDispatchCompute(graph.n_vertices, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Swap buffers
        currentBuffer = 1 - currentBuffer;
}

void drawLines(const glm::vec2* points, size_t count) {
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::vec2), points, GL_STREAM_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glDrawArrays(GL_LINES, 0, count);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void drawPoints(const glm::vec2* points, size_t count) {
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::vec2), points, GL_STREAM_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glDrawArrays(GL_POINTS, 0, count);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void GPUGraph::render(glm::mat4 projection) {
        // Validate buffer sizes
        GLint bufSize;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBOs[currentBuffer]);
        glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &bufSize);
        
        const size_t requiredSize = graph.vertices.size() * sizeof(NodeData);
        if (bufSize < requiredSize) {
            std::cerr << "Buffer size mismatch! "
                      << "Required: " << requiredSize 
                      << ", Actual: " << bufSize << "\n";
            return;
        }

        std::vector<NodeData> currentNodes(graph.vertices.size());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBOs[currentBuffer]);
        glGetBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            0,  // Correct offset
            graph.vertices.size() * sizeof(NodeData),
            currentNodes.data()
        );

        // Batch edge points
        std::vector<glm::vec2> edgePoints;
        for (auto& [from, edge] : graph.adjacency_list) {
            if (from >= currentNodes.size() || edge.end >= currentNodes.size()) {
                continue;  // Skip invalid edges
            }
            edgePoints.push_back(currentNodes[from].pos);
            edgePoints.push_back(currentNodes[edge.end].pos);
        }

        glUseProgram(nodeShader);
        glUniformMatrix4fv(glGetUniformLocation(nodeShader, "projection"),
                         1, GL_FALSE, &projection[0][0]);
        glUniform4f(glGetUniformLocation(nodeShader, "color"),
                   params.vertex_color.x,
                   params.vertex_color.y,
                   params.vertex_color.z,
                   params.vertex_color.w);

        // Draw all vertices
        if (!edgePoints.empty()) {
            drawPoints(edgePoints.data(), edgePoints.size());
        }


        glUseProgram(edgeShader);
        glUniformMatrix4fv(glGetUniformLocation(edgeShader, "projection"),
                         1, GL_FALSE, &projection[0][0]);
        glUniform4f(glGetUniformLocation(edgeShader, "color"),
                   params.edge_color.x, 
                   params.edge_color.y,
                   params.edge_color.z,
                   params.edge_color.w);



        // Draw all edges
        if (!edgePoints.empty()) {
            drawLines(edgePoints.data(), edgePoints.size());
        }
}
