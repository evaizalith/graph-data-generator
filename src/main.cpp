#include <GL/glut.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glut.h>
#include "graph.hpp"
#include "graph_generator.hpp"
#include "force_directed_layout.hpp"

SparseGraph<int>* graph;
ForceDirectedParams params;
ForceDirectedLayout<int> layout;

struct graph_p {
    int n_vertices = 30;
    int n_keywords = 10;
    int min_degree = 1;
    int max_degree = 5;
    int min_keywords = 1;
    int max_keywords = 5;
    ImVec4 vertex_color = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 edge_color = ImVec4(0.0f, 1.0f, 0.0f, 1.00f);
} graph_p;

void genGraph() {
    GraphGenerator<int> gen(std::time(nullptr), 5, 5);
    graph = gen.generate(
            graph_p.n_vertices,
            graph_p.n_keywords,
            graph_p.min_keywords,
            graph_p.max_keywords,
            graph_p.min_degree,
            graph_p.max_degree);
    std::cout << *graph << std::endl;
    layout.initialize_positions(*graph, params);
}

void showMenu(ImGuiIO& io) {
    
    ImGui::Begin("Graph Generator");                          

    ImGui::TextWrapped("Modify the parameters of the graph and press 'Generate Graph' when you're done.");  
    //ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::InputInt("Number of Vertices", &graph_p.n_vertices);
    ImGui::InputInt("Min Degree", &graph_p.min_degree);
    ImGui::InputInt("Max Degree", &graph_p.max_degree);
    ImGui::InputInt("Number of Keywords", &graph_p.n_keywords);
    ImGui::InputInt("Min Keywords", &graph_p.min_keywords);
    ImGui::InputInt("Max Keywords", &graph_p.max_keywords);

    ImGui::ColorEdit3("Vertex Color", (float*)&graph_p.vertex_color);
    ImGui::ColorEdit3("Edge Color", (float*)&graph_p.edge_color);

    if (ImGui::Button("Generate Graph"))                            
        genGraph();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
}

void drawGraph() {
    layout.calculate(*graph, params);
    const auto& positions = layout.get_positions();

    // Draw edges
    glColor3f(graph_p.edge_color.x, graph_p.edge_color.y, graph_p.edge_color.z);
    glBegin(GL_LINES);
    for(const auto& edge : graph->adjacency_list) {
        int src = edge.first;
        int dest = edge.second;
        if(positions.count(src) && positions.count(dest)) {
            glVertex2f(positions.at(src).first, positions.at(src).second);
            glVertex2f(positions.at(dest).first, positions.at(dest).second);
        }
    }
    glEnd();

    // Draw vertices
    glColor3f(graph_p.vertex_color.x, graph_p.vertex_color.y, graph_p.vertex_color.z);
    glPointSize(6.0);
    glBegin(GL_POINTS);
    for(const auto& pos_entry : positions) {
        const auto& pos = pos_entry.second;
        glVertex2f(pos.first, pos.second);
    }
    glEnd();

    delete positions;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    showMenu(io);
    drawGraph();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glutSwapBuffers();
    glutPostRedisplay();

}

void reshape(int w, int h) {
    std::cout << params.width << ", " << params.height << std::endl;
    params.width = w;
    params.height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    layout.shift_to_middle(*graph, params);
    //layout.initialize_positions(*graph, params);

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    genGraph();

    glutInit(&argc, argv);
    glutInitWindowSize(params.width, params.height);
    glutCreateWindow("Graph Data Generator");
    glOrtho(0, params.width, params.height, 0, -1, 1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    // Initialize ImGui
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //ImGui::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGLUT_InstallFuncs();
   
    glutMainLoop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

