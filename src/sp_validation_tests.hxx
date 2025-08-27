#ifndef __SP_VALIDATION_TESTS_HXX__
#define __SP_VALIDATION_TESTS_HXX__

#include "gsp-sp-op.hxx"
#include "GraphGenerator.hxx"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <unordered_set>

void run_validation_tests_for_graph(graph const& base_graph, const std::string &out_prefix);

inline graph create_permuted_graph(graph const& g, const std::vector<int>& perm) {
    graph g2;
    g2.n = g.n;
    g2.adjLists.resize(g.n);
    
    for (int u = 0; u < g.n; u++) {
        for (int v : g.adjLists[u]) {
            if (u < v) {  // Only process each edge once
                g2.add_edge(perm[u], perm[v]);
            }
        }
    }
    
    int edge_count = 0;
    for (int u = 0; u < g2.n; u++) {
        for (int v : g2.adjLists[u]) {
            if (u < v) edge_count++;
        }
    }
    g2.e = edge_count;
    
    return g2;
}

inline bool vertex_labeling_test(graph const& g) {
    std::vector<int> perm(g.n);
    for (int i = 0; i < g.n; i++) perm[i] = i;
    
    std::random_device rd;
    std::mt19937 g_rng(rd());
    std::shuffle(perm.begin(), perm.end(), g_rng);
    
    graph g2 = create_permuted_graph(g, perm);
    
    gsp_sp_op_result r1 = GSP_SP_OP(g);
    gsp_sp_op_result r2 = GSP_SP_OP(g2);
    
    bool same_results = (r1.is_gsp == r2.is_gsp) && 
                       (r1.is_sp == r2.is_sp) && 
                       (r1.is_op == r2.is_op);
    
    bool auth1 = r1.authenticate(g);
    bool auth2 = r2.authenticate(g2);
    
    return same_results && auth1 && auth2;
}

inline bool edge_ordering_test(graph const& g) {
    graph g_copy = g;
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    for (auto &adj : g_copy.adjLists) {
        std::shuffle(adj.begin(), adj.end(), rng);
    }
    
    gsp_sp_op_result r1 = GSP_SP_OP(g);
    gsp_sp_op_result r2 = GSP_SP_OP(g_copy);
    
    bool same_results = (r1.is_gsp == r2.is_gsp) && 
                       (r1.is_sp == r2.is_sp) && 
                       (r1.is_op == r2.is_op);
    
    return same_results && r1.authenticate(g) && r2.authenticate(g_copy);
}

inline bool multiple_root_test(graph const& g) {
    gsp_sp_op_result base_result = GSP_SP_OP(g);
    
    for (int root = 0; root < std::min(g.n, 10); root++) {  // Test up to 10 different roots
        std::vector<int> perm(g.n);
        std::iota(perm.begin(), perm.end(), 0);
        std::swap(perm[0], perm[root]);
        
        std::random_device rd;
        std::mt19937 rng(rd());
        std::shuffle(perm.begin() + 1, perm.end(), rng);
        
        graph g_reordered = create_permuted_graph(g, perm);
        gsp_sp_op_result r = GSP_SP_OP(g_reordered);
        
        if (r.is_gsp != base_result.is_gsp || 
            r.is_sp != base_result.is_sp || 
            r.is_op != base_result.is_op) {
            return false;
        }
        
        if (!r.authenticate(g_reordered)) {
            return false;
        }
    }
    return true;
}

inline bool adjacency_list_validation(graph const& g) {
    for (int u = 0; u < g.n; u++) {
        std::unordered_set<int> neighbors;
        
        for (int v : g.adjLists[u]) {
            if (v < 0 || v >= g.n) {
                std::cerr << "Invalid vertex " << v << " in adjacency list of " << u << std::endl;
                return false;
            }
            
            if (u == v) {
                std::cerr << "Self-loop detected at vertex " << u << std::endl;
                return false;
            }
            
            if (neighbors.count(v)) {
                std::cerr << "Multi-edge detected: " << u << " -- " << v << std::endl;
                return false;
            }
            neighbors.insert(v);
            
            bool found_reciprocal = false;
            for (int w : g.adjLists[v]) {
                if (w == u) {
                    found_reciprocal = true;
                    break;
                }
            }
            if (!found_reciprocal) {
                std::cerr << "Missing reciprocal edge for " << u << " -- " << v << std::endl;
                return false;
            }
        }
    }
    return true;
}

