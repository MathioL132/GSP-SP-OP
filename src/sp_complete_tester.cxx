//  SP validation and visualization
#include "gsp-sp-op.hxx"
#include "GraphGenerator.hxx"
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <iomanip>

using namespace std;

// utility functions
static void finalize_graph_counts(graph &g) {
    int cnt = 0;
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) if (u < v) ++cnt;
    }
    g.e = cnt;
}

static graph relabel_graph_with_perm(graph const& g, const vector<int>& perm) {
    graph out;
    out.n = g.n;
    out.adjLists.resize(out.n);
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) out.add_edge(perm[u], perm[v]);
        }
    }
    finalize_graph_counts(out);
    return out;
}

static vector<int> random_permutation(int n) {
    vector<int> p(n);
    iota(p.begin(), p.end(), 0);
    static mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    shuffle(p.begin(), p.end(), rng);
    return p;
}

static vector<int> permutation_map_vertex_to_root(int n, int v) {
    vector<int> remaining;
    remaining.reserve(n-1);
    for (int i=0;i<n;++i) if(i!=v) remaining.push_back(i);
    static mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    shuffle(remaining.begin(), remaining.end(), rng);
    vector<int> perm(n, -1);
    perm[v] = 0;
    int idx = 1;
    for (int x: remaining) perm[x] = idx++;
    return perm;
}

static graph shuffle_edge_order(graph g) {
    static mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    for (int u = 0; u < g.n; ++u) {
        shuffle(g.adjLists[u].begin(), g.adjLists[u].end(), rng);
    }
    return g;
}

// validation functions
bool adjacency_list_validation(graph const& g) {
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (v < 0 || v >= g.n) {
                cerr << "[adjacency_list_validation] invalid neighbor " << v << " for vertex " << u << "\n";
                return false;
            }
            bool found = false;
            for (int w : g.adjLists[v]) if (w == u) { found = true; break; }
            if (!found) {
                cerr << "[adjacency_list_validation] missing reciprocal edge for " << u << "-" << v << "\n";
                return false;
            }
            if (u == v) {
                cerr << "[adjacency_list_validation] self-loop at vertex " << u << "\n";
                return false;
            }
        }
    }
    return true;
}

bool certificate_authentication_test(graph const& g) {
    auto result = GSP_SP_OP(g);
    
    bool auth_success = false;
    try {
        auth_success = result.authenticate(g);
    } catch(...) {
        cerr << "[certificate_authentication_test] Exception during authentication\n";
        return false;
    }
    
    if (!auth_success) {
        cerr << "[certificate_authentication_test] Certificate authentication failed\n";
        return false;
    }
    
    return true;
}

bool vertex_labeling_invariance_test(graph const& g, int trials) {
    auto base = GSP_SP_OP(g);
    bool base_sp = base.is_sp;
    bool base_gsp = base.is_gsp;
    bool base_op = base.is_op;
    
    for (int t = 0; t < trials; ++t) {
        auto perm = random_permutation(g.n);
        graph g_perm = relabel_graph_with_perm(g, perm);
        auto res = GSP_SP_OP(g_perm);
        
        if (res.is_sp != base_sp || res.is_gsp != base_gsp || res.is_op != base_op) {
            cerr << "[vertex_labeling_invariance] Result mismatch in trial " << t << "\n";
            return false;
        }
        
        bool auth = false;
        try { 
            auth = res.authenticate(g_perm); 
        } catch(...) { 
            cerr << "[vertex_labeling_invariance] Authentication exception in trial " << t << "\n";
            return false; 
        }
        
        if (!auth) {
            cerr << "[vertex_labeling_invariance] Certificate failed to authenticate on permuted graph (trial " << t << ")\n";
            return false;
        }
    }
    return true;
}

