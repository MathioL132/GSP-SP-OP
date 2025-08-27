#ifndef __SP_VISUALIZATION_HXX__
#define __SP_VISUALIZATION_HXX__

#include "graph.hxx"
#include "sp-tree.hxx"
#include "gsp-sp-op.hxx"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

inline void write_graph_dot(graph const& g, std::string const& filename) {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return;

    ofs << "graph G {\n";
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) ofs << "    " << u << " -- " << v << ";\n";
        }
    }
    ofs << "}\n";
    ofs.close();

    std::cout << "Graph DOT written to " << filename << "\n";
}

inline void write_sp_tree_dot(sp_tree_node* node, std::ofstream& ofs, int& counter) {
    if (!node) return;

    int my_id = counter++;
    std::string label;
    switch(node->comp) {
        case c_type::edge: label = std::to_string(node->source) + "--" + std::to_string(node->sink); break;
        case c_type::series: label = "Series"; break;
        case c_type::parallel: label = "Parallel"; break;
        case c_type::antiparallel: label = "Antiparallel"; break;
        case c_type::dangling: label = "Dangling"; break;
    }
    ofs << "    node" << my_id << " [label=\"" << label << "\", shape=box];\n";

    if (node->l) {
        int left_id = counter;
        write_sp_tree_dot(node->l, ofs, counter);
        ofs << "    node" << my_id << " -> node" << left_id << ";\n";
    }
    if (node->r) {
        int right_id = counter;
        write_sp_tree_dot(node->r, ofs, counter);
        ofs << "    node" << my_id << " -> node" << right_id << ";\n";
    }
}

inline void write_decomposition_dot(sp_tree const& T, std::string const& filename) {
    if (!T.root) return;
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return;

    ofs << "digraph SPTree {\n";
    int counter = 0;
    write_sp_tree_dot(T.root, ofs, counter);
    ofs << "}\n";
    ofs.close();
    std::cout << "SP decomposition DOT written to " << filename << "\n";
}

inline void draw_sp_graph(graph const& g, std::string const& filename) {
    write_graph_dot(g, filename);
}

inline void draw_decomposition_tree(gsp_sp_op_result const& result, std::string const& filename) {
    write_decomposition_dot(result.decomposition_tree(), filename);
}

#endif
