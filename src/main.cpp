#include <GL/glut.h>
#include "graph.hpp"
#include "graph_generator.hpp"

SparseGraph<int>* graph;

void genGraph() {
    GraphGenerator<int> gen(5, 5, 3);
    graph = gen.generate(10, 10, 1, 5, 0, 3);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw edges in green
    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_LINES);
    for (int i = 0; i < graph->n_vertices; ++i) {
        const auto& adj = graph->get_adjacent(i);
        for (int j : adj) {
            Vertex<int>* v1 = graph->vertices[i];
            Vertex<int>* v2 = graph->vertices[j];
            glVertex2f((v1->id >> 2) * v1->id, (v1->id >> 2) * 13);
            glVertex2f((v2->id >> 2) * v2->id, (v2->id >> 2) * 13);
        }
    }
    glEnd();

    // Draw vertices as white points
    glColor3f(1.0, 1.0, 1.0);
    glPointSize(8.0);
    glBegin(GL_POINTS);
    for (const auto& v : graph->vertices) {
        glVertex2f((v->id >> 2) * v->id, (v->id >> 2) * 13);
    }
    glEnd();

    glutSwapBuffers();
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
    std::cout << *graph << std::endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Graph Display");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutMainLoop();

    return 0;
}