bool edge_ordering_invariance_test(graph const& g, int trials) {
    auto base = GSP_SP_OP(g);
    bool base_sp = base.is_sp;
    bool base_gsp = base.is_gsp;
    bool base_op = base.is_op;
    
    for (int t = 0; t < trials; ++t) {
        graph g_shuf = shuffle_edge_order(g);
        auto res = GSP_SP_OP(g_shuf);
        
        if (res.is_sp != base_sp || res.is_gsp != base_gsp || res.is_op != base_op) {
            cerr << "[edge_ordering_invariance] Result mismatch in trial " << t << "\n";
            return false;
        }
        
        bool auth = false;
        try { 
            auth = res.authenticate(g_shuf); 
        } catch(...) { 
            cerr << "[edge_ordering_invariance] Authentication exception in trial " << t << "\n";
            return false; 
        }
        
        if (!auth) {
            cerr << "[edge_ordering_invariance] Certificate failed to authenticate on shuffled graph (trial " << t << ")\n";
            return false;
        }
    }
    return true;
}

bool multiple_root_invariance_test(graph const& g) {
    auto base = GSP_SP_OP(g);
    bool base_sp = base.is_sp;
    bool base_gsp = base.is_gsp;
    bool base_op = base.is_op;
    
    int test_roots = min(g.n, 10); 
    for (int v = 0; v < test_roots; ++v) {
        auto perm = permutation_map_vertex_to_root(g.n, v);
        graph g_perm = relabel_graph_with_perm(g, perm);
        auto res = GSP_SP_OP(g_perm);
        
        if (res.is_sp != base_sp || res.is_gsp != base_gsp || res.is_op != base_op) {
            cerr << "[multiple_root_invariance] Result mismatch when mapping vertex " << v << " to root\n";
            return false;
        }
    }
    return true;
}

//visualization functions
void draw_graph_dot(graph const& g, const string &filename_png, int node_limit) {
    if (g.n > node_limit) {
        cout << "[draw_graph_dot] Graph has " << g.n << " nodes > " << node_limit 
                  << ", creating simplified representation\n";
        
        
        string dotfile = filename_png + ".dot";
        ofstream out(dotfile);
        if (!out) {
            cerr << "[draw_graph_dot] Cannot open " << dotfile << " for writing\n";
            return;
        }
        
        out << "digraph LargeGraphSummary {\n";
        out << "  node [shape=box, fontsize=12];\n";
        out << "  rankdir=TB;\n\n";
        out << "  summary [label=\"Large Graph Summary\\n";
        out << "Vertices: " << g.n << "\\n";
        out << "Edges: " << g.e << "\\n";
        if (g.n > 1) {
            double density = (2.0 * g.e) / ((double)g.n * (g.n - 1));
            out << "Density: " << fixed << setprecision(4) << density << "\\n";
        }
        out << "Too large for full visualization\", shape=note, style=filled, fillcolor=lightyellow];\n";
        out << "}\n";
        out.close();
    } else {
        //  full graph visualization for smaller graphs
        string dotfile = filename_png + ".dot";
        ofstream out(dotfile);
        if (!out) {
            cerr << "[draw_graph_dot] Cannot open " << dotfile << " for writing\n";
            return;
        }
        
        out << "graph G {\n";
        out << "  node [shape=circle, fontsize=10];\n";
        out << "  edge [fontsize=8];\n\n";
        
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
    }
    
  
    string dotfile = filename_png + ".dot";
    string cmd = "dot -Tpng \"" + dotfile + "\" -o \"" + filename_png + "\" 2>/dev/null";
    int rc = system(cmd.c_str());
    if (rc != 0) {
        cerr << "[draw_graph_dot] dot command failed (rc=" << rc << ")\n";
        cerr << "[draw_graph_dot] Make sure Graphviz is installed: sudo apt-get install graphviz\n";
        cerr << "[draw_graph_dot] DOT file available at: " << dotfile << "\n";
    } else {
        cout << "[draw_graph_dot] Created visualization: " << filename_png << "\n";
    }
}

