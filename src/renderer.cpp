#include <vector>
#include "renderer.hpp"
#include "shader_util.hpp"

void GPUGraph::createBuffers() {
    glGenBuffers(2, nodeSSBOs);
    glGenBuffers(1, &edgeSSBO);
    updateEdgeBuffer();
}

void GPUGraph::loadShaders() {
        forceSimProgram = createShaderProgram("force_directed.comp");
        nodeShader = createShaderProgram("node.vert", "node.frag");
        edgeShader = createShaderProgram("edge.vert", "edge.frag");
}

void GPUGraph::updateEdgeBuffer() {
        // Convert adjacency list to edge array
        std::vector<GPUEdge<int>> edges;
        for(auto& [from, edge] : graph.adjacency_list) {
            edges.push_back({static_cast<int>(from), 
                           static_cast<int>(edge.end),
                           static_cast<float>(edge.weight)});
        }

        // Upload to GPU
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                    edges.size() * sizeof(Edge<int>),
                    edges.data(), GL_STATIC_DRAW);
}

void GPUGraph::updateNodeBuffers() {
        // Convert vertices to node data
        std::vector<NodeData> nodes;
        for(auto* vert : graph.vertices) {
            nodes.push_back({
                glm::vec2(rand() % 1000 / 1000.0f, rand() % 1000 / 1000.0f), // Random init
                glm::vec2(0.0f)
            });
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

void GPUGraph::render(glm::mat4 projection) {
        // Draw edges
        glUseProgram(edgeShader);
        glUniformMatrix4fv(glGetUniformLocation(edgeShader, "projection"),
                         1, GL_FALSE, &projection[0][0]);
        glUniform4f(glGetUniformLocation(edgeShader, "color"),
                   params.edge_color.x, 
                   params.edge_color.y,
                   params.edge_color.z,
                   params.edge_color.w);

        // Draw lines between connected nodes
        for(auto& [from, edge] : graph.adjacency_list) {
            // Get positions from current buffer
            NodeData start, end;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBOs[currentBuffer]);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, from * sizeof(NodeData),
                              sizeof(NodeData), &start);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, edge.end * sizeof(NodeData),
                              sizeof(NodeData), &end);

            // Draw line
            glm::vec2 points[2] = {start.pos, end.pos};
            drawLines(points, 2);
        }

        // Draw nodes
        glUseProgram(nodeShader);
        glUniformMatrix4fv(glGetUniformLocation(nodeShader, "projection"),
                         1, GL_FALSE, &projection[0][0]);
        glUniform4f(glGetUniformLocation(nodeShader, "color"),
                   params.vertex_color.x,
                   params.vertex_color.y,
                   params.vertex_color.z,
                   params.vertex_color.w);

        // Bind node positions as vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, nodeSSBOs[currentBuffer]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(NodeData), 0);

        // Draw points
        glDrawArrays(GL_POINTS, 0, graph.n_vertices);
}