inline bool certificate_authentication_test(graph const& g) {
    gsp_sp_op_result result = GSP_SP_OP(g);
    
    if (!result.authenticate(g)) {
        std::cerr << "Certificate authentication failed" << std::endl;
        return false;
    }
    
    if (result.gsp_reason && !result.gsp_reason->authenticate(g)) {
        std::cerr << "GSP certificate authentication failed" << std::endl;
        return false;
    }
    
    if (result.sp_reason && !result.sp_reason->authenticate(g)) {
        std::cerr << "SP certificate authentication failed" << std::endl;
        return false;
    }
    
    if (result.op_reason && !result.op_reason->authenticate(g)) {
        std::cerr << "OP certificate authentication failed" << std::endl;
        return false;
    }
    
    return true;
}

inline void draw_decomposition_tree_simple(gsp_sp_op_result const& result) {
    if (!result.is_sp) {
        std::cout << "Graph is not series-parallel - no decomposition tree available\n";
        return;
    }
    
    std::cout << "SP Decomposition available (specific tree structure depends on certificate format)\n";
    
    if (result.sp_reason || result.gsp_reason) {
        std::cout << "Certificate type: " << (result.sp_reason ? "SP" : "GSP") << std::endl;
    }
}

inline void run_comprehensive_validation(graph const& g, const std::string &out_prefix) {
    std::cout << "=== Comprehensive SP Validation Tests ===\n";
    std::cout << "Graph: n=" << g.n << ", e=" << g.e << std::endl;
    
    std::cout << "1. Adjacency list validation: ";
    bool adj_valid = adjacency_list_validation(g);
    std::cout << (adj_valid ? "PASS" : "FAIL") << std::endl;
    if (!adj_valid) return;
    
    gsp_sp_op_result result = GSP_SP_OP(g);
    std::cout << "Algorithm results: GSP=" << result.is_gsp 
              << ", SP=" << result.is_sp << ", OP=" << result.is_op << std::endl;
    
    std::cout << "2. Certificate authentication: ";
    bool cert_auth = certificate_authentication_test(g);
    std::cout << (cert_auth ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "3. Vertex labeling invariance: ";
    bool vertex_inv = vertex_labeling_test(g);
    std::cout << (vertex_inv ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "4. Edge ordering invariance: ";
    bool edge_inv = edge_ordering_test(g);
    std::cout << (edge_inv ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "5. Multiple root invariance: ";
    bool root_inv = multiple_root_test(g);
    std::cout << (root_inv ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "6. Decomposition tree visualization:\n";
    draw_decomposition_tree_simple(result);
    
    bool all_passed = adj_valid && cert_auth && vertex_inv && edge_inv && root_inv;
    std::cout << "\nOverall result: " << (all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;
    
    if (!all_passed) {
        std::cout << "Consider saving failed test case with prefix: " << out_prefix << std::endl;
    }
}

inline void run_validation_tests_with_generator() {
    std::cout << "=== Testing with Generated Graphs ===\n";
    
    struct TestCase {
        long nC, lC, nK, lK, three_edges;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        {5, 4, 0, 0, 0, "Cycle-only graphs"},
        {0, 0, 3, 4, 0, "Clique-only graphs"}, 
        {3, 3, 2, 3, 0, "Mixed cycles and cliques"},
        {4, 5, 0, 0, 1, "Cycles with 3-edge connections"}
    };
    
    for (size_t i = 0; i < test_cases.size(); i++) {
        auto& tc = test_cases[i];
        std::cout << "\n--- Test Case " << (i+1) << ": " << tc.description << " ---\n";
        
        try {
            long seed = static_cast<long>(std::time(nullptr)) + static_cast<long>(i);
            graph g = generate_graph(tc.nC, tc.lC, tc.nK, tc.lK, tc.three_edges, seed);
            
            std::string prefix = "gen_test_" + std::to_string(i);
            run_comprehensive_validation(g, prefix);
            
        } catch (const std::exception& e) {
            std::cerr << "Error in test case " << (i+1) << ": " << e.what() << std::endl;
        }
    }
}

#endif 
