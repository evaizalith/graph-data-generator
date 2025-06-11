#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstdlib>
#include "graph.hpp"
#include "graph_generator.hpp"
#include "force_directed_layout.hpp"
#include "renderer.hpp"
#include "keyword_distance_matrix.hpp"
#include "csv_writer.hpp"

#define GLEW_STATIC

GLFWwindow* window;

SparseGraph<int>* graph;
ForceDirectedParams params;
GPUGraph* gpuGraph;

GraphParameters graph_p;

int original_w = params.width;
int original_h = params.height;
float d_w;
float d_h;

struct View_t {
    float x = -200;
    float y = -100;
    float zoom = 1.0f;
} view;

int MOVE_SENSITIVITY = 10; // Determines camera panning speed
float ZOOM_SENSITIVITY = 0.1f;
float simSpeed = 0.016f;
bool renderGraph = true;
bool gpuComputation = true;

void resetView() {
    view.x = -params.width/4;
    view.y = -params.height/4;
    view.zoom = 1.0;
}

void keyDistMatrix() {
    KeywordDistanceMatrix mat(graph_p.n_keywords, graph_p.n_vertices, graph_p.max_weight); 
    
    if (gpuComputation) mat.calculate_matrix_hybrid(graph);
    else mat.calculate_matrix_cpu(graph);

    std::cout << "Finished calculating keyword-distance matrix" << std::endl;

    CSVWriter writer;
    writer.write("keyword_distance_matrix.csv", mat);
}

void genGraph() {
    GraphGenerator<int> gen(std::time(nullptr), 5, 5);
    
    if (graph) delete graph;
    if (gpuGraph) delete gpuGraph;
    //layout.reset_positions();

    graph = gen.generate(
            graph_p.n_vertices,
            graph_p.n_keywords,
            graph_p.min_keywords,
            graph_p.max_keywords,
            graph_p.min_degree,
            graph_p.max_degree,
            graph_p.min_weight,
            graph_p.max_weight);
     try {
         gpuGraph = new GPUGraph(*graph, graph_p);
    } catch(std::exception& e) {
        std::cout << "Exception " << e.what() << ", unable to display graph.\n";
        gpuGraph = nullptr;
    }
}

void reshape(int w, int h) {
    params.width = w;
    params.height = h;

    d_w = original_w - (w * 0.5); 
    d_h = original_h - (h * 0.5);

    int fourth_w = (int)(w / 4);
    int fourth_h = (int)(h / 4);

    view.x = -fourth_w;
    view.y = -fourth_h;

    glViewport(0, 0, w, h);
    glfwSetWindowSize(window, w, h);

    if (gpuGraph) {
        gpuGraph->screenWidth = w;
        gpuGraph->screenHeight = h;
    }
}

void keyboard_input(GLFWwindow *win, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_W && action == GLFW_REPEAT) {
        view.y += MOVE_SENSITIVITY;
    }
    if (key == GLFW_KEY_S && action == GLFW_REPEAT) {
        view.y -= MOVE_SENSITIVITY;
    }
    if (key == GLFW_KEY_A && action == GLFW_REPEAT) {
        view.x -= MOVE_SENSITIVITY;
    }
    if (key == GLFW_KEY_D && action == GLFW_REPEAT) {
        view.x += MOVE_SENSITIVITY;
    }
    if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS) {
        view.zoom += ZOOM_SENSITIVITY;
    }
    if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS) {
        view.zoom -= ZOOM_SENSITIVITY;
    }

    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        genGraph();
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        resetView();
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        std::cout << *graph << std::endl;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        keyDistMatrix(); 
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        exit(0);
    }
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
    ImGui::InputFloat("Simulation Speed", &simSpeed);

    if (ImGui::Button("Generate Graph (G)")) genGraph();
    if (ImGui::Button("Print Graph (P)")) std::cout << *graph << std::endl;
    if (ImGui::Button("Calculate Keyword-Distance Matrix (M)")) keyDistMatrix(); 
    if (ImGui::Button("Reset View (R)")) resetView();

    ImGui::Checkbox("Render Graph", &renderGraph);
    ImGui::Checkbox("Use GPU to compute keyword-distance matrices", &gpuComputation);

    ImGui::TextWrapped("Use WASD to pan view, Page Up/Down to zoom");

    ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    if (ImGui::Button("Quit (Esc)"))
        exit(0);

    ImGui::End();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
   
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    if (params.width != io.DisplaySize.x || params.height != io.DisplaySize.y)
        reshape(io.DisplaySize.x, io.DisplaySize.y);

    showMenu(io);

    if (gpuGraph && renderGraph) {
        try {
        gpuGraph->simulate(simSpeed);
        glm::mat4 proj = glm::ortho((d_w + view.x) / view.zoom, ((float)params.width + view.x) / view.zoom, (d_h + view.y) / view.zoom, ((float)params.height + view.y) / view.zoom);

        gpuGraph->render(proj);
        } catch (std::exception& e) {
            std::cout << "Unable to correctly display or simulate; disabling both\n" << std::endl;
            renderGraph = false;
        }
    }

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}



void glDebugOutput(GLenum source, 
                            GLenum type, 
                            unsigned int id, 
                            GLenum severity, 
                            GLsizei length, 
                            const char *message, 
                            const void *userParam)
{
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

    std::cout << "OpenGL Debug message (" << id << "): " <<  message << " | ";

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << " | ";

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break; 
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << " | ";
    
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
}

int main(int argc, char** argv) {
    glewExperimental = GL_TRUE;
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    window = glfwCreateWindow(params.width, params.height, "Graph Generator", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();
    if (!window)
    {
        std::cerr << "Unable to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, keyboard_input);

    glPointSize(6);

    int flags; 
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_FALSE);

        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);

    }

    genGraph();

    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
 
    while (!glfwWindowShouldClose(window)) {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
        glFinish();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