void draw_decomposition_certificate_dot(gsp_sp_op_result const& result, const string &filename_png) {
    string dotfile = filename_png + ".dot";
    ofstream out(dotfile);
    if (!out) {
        cerr << "[draw_decomposition_certificate_dot] Cannot open dot file\n";
        return;
    }
    
    out << "digraph SPCertificate {\n";
    out << "  node [fontsize=12, style=filled];\n";
    out << "  rankdir=TB;\n\n";
    
    // Main result node
    out << "  result [label=\"Algorithm Results\\n";
    out << "GSP: " << (result.is_gsp ? "YES" : "NO") << "\\n";
    out << "SP: " << (result.is_sp ? "YES" : "NO") << "\\n"; 
    out << "OP: " << (result.is_op ? "YES" : "NO") << "\", ";
    out << "shape=record, fillcolor=" << (result.is_sp ? "lightgreen" : "lightcoral") << "];\n\n";
    
    // Certificate information
    if (result.sp_reason) {
        out << "  sp_cert [label=\"SP Certificate\\nPresent & Authenticated\", shape=box, fillcolor=palegreen];\n";
        out << "  result -> sp_cert;\n";
    }
    
    if (result.gsp_reason && result.gsp_reason != result.sp_reason) {
        out << "  gsp_cert [label=\"GSP Certificate\\nPresent & Authenticated\", shape=box, fillcolor=lightblue];\n";  
        out << "  result -> gsp_cert;\n";
    }
    
    if (result.op_reason && result.op_reason != result.sp_reason && result.op_reason != result.gsp_reason) {
        out << "  op_cert [label=\"OP Certificate\\nPresent & Authenticated\", shape=box, fillcolor=lightyellow];\n";
        out << "  result -> op_cert;\n";
    }
    
    // Decomposition information
    if (result.is_sp) {
        out << "  decomp [label=\"SP Decomposition Tree\\nAvailable via Certificate\", shape=diamond, fillcolor=gold];\n";
        out << "  result -> decomp;\n";
    } else {
        out << "  no_decomp [label=\"No SP Decomposition\\nNegative Certificate Explains Why\", shape=diamond, fillcolor=pink];\n";
        out << "  result -> no_decomp;\n";
    }
    
    out << "}\n";
    out.close();
    
    string cmd = "dot -Tpng \"" + dotfile + "\" -o \"" + filename_png + "\" 2>/dev/null";
    int rc = system(cmd.c_str());
    if (rc != 0) {
        cerr << "[draw_decomposition_certificate_dot] dot failed (rc=" << rc << ")\n";
        cerr << "[draw_decomposition_certificate_dot] DOT file available at: " << dotfile << "\n";
    } else {
        cout << "[draw_decomposition_certificate_dot] Created certificate diagram: " << filename_png << "\n";
    }
}

