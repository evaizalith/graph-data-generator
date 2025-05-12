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

GraphParameters graph_p;

void genGraph() {
    GraphGenerator<int> gen(std::time(nullptr), 5, 5);
    
    if (graph) delete graph;
    layout.reset_positions();

    graph = gen.generate(
            graph_p.n_vertices,
            graph_p.n_keywords,
            graph_p.min_keywords,
            graph_p.max_keywords,
            graph_p.min_degree,
            graph_p.max_degree,
            graph_p.min_weight,
            graph_p.max_weight);
    std::cout << *graph << std::endl;
    layout.initialize_positions(*graph, params);
}

void reshape(int w, int h) {
    params.width = w;
    params.height = h;
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);

    glutPostRedisplay();

    layout.initialize_positions(*graph, params);
}

void showMenu(ImGuiIO& io) {
    
    ImGui::Begin("Graph Generator");                          

    ImGui::TextWrapped("Modify the parameters of the graph and press 'Generate Graph' when you're done.");  

    ImGui::InputInt("Number of Vertices", &graph_p.n_vertices);
    ImGui::InputInt("Min Degree", &graph_p.min_degree);
    ImGui::InputInt("Max Degree", &graph_p.max_degree);
    ImGui::InputInt("Number of Keywords", &graph_p.n_keywords);
    ImGui::InputInt("Min Keywords", &graph_p.min_keywords);
    ImGui::InputInt("Max Keywords", &graph_p.max_keywords);
    ImGui::InputInt("Min Weight", &graph_p.min_weight);
    ImGui::InputInt("Max Weight", &graph_p.max_weight);


    ImGui::ColorEdit3("Vertex Color", (float*)&graph_p.vertex_color);
    ImGui::ColorEdit3("Edge Color", (float*)&graph_p.edge_color);

    if (ImGui::Button("Generate Graph"))                            
        genGraph();
    ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
}

void drawGraph() {
    layout.calculate(*graph, params);
    const std::map<int, std::pair<float, float>>& positions = layout.get_positions();

    // Draw edges
    glColor3f(graph_p.edge_color.x, graph_p.edge_color.y, graph_p.edge_color.z);
    glBegin(GL_TRIANGLES);
    for(const auto& edge : graph->adjacency_list) {
        int src = edge.first;
        int dest = edge.second.end;
        if(positions.count(src) && positions.count(dest)) {
            glVertex2f(positions.at(src).first + 1, positions.at(src).second + 1);
            glVertex2f(positions.at(dest).first - 1, positions.at(dest).second - 1);
            glVertex2f(positions.at(src).first - 3, positions.at(src).second + 3);
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

}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    if (params.width != io.DisplaySize.x || params.height != io.DisplaySize.y)
        reshape(io.DisplaySize.x, io.DisplaySize.y);

    showMenu(io);
    drawGraph();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glutSwapBuffers();
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGLUT_InstallFuncs();
   
    glutMainLoop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

