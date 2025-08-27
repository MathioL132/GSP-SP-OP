#include "sp_visualization.hxx"
#include "gsp-sp-op-certificates.hxx"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>

static std::string escape_label_for_dot(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
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
    for (int u = 0; u < g.n; ++u) out << "  " << u << " [label=\"" << u << "\"];\n    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) out << "  " << u << " -- " << v << ";\n";
        }
    }
    out << "}\n";
    out.close();

    std::string cmd = "dot -Tpng " + dotfile + " -o " + filename_png;
    int rc = system(cmd.c_str());
    if (rc != 0) std::cerr << "[draw_graph_dot] dot failed (rc=" << rc << ")\n";
    else std::cerr << "[draw_graph_dot] wrote " << filename_png << "\n";
}

void draw_decomposition_certificate_dot(gsp_sp_op_result const& result, const std::string &filename_png) {
    if (!result.is_sp) {
        std::cerr << "[draw_decomposition_certificate_dot] result is not SP -> skipping\n";
        return;
    }

    std::shared_ptr<positive_cert_gsp> gsp = nullptr;
    if (result.sp_reason) gsp = std::dynamic_pointer_cast<positive_cert_gsp>(result.sp_reason);
    if (!gsp && result.gsp_reason) gsp = std::dynamic_pointer_cast<positive_cert_gsp>(result.gsp_reason);

    std::string dotfile = filename_png + ".dot";
    std::ofstream out(dotfile);
    if (!out) {
        std::cerr << "[draw_decomposition_certificate_dot] cannot open dot file\n";
        return;
    }
    out << "digraph Decomposition {\n";
    out << "  node [shape=record, fontsize=10];\n";

    std::string label;
    if (gsp && gsp->decomposition.root) {
        std::ostringstream oss;
        oss << gsp->decomposition; // uses operator<< from sp-tree.hxx
        label = oss.str();
    } else {
        label = "SP decomposition (opaque)";
    }
    if (label.size() > 1000) label = label.substr(0,1000) + " ...";

    out << "  root [label=\"" << escape_label_for_dot(label) << "\"];\n";
    out << "}\n";
    out.close();

    std::string cmd = "dot -Tpng " + dotfile + " -o " + filename_png;
    int rc = system(cmd.c_str());
    if (rc != 0) std::cerr << "[draw_decomposition_certificate_dot] dot failed (rc=" << rc << ")\n";
    else std::cerr << "[draw_decomposition_certificate_dot] wrote " << filename_png << "\n";
}