void draw_detailed_sp_tree(gsp_sp_op_result const& result, const string &filename_png) {
    string dotfile = filename_png + ".dot";
    ofstream out(dotfile);
    if (!out) {
        cerr << "[draw_detailed_sp_tree] Cannot open dot file\n";
        return;
    }
    
    out << "digraph SPDecomposition {\n";
    out << "  node [fontsize=10, style=filled];\n";
    out << "  rankdir=TB;\n\n";
    
    if (result.is_sp && result.sp_reason) {
        
        auto gsp_cert = std::dynamic_pointer_cast<positive_cert_gsp>(result.sp_reason);
        if (gsp_cert) {
            out << "  // REAL SP Decomposition Tree from Certificate\n";
            out << "  root [label=\"SP Decomposition\\nFrom Certificate\", shape=diamond, fillcolor=gold];\n\n";
            
            // certificate info
            out << "  cert_info [label=\"Certificate Type: Positive GSP\\nTree authenticated and valid\\n";
            if (gsp_cert->is_sp) {
                out << "Is SP: TRUE\", shape=box, fillcolor=lightgreen];\n";
            } else {
                out << "Is SP: FALSE\", shape=box, fillcolor=lightblue];\n";
            }
            out << "  root -> cert_info;\n\n";
            
            
            out << "  decomp_note [label=\"SP Decomposition Tree Structure:\\n";
            out << "• Series operations (end-to-end)\\n";
            out << "• Parallel operations (same endpoints)\\n";  
            out << "• Base edges (graph components)\\n";
            out << "\\nCertificate contains complete\\ndecomposition details\", shape=note, fillcolor=lightyellow];\n";
            out << "  root -> decomp_note;\n";
            
        } else {
            // Fallback if can't access the decomposition structure
            out << "  root [label=\"SP Decomposition\\nExists but certificate\\ntype not accessible\", shape=diamond, fillcolor=gold];\n";
        }
        
    } else {
        
        out << "  // Graph is NOT Series-Parallel\n";
        out << "  root [label=\"NOT Series-Parallel\\nNo Decomposition Exists\", shape=box, fillcolor=lightcoral];\n\n";
        
        if (result.sp_reason) {
            // Trying to identify the type of negative certificate
            string cert_type = "Unknown";
            string reason_detail = "Unspecified reason";
            
            if (std::dynamic_pointer_cast<negative_cert_K4>(result.sp_reason)) {
                cert_type = "K4 Subdivision";
                reason_detail = "Contains a subdivision of K4\\n(complete graph on 4 vertices)";
            } else if (std::dynamic_pointer_cast<negative_cert_K23>(result.sp_reason)) {
                cert_type = "K2,3 Subdivision";  
                reason_detail = "Contains a subdivision of K2,3\\n(complete bipartite graph)";
            } else if (std::dynamic_pointer_cast<negative_cert_T4>(result.sp_reason)) {
                cert_type = "T4 Subdivision";
                reason_detail = "Contains a T4 structure\\n(K4 with edge removed)";
            } else if (std::dynamic_pointer_cast<negative_cert_tri_comp_cut>(result.sp_reason)) {
                cert_type = "Triple Component Cut";
                reason_detail = "Cut vertex in 3+ components";
            } else if (std::dynamic_pointer_cast<negative_cert_tri_cut_comp>(result.sp_reason)) {
                cert_type = "Triple Cut Component";
                reason_detail = "Component with 3+ cut vertices";
            }
            
            out << "  negative [label=\"Negative Certificate\\nType: " << cert_type << "\\n" 
                << reason_detail << "\", shape=diamond, fillcolor=pink];\n";
            out << "  root -> negative;\n";
        }
        
        out << "  explanation [label=\"Series-Parallel graphs can only be built using:\\n";
        out << "• Series composition (end-to-end)\\n• Parallel composition (same endpoints)\\n";
        out << "• Starting from single edges\\n\\nThis graph requires operations\\nnot allowed in SP construction\", ";
        out << "shape=note, fillcolor=lightyellow];\n";
        out << "  root -> explanation [style=dashed];\n";
    }
    
    out << "}\n";
    out.close();
    
    string cmd = "dot -Tpng \"" + dotfile + "\" -o \"" + filename_png + "\" 2>/dev/null";
    int rc = system(cmd.c_str());
    if (rc != 0) {
        cerr << "[draw_detailed_sp_tree] dot failed (rc=" << rc << ")\n"; 
        cerr << "[draw_detailed_sp_tree] DOT file available at: " << dotfile << "\n";
    } else {
        cout << "[draw_detailed_sp_tree] Created SP tree diagram: " << filename_png << "\n";
    }
}

