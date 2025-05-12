#ifndef FORCE_DIRECTED_LAYOUT_HPP
#define FORCE_DIRECTED_LAYOUT_HPP

#include "graph.hpp"
#include <map>
#include <cmath>
#include <ctime>
#include <vector>
#include <algorithm>

struct ForceDirectedParams {
    float width = 1000.0f;
    float height = 800.0f;
    float k_repulsion = 2000.0f;
    float k_attraction = 0.2f;
    float ideal_length = 200.0f;
    float time_step = 0.01f;
    int max_iterations = 100;
};

template<typename T>
class ForceDirectedLayout {
public:
    void calculate(SparseGraph<T>& graph, const ForceDirectedParams& params);
    const std::map<T, std::pair<float, float>>& get_positions() const;
    void initialize_positions(SparseGraph<T>& graph, const ForceDirectedParams& params);
    void shift_to_middle(SparseGraph<T> graph, const ForceDirectedParams& params);
    void reset_positions();

private:
    std::map<T, std::pair<float, float>> positions;
    std::vector<T> get_all_vertex_ids(SparseGraph<T>& graph);
    size_t counter = 0; 

};

template <typename T>
void ForceDirectedLayout<T>::calculate(SparseGraph<T>& graph, const ForceDirectedParams& params) {
    counter = 0;
    for(int iter = 0; iter < params.max_iterations; ++iter) {
        std::map<T, std::pair<float, float>> forces;

        std::srand(std::time(nullptr) * ++counter);
       
        auto vertex_ids = get_all_vertex_ids(graph);

        // Repulsive forces between all vertices
        
        // As a temporary measure to make software rendering faster, we apply repulsion stochastically
        size_t i = (static_cast<size_t>(std::rand()) * RAND_MAX) % vertex_ids.size();
        //size_t i = counter++ % vertex_ids.size();
        for(size_t j = 0; j < vertex_ids.size(); ++j) {
            T id1 = vertex_ids[i];
            T id2 = vertex_ids[j];
            auto& pos1 = positions[id1];
            auto& pos2 = positions[id2];
            
            float dx = pos2.first - pos1.first;
            float dy = pos2.second - pos1.second;
            float distance = std::hypot(dx, dy) + 0.01f;
            float force = params.k_repulsion / (distance * distance);
            
            forces[id1].first -= (dx / distance) * force;
            forces[id1].second -= (dy / distance) * force;
            forces[id2].first += (dx / distance) * force;
            forces[id2].second += (dy / distance) * force;
        }

        // Attractive forces between connected vertices
        for(const auto& edge : graph.adjacency_list) {
            T src = edge.first;
            T dest = edge.second.end;
            
            auto& pos_src = positions[src];
            auto& pos_dest = positions[dest];
            float dx = pos_dest.first - pos_src.first;
            float dy = pos_dest.second - pos_src.second;
            float distance = std::hypot(dx, dy) + 0.01f;
            float force = (distance - params.ideal_length) * params.k_attraction * edge.second.weight;
            
            forces[src].first += (dx / distance) * force;
            forces[src].second += (dy / distance) * force;
            forces[dest].first -= (dx / distance) * force;
            forces[dest].second -= (dy / distance) * force;
        }

        // Update positions with bounds check
        for(typename std::map<T, std::pair<float, float>>::iterator it = forces.begin(); 
            it != forces.end(); ++it) {
            T id = it->first;
            float fx = it->second.first;
            float fy = it->second.second;
            
            positions[id].first += fx * params.time_step;
            positions[id].second += fy * params.time_step;
            
            // Manual clamping
            positions[id].first = std::max(0.0f, std::min(params.width, positions[id].first));
            positions[id].second = std::max(0.0f, std::min(params.height, positions[id].second));
        }
    }
}

template <typename T>
const std::map<T, std::pair<float, float>>& ForceDirectedLayout<T>::get_positions() const {
    return positions;
}

template <typename T>
void ForceDirectedLayout<T>::reset_positions() {
    positions.clear();
}

template <typename T>
void ForceDirectedLayout<T>::initialize_positions(SparseGraph<T>& graph, const ForceDirectedParams& params) {
        std::srand(std::time(nullptr));
        for(size_t i = 0; i < graph.vertices.size(); ++i) {
            if(graph.vertex_exists(static_cast<T>(i))) {
                positions[static_cast<T>(i)] = {
                    static_cast<float>(std::rand()) / RAND_MAX * params.width,
                    static_cast<float>(std::rand()) / RAND_MAX * params.height
                };
            }
        }
}

template <typename T>
void ForceDirectedLayout<T>::shift_to_middle(SparseGraph<T> graph, const ForceDirectedParams& params) {
    if (positions.empty()) return;

    for (size_t i = 0; i < graph.vertices.size(); ++i) {
        positions[static_cast<T>(i)] = {
            (params.width - positions[i].first) + (params.width / 2),
            (params.width - positions[i].second) + (params.height / 2)
        };
    }
}

template <typename T>
std::vector<T> ForceDirectedLayout<T>::get_all_vertex_ids(SparseGraph<T>& graph) {
    std::vector<T> ids;
    for(size_t i = 0; i < graph.vertices.size(); ++i) {
        if(graph.vertex_exists(static_cast<T>(i))) {
            ids.push_back(static_cast<T>(i));
        }
    }
    return ids;
}

#endif
