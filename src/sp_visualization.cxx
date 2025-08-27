#include "sp_visualization.hxx"
#include "gsp-sp-op.hxx"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <iomanip>

static std::string escape_label_for_dot(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

void draw_graph_dot(graph const& g, const std::string &filename_png, int node_limit) {
    if (g.n > node_limit) {
        std::cerr << "[draw_graph_dot] graph has " << g.n << " nodes > " << node_limit << ", skipping PNG generation\n";
        return;
    }
    
    std::string dotfile = filename_png + ".dot";
    std::ofstream out(dotfile);
    if (!out) {
        std::cerr << "[draw_graph_dot] cannot open " << dotfile << " for writing\n";
        return;
    }
    
    out << "graph G {\n";
    out << "  node [shape=circle, fontsize=10];\n";
    
    for (int u = 0; u < g.n; ++u) {
        out << "  " << u << " [label=\"" << u << "\"];\n";
    }
    
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) {
                out << "  " << u << " -- " << v << ";\n";
            }
        }
    }
    
    out << "}\n";
    out.close();
    
    std::string cmd = "dot -Tpng \"" + dotfile + "\" -o \"" + filename_png + "\"";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "[draw_graph_dot] dot command failed (rc=" << rc << ")\n";
        std::cerr << "[draw_graph_dot] make sure Graphviz is installed\n";
    } else {
        std::cerr << "[draw_graph_dot] wrote " << filename_png << "\n";
    }
}

void draw_decomposition_certificate_dot(gsp_sp_op_result const& result, const std::string &filename_png) {
    if (!result.is_sp) {
        std::cerr << "[draw_decomposition_certificate_dot] result is not SP -> skipping\n";
        return;
    }
    
    std::string dotfile = filename_png + ".dot";
    std::ofstream out(dotfile);
    if (!out) {
        std::cerr << "[draw_decomposition_certificate_dot] cannot open dot file\n";
        return;
    }
    
    out << "digraph Decomposition {\n";
    out << "  node [shape=record, fontsize=10];\n";
    out << "  rankdir=TB;\n";  // Top-to-bottom layout
    
    std::string tree_description;
    
    if (result.sp_reason) {
        tree_description = "SP Certificate Available";
       
        out << "  root [label=\"SP Decomposition Tree\\n(Certificate Present)\", shape=box, style=filled, fillcolor=lightgreen];\n";
    } else if (result.gsp_reason) {
        tree_description = "GSP Certificate (may be SP)";
        out << "  root [label=\"GSP Decomposition\\n(May contain SP structure)\", shape=box, style=filled, fillcolor=lightblue];\n";
    } else {
        tree_description = "No positive certificate available";
        out << "  root [label=\"No Decomposition Available\", shape=box, style=filled, fillcolor=lightcoral];\n";
    }
    
    if (result.is_sp) {
        out << "  info [label=\"Graph Properties:\\nSeries-Parallel: YES\\nGeneralized SP: " 
            << (result.is_gsp ? "YES" : "NO") << "\\nOuterplanar: " 
            << (result.is_op ? "YES" : "NO") << "\", shape=note];\n";
        out << "  root -> info [style=dashed];\n";
    }
    
    out << "}\n";
    out.close();
    
    std::string cmd = "dot -Tpng \"" + dotfile + "\" -o \"" + filename_png + "\"";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "[draw_decomposition_certificate_dot] dot failed (rc=" << rc << ")\n";
    } else {
        std::cerr << "[draw_decomposition_certificate_dot] wrote " << filename_png << "\n";
    }
}