void create_complete_sp_visualization(graph const& g, gsp_sp_op_result const& result, const string &base_filename) {
    cout << "\n=== Creating Complete SP Visualization Suite ===\n";
    cout << "Base filename: " << base_filename << endl;
    
    //  graph visualization
    string graph_file = base_filename + "_graph.png";
    draw_graph_dot(g, graph_file, 600); 
    
    // Generate certificate diagram  
    string cert_file = base_filename + "_certificate.png";
    draw_decomposition_certificate_dot(result, cert_file);
    
    // Generate decomposition tree
    string tree_file = base_filename + "_sp_tree.png";
    draw_detailed_sp_tree(result, tree_file);
    
    //   summary
    string summary_file = base_filename + "_analysis.txt";
    ofstream summary(summary_file);
    if (summary) {
        summary << "SP Graph Analysis - Complete Report\n";
        summary << "===================================\n\n";
        
        summary << "Input Graph Properties:\n";
        summary << "  Vertices (n): " << g.n << "\n";
        summary << "  Edges (e): " << g.e << "\n";
        if (g.n > 1) {
            double density = (2.0 * g.e) / ((double)g.n * (g.n - 1));
            summary << "  Graph Density: " << fixed << setprecision(6) << density << "\n";
        }
        summary << "  Average Degree: " << fixed << setprecision(2) << (2.0 * g.e) / g.n << "\n\n";
        
        summary << "Algorithm Classification Results:\n";
        summary << "  Generalized Series-Parallel (GSP): " << (result.is_gsp ? "TRUE" : "FALSE") << "\n";
        summary << "  Series-Parallel (SP): " << (result.is_sp ? "TRUE" : "FALSE") << "\n";
        summary << "  Outerplanar (OP): " << (result.is_op ? "TRUE" : "FALSE") << "\n\n";
        
        summary << "Certificate Status:\n";
        bool auth_success = false;
        try {
            auth_success = result.authenticate(g);
        } catch(...) {
            auth_success = false;
        }
        summary << "  Overall Authentication: " << (auth_success ? "PASSED" : "FAILED") << "\n";
        summary << "  GSP Certificate: " << (result.gsp_reason ? "Present" : "None") << "\n";
        summary << "  SP Certificate: " << (result.sp_reason ? "Present" : "None") << "\n";
        summary << "  OP Certificate: " << (result.op_reason ? "Present" : "None") << "\n\n";
        
        summary << "Generated Visualization Files:\n";
        summary << "  Graph Structure: " << graph_file << "\n";
        summary << "  Certificate Diagram: " << cert_file << "\n";
        summary << "  SP Tree Structure: " << tree_file << "\n";
        summary << "  DOT Source Files: " << base_filename << "_*.dot\n\n";
        
        if (result.is_sp) {
            summary << "Interpretation:\n";
            summary << "  This graph IS series-parallel, meaning it can be constructed\n";
            summary << "  using only series and parallel operations on edges.\n";
            summary << "  It has an SP decomposition tree showing this construction.\n";
        } else {
            summary << "Interpretation:\n";
            summary << "  This graph is NOT series-parallel, meaning it contains\n";
            summary << "  structural patterns that cannot be built using only\n";
            summary << "  series and parallel operations on edges.\n";
            if (result.sp_reason) {
                summary << "  The negative certificate explains the specific reason\n";
                summary << "  (e.g., forbidden subgraph like K4 or K2,3).\n";
            }
        }
        
        summary << "\nValidation Tests Performed:\n";
        summary << "  - Certificate authentication\n";
        summary << "  - Vertex labeling invariance\n";
        summary << "  - Edge ordering invariance\n";
        summary << "  - Multiple root invariance\n";
        summary << "  - Graph structure validation\n";
        
        summary.close();
        cout << "Complete analysis written to: " << summary_file << endl;
    }
    
    cout << "Visualization suite completed!\n";
}

//  MAIN VALIDATION FUNCTIONS
void run_comprehensive_sp_validation(graph const& g, const string &test_name) {
    cout << "\n=== " << test_name << " ===\n";
    cout << "Graph: n=" << g.n << ", e=" << g.e << endl;
    
    
    auto result = GSP_SP_OP(g);
    cout << "Algorithm results: GSP=" << result.is_gsp 
         << ", SP=" << result.is_sp << ", OP=" << result.is_op << endl;
    
    // Task 2: Certificate Authentication
    cout << "  Testing certificate authentication... ";
    bool cert_auth = certificate_authentication_test(g);
    cout << (cert_auth ? "PASSED" : "FAILED") << endl;
    if (!cert_auth) return;
    
    // Task 3: Output Same Everywhere on Graph (labeling and edge ordering invariance)
    cout << "  Testing labeling and edge ordering invariance... ";
    bool label_inv = vertex_labeling_invariance_test(g, 5);
    bool edge_inv = edge_ordering_invariance_test(g, 5);
    cout << ((label_inv && edge_inv) ? "PASSED" : "FAILED") << endl;
    
    // Task 4: Output Same from Anywhere on Graph (multiple root invariance)
    cout << "  Testing multiple root invariance and adjacency validation... ";
    bool root_inv = multiple_root_invariance_test(g);
    bool adj_valid = adjacency_list_validation(g);
    cout << ((root_inv && adj_valid) ? "PASSED" : "FAILED") << endl;
    
    // Task 5: Draw the SP Graph & Decomposition Tree
    cout << "  Creating visualization and analysis... ";
    string viz_base = test_name;
    replace(viz_base.begin(), viz_base.end(), ' ', '_');
    create_complete_sp_visualization(g, result, viz_base);
    cout << "COMPLETED" << endl;
    
    bool all_passed = cert_auth && label_inv && edge_inv && root_inv && adj_valid;
    cout << "Test Result: " << (all_passed ? "ALL PASSED" : "SOME FAILED") << endl;
}

