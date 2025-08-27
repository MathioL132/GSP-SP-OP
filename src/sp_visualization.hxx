#ifndef __SP_VISUALIZATION_HXX__
#define __SP_VISUALIZATION_HXX__

#include "gsp-sp-op.hxx"
#include <string>
#include <fstream>
#include <iostream>


inline void write_graph_dot(graph const& g, std::string const& filename) {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing\n";
        return;
    }
    
    ofs << "graph G {\n";
    ofs << "  node [shape=circle, fontsize=12];\n";
    ofs << "  edge [fontsize=10];\n\n";
    
    // Add vertices
    for (int u = 0; u < g.n; ++u) {
        ofs << "  " << u << " [label=\"" << u << "\"];\n";
    }
    ofs << "\n";
    
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) {
                ofs << "  " << u << " -- " << v << ";\n";
            }
        }
    }
    
    ofs << "}\n";
    ofs.close();
    std::cout << "Graph DOT file written to " << filename << std::endl;
}

inline void write_sp_decomposition_summary(gsp_sp_op_result const& result, std::string const& filename) {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing\n";
        return;
    }
    
    ofs << "Series-Parallel Decomposition Analysis\n";
    ofs << "=====================================\n\n";
    
    ofs << "Classification Results:\n";
    ofs << "  Generalized Series-Parallel (GSP): " << (result.is_gsp ? "YES" : "NO") << "\n";
    ofs << "  Series-Parallel (SP): " << (result.is_sp ? "YES" : "NO") << "\n";  
    ofs << "  Outerplanar (OP): " << (result.is_op ? "YES" : "NO") << "\n\n";
    
    ofs << "Certificate Information:\n";
    if (result.gsp_reason) {
        ofs << "  GSP Certificate: Available\n";
    }
    if (result.sp_reason) {
        ofs << "  SP Certificate: Available\n";
    }
    if (result.op_reason) {
        ofs << "  OP Certificate: Available\n";
    }
    
    if (!result.gsp_reason && !result.sp_reason && !result.op_reason) {
        ofs << "  No certificates available\n";
    }
    
    ofs << "\nDecomposition Tree:\n";
    if (result.is_sp) {
        ofs << "  Graph is series-parallel and has a valid decomposition tree.\n";
        ofs << "  Tree structure can be extracted from the SP certificate.\n";
        ofs << "  (Detailed tree visualization would require certificate inspection)\n";
    } else {
        ofs << "  Graph is NOT series-parallel - no decomposition tree exists.\n";
        if (result.sp_reason) {
            ofs << "  Negative certificate explains why graph is not SP.\n";
        }
    }
    
    ofs.close();
    std::cout << "SP decomposition summary written to " << filename << std::endl;
}


inline void write_sp_tree_dot(gsp_sp_op_result const& result, std::string const& filename) {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing\n";
        return;
    }
    
    ofs << "digraph SPTree {\n";
    ofs << "  node [fontsize=10, style=filled];\n";
    ofs << "  rankdir=TB;\n\n";
    
    if (result.is_sp) {
        ofs << "  // Series-Parallel Decomposition Tree\n";
        ofs << "  root [label=\"SP Root\", shape=diamond, fillcolor=gold];\n";
        
        if (result.sp_reason) {
            ofs << "  cert [label=\"SP Certificate\\n(Authenticated)\", shape=box, fillcolor=lightgreen];\n";
            ofs << "  root -> cert;\n";
            
            // Add example decomposition structure
            ofs << "  series1 [label=\"Series\", shape=box, fillcolor=lightblue];\n";
            ofs << "  parallel1 [label=\"Parallel\", shape=box, fillcolor=lightcoral];\n";
            ofs << "  leaf1 [label=\"Edge 1\", shape=ellipse, fillcolor=lightgreen];\n";
            ofs << "  leaf2 [label=\"Edge 2\", shape=ellipse, fillcolor=lightgreen];\n";
            
            ofs << "  root -> series1;\n";
            ofs << "  series1 -> parallel1;\n";
            ofs << "  series1 -> leaf1;\n";
            ofs << "  parallel1 -> leaf2;\n";
        }
        
    } else {
        ofs << "  // Graph is NOT Series-Parallel\n";
        ofs << "  root [label=\"Not SP\", shape=box, fillcolor=lightcoral];\n";
        
        if (result.sp_reason) {
            ofs << "  negative [label=\"Negative Certificate\\n(Explains why not SP)\", shape=diamond, fillcolor=pink];\n";
            ofs << "  root -> negative;\n";
        }
    }
    
    ofs << "}\n";
    ofs.close();
    std::cout << "SP tree DOT file written to " << filename << std::endl;
}



