#include <GL/glut.h>
#include "graph.hpp"
#include "graph_generator.hpp"
#include "force_directed_layout.hpp"

SparseGraph<int>* graph;
ForceDirectedParams params;
ForceDirectedLayout<int> layout;

void genGraph() {
    GraphGenerator<int> gen(1, 5, 5);
    graph = gen.generate(30, 10, 1, 5, 1, 10);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Update layout
    layout.calculate(*graph, params);
    const auto& positions = layout.get_positions();

    // Draw edges
    glColor3f(0.4, 1.0, 0.4);
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
    glColor3f(1.0, 1.0, 1.0);
    glPointSize(6.0);
    glBegin(GL_POINTS);
    for(const auto& pos_entry : positions) {
        const auto& pos = pos_entry.second;
        glVertex2f(pos.first, pos.second);
    }
    glEnd();

    glutSwapBuffers();
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 100, 0, 100);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    genGraph();
    layout.initialize_positions(*graph, params);

    glutInit(&argc, argv);
    glutInitWindowSize(params.width, params.height);
    glutCreateWindow("Graph Data Generator");
    glOrtho(0, params.width, params.height, 0, -1, 1);
   
    std::cout << *graph << std::endl;
    
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}