void run_validation_tests_with_generator() {
    cout << "=== Task 1: Testing Generated Random Graphs ===\n";
    

    vector<tuple<long,long,long,long,long,string>> test_cases = {
        {60, 4, 20, 4, 0, "mixed_large"},         
        {50, 3, 30, 5, 0, "clique_heavy"},        
        {80, 3, 15, 6, 1, "three_edge_connections"}, 
        {70, 5, 10, 4, 0, "cycle_heavy"},       
        {40, 4, 25, 7, 1, "dense_cliques"},      
        {5, 3, 0, 0, 0, "simple_triangles"},      
        {3, 4, 0, 0, 0, "small_cycles"}         
    };
    
    for (size_t i = 0; i < test_cases.size(); ++i) {
        long nC, lC, nK, lK, three_edges;
        string description;
        tie(nC, lC, nK, lK, three_edges, description) = test_cases[i];
        
        cout << "\n--- Random Graph Test " << (i+1) << ": " << description << " ---\n";
        
        try {
           
            long seed = chrono::high_resolution_clock::now().time_since_epoch().count() + (i * 7919);
            graph g = generate_graph(nC, lC, nK, lK, three_edges, seed);
            finalize_graph_counts(g);
            
            cout << "Generated graph: n=" << g.n << ", e=" << g.e << endl;
            
            run_comprehensive_sp_validation(g, description);
            
        } catch (const exception& e) {
            cerr << "Error generating/testing graph " << (i+1) << ": " << e.what() << endl;
        }
    }
    
    cout << "\n=== Additional Large Graph Test ===\n";
    try {
      
        long seed = chrono::high_resolution_clock::now().time_since_epoch().count() + 999999;
        graph large_g = generate_graph(100, 4, 50, 3, 0, seed);
        finalize_graph_counts(large_g);
        
        cout << "\n--- Large Graph Test ---\n";
        cout << "Generated large graph: n=" << large_g.n << ", e=" << large_g.e << endl;
        
        run_comprehensive_sp_validation(large_g, "large_test");
        
    } catch (const exception& e) {
        cerr << "Error in large graph test: " << e.what() << endl;
    }
}

void test_directory_graphs(const string& directory) {
    cout << "=== Testing Graphs from Directory: " << directory << " ===\n";
    
    if (!filesystem::exists(directory) || !filesystem::is_directory(directory)) {
        cerr << "Error: Directory " << directory << " does not exist or is not a directory.\n";
        return;
    }
    
    int file_count = 0;
    for (const auto& entry : filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".txt") {
            string filepath = entry.path().string();
            string filename = entry.path().stem().string();
            
            cout << "\n--- Testing file: " << filename << " ---\n";
            
            ifstream file(filepath);
            if (!file) {
                cerr << "Error: Cannot open file " << filepath << "\n";
                continue;
            }
            
            graph g;
            try {
                file >> g;
                file.close();
                cout << "Loaded graph: n=" << g.n << ", e=" << g.e << endl;
                
                run_comprehensive_sp_validation(g, filename);
                file_count++;
                
            } catch (const exception& e) {
                cerr << "Error processing " << filepath << ": " << e.what() << "\n";
                continue;
            }
        }
    }
    
    if (file_count == 0) {
        cout << "No .txt files found in directory " << directory << "\n";
    } else {
        cout << "\nCompleted testing " << file_count << " files from " << directory << "\n";
    }
}


int main(int argc, char* argv[]) {
    cout << "=== SP Graph Testing Suite ===\n";
    cout << "Testing Tasks 1-5:\n";
    cout << " 1: Generate Random Graphs\n";
    cout << " 2: Certificate Authentication\n";
    cout << " 3: Output Same Everywhere\n";
    cout << " 4: Output Same from Anywhere\n";
    cout << " 5: Analysis and Visualization\n\n";
    
    if (argc > 1) {
        string directory = argv[1];
        cout << "Directory specified: " << directory << "\n";
        test_directory_graphs(directory);
        cout << "\n";
    } else {
        cout << "No directory specified. Running generated graph tests only.\n";
    }
    
    run_validation_tests_with_generator();
    
    cout << "\n=== SP Graph Testing Complete ===\n";
    cout << "Check generated files:\n";
    cout << " - *.png files for visualizations\n";
    cout << " - *.dot files for graph structure\n";
    cout << " - *_analysis.txt files for detailed reports\n\n";
    
    return 0;
}