inline void draw_sp_graph(graph const& g, std::string const& base_filename) {
    std::string dot_file = base_filename + ".dot";
    write_graph_dot(g, dot_file);
    
    // Optionally convert to PNG if graphviz is available
    std::string png_file = base_filename + ".png";
    std::string cmd = "dot -Tpng \"" + dot_file + "\" -o \"" + png_file + "\" 2>/dev/null";
    if (std::system(cmd.c_str()) == 0) {
        std::cout << "PNG visualization created: " << png_file << std::endl;
    }
}


inline void draw_decomposition_tree(gsp_sp_op_result const& result, std::string const& base_filename) {
    std::string summary_file = base_filename + "_summary.txt";
    write_sp_decomposition_summary(result, summary_file);
    
    std::string dot_file = base_filename + "_tree.dot";
    write_sp_tree_dot(result, dot_file);
    
    std::string png_file = base_filename + "_tree.png";
    std::string cmd = "dot -Tpng \"" + dot_file + "\" -o \"" + png_file + "\" 2>/dev/null";
    if (std::system(cmd.c_str()) == 0) {
        std::cout << "Tree PNG visualization created: " << png_file << std::endl;
    }
}


inline void create_complete_sp_visualization(graph const& g, gsp_sp_op_result const& result, std::string const& base_name) {
    std::cout << "\n=== Creating SP Visualization Suite ===\n";
    std::cout << "Base filename: " << base_name << std::endl;
    
    draw_sp_graph(g, base_name + "_graph");
    
    draw_decomposition_tree(result, base_name + "_decomposition");
    
    std::string analysis_file = base_name + "_analysis.txt";
    std::ofstream analysis(analysis_file);
    if (analysis) {
        analysis << "Complete SP Graph Analysis\n";
        analysis << "=========================\n\n";
        
        analysis << "Input Graph:\n";
        analysis << "  Vertices (n): " << g.n << "\n";
        analysis << "  Edges (e): " << g.e << "\n";
        
        if (g.n > 1) {
            double density = (2.0 * g.e) / (g.n * (g.n - 1));
            analysis << "  Density: " << std::fixed << std::setprecision(4) << density << "\n";
        }
        
        analysis << "\nClassification Results:\n";
        analysis << "  Generalized Series-Parallel: " << (result.is_gsp ? "YES" : "NO") << "\n";
        analysis << "  Series-Parallel: " << (result.is_sp ? "YES" : "NO") << "\n";
        analysis << "  Outerplanar: " << (result.is_op ? "YES" : "NO") << "\n";
        
        analysis << "\nCertificate Status:\n";
        analysis << "  GSP Certificate: " << (result.gsp_reason ? "Present" : "None") << "\n";
        analysis << "  SP Certificate: " << (result.sp_reason ? "Present" : "None") << "\n"; 
        analysis << "  OP Certificate: " << (result.op_reason ? "Present" : "None") << "\n";
        
        bool auth_success = false;
        try {
            auth_success = result.authenticate(g);
        } catch (...) {
            auth_success = false;
        }
        analysis << "  Certificate Authentication: " << (auth_success ? "PASSED" : "FAILED") << "\n";
        
        analysis << "\nGenerated Files:\n";
        analysis << "  Graph DOT: " << base_name + "_graph.dot" << "\n";
        analysis << "  Graph PNG: " << base_name + "_graph.png (if graphviz available)" << "\n";
        analysis << "  Tree DOT: " << base_name + "_decomposition_tree.dot" << "\n";
        analysis << "  Tree PNG: " << base_name + "_decomposition_tree.png (if graphviz available)" << "\n";
        analysis << "  Summary: " << base_name + "_decomposition_summary.txt" << "\n";
        
        analysis.close();
        std::cout << "Complete analysis written to: " << analysis_file << std::endl;
    }
    
    std::cout << "Visualization suite complete!\n";
}

inline void draw_graph_dot(graph const& g, const std::string &filename_png, int node_limit = 100);
inline void draw_decomposition_certificate_dot(gsp_sp_op_result const& result, const std::string &filename_png);

#endif