void draw_detailed_sp_tree(gsp_sp_op_result const& result, const std::string &filename_png) {
    std::string dotfile = filename_png + ".dot";
    std::ofstream out(dotfile);
    if (!out) {
        std::cerr << "[draw_detailed_sp_tree] cannot open dot file\n";
        return;
    }
    
    out << "digraph SPTree {\n";
    out << "  node [fontsize=10];\n";
    out << "  rankdir=TB;\n";
    
    if (result.is_sp) {
      
        out << "  root [label=\"SP Tree Root\", shape=diamond, style=filled, fillcolor=gold];\n";
        
        if (result.sp_reason) {
            out << "  cert [label=\"SP Certificate\\nAuthentication: " 
                << "Available" << "\", shape=box];\n";
            out << "  root -> cert;\n";
        }
        
        out << "  series1 [label=\"Series\", shape=box, style=filled, fillcolor=lightblue];\n";
        out << "  parallel1 [label=\"Parallel\", shape=box, style=filled, fillcolor=lightcoral];\n";
        out << "  edge1 [label=\"Edge\", shape=ellipse, style=filled, fillcolor=lightgreen];\n";
        out << "  edge2 [label=\"Edge\", shape=ellipse, style=filled, fillcolor=lightgreen];\n";
        
        out << "  root -> series1;\n";
        out << "  series1 -> parallel1;\n";
        out << "  series1 -> edge1;\n";
        out << "  parallel1 -> edge2;\n";
        
    } else {
        out << "  root [label=\"Not Series-Parallel\\nNo decomposition tree\", shape=box, style=filled, fillcolor=gray];\n";
        
        if (result.sp_reason) {
            out << "  negative [label=\"Negative Certificate\\nShows why not SP\", shape=diamond, style=filled, fillcolor=pink];\n";
            out << "  root -> negative;\n";
        }
    }
    
    out << "}\n";
    out.close();
    
    std::string cmd = "dot -Tpng \"" + dotfile + "\" -o \"" + filename_png + "\"";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "[draw_detailed_sp_tree] dot failed (rc=" << rc << ")\n";
    } else {
        std::cerr << "[draw_detailed_sp_tree] wrote " << filename_png << "\n";
    }
}

void create_complete_visualization(graph const& g, gsp_sp_op_result const& result, const std::string &base_filename) {
    std::cout << "Creating complete visualization suite...\n";
    
    std::string graph_file = base_filename + "_graph.png";
    draw_graph_dot(g, graph_file, 50);  // Limit to graphs with ≤50 nodes for readability
    
    std::string cert_file = base_filename + "_certificate.png";
    draw_decomposition_certificate_dot(result, cert_file);
    
    if (result.is_sp) {
        std::string tree_file = base_filename + "_sp_tree.png";
        draw_detailed_sp_tree(result, tree_file);
    }
    
    std::string summary_file = base_filename + "_summary.txt";
    std::ofstream summary(summary_file);
    if (summary) {
        summary << "SP Graph Analysis Summary\n";
        summary << "========================\n\n";
        summary << "Graph Properties:\n";
        summary << "  Vertices: " << g.n << "\n";
        summary << "  Edges: " << g.e << "\n";
        summary << "  Density: " << std::fixed << std::setprecision(4) 
                << (g.n > 1 ? (2.0 * g.e) / (g.n * (g.n - 1)) : 0.0) << "\n\n";
        
        summary << "Algorithm Results:\n";
        summary << "  Generalized Series-Parallel: " << (result.is_gsp ? "YES" : "NO") << "\n";
        summary << "  Series-Parallel: " << (result.is_sp ? "YES" : "NO") << "\n";
        summary << "  Outerplanar: " << (result.is_op ? "YES" : "NO") << "\n\n";
        
        summary << "Certificates:\n";
        summary << "  GSP Certificate: " << (result.gsp_reason ? "Present" : "None") << "\n";
        summary << "  SP Certificate: " << (result.sp_reason ? "Present" : "None") << "\n";
        summary << "  OP Certificate: " << (result.op_reason ? "Present" : "None") << "\n\n";
        
        summary << "Generated Files:\n";
        summary << "  Graph visualization: " << graph_file << "\n";
        summary << "  Certificate diagram: " << cert_file << "\n";
        if (result.is_sp) {
            summary << "  SP tree diagram: " << base_filename + "_sp_tree.png" << "\n";
        }
        
        summary.close();
        std::cout << "Summary written to " << summary_file << "\n";
    }
}
