#ifndef EVA_GRAPH_RENDERER
#define EVA_GRAPH_RENDERER

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "graph.hpp"

struct NodeData {
    glm::vec2 pos;
    glm::vec2 velocity;
};

struct GPUEdge {
    int from;
    int end;
    float weight;
};

class GPUGraph {
public:
    GPUGraph(SparseGraph<int>& g, GraphParameters& p) : graph(g), params(p) { currentBuffer = 0; createBuffers(); try {loadShaders(); } catch (std::exception& e) {throw(e);} };

    void createBuffers();
    void loadShaders();
    void updateEdgeBuffer();
    void updateNodeBuffers();
    void simulate(float dt);
    void render(glm::mat4 projection);

    int screenWidth = 1000;
    int screenHeight = 800;

private:
    GLuint nodeSSBOs[2];
    GLuint edgeSSBO;
    GLuint forceSimProgram;
    GLuint nodeShader;
    GLuint edgeShader;
    GLuint currentBuffer;

    SparseGraph<int>& graph;
    GraphParameters& params;
};

#endif
