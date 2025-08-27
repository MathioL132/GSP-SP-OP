#include "gsp-sp-op.hxx"
#include "GraphGenerator.hxx"
#include "sp_validation_tests.hxx"
#include "sp_visualization.hxx"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <numeric>
#include <iomanip>
void recompute_edge_count(graph& g) {
    int count = 0;
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) ++count; 
        }
    }
    g.e = count;
}
graph create_permuted_graph_copy(const graph& g, const std::vector<int>& perm) {
    graph g_permuted;
    g_permuted.n = g.n;
    g_permuted.adjLists.resize(g.n);
    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) {
                g_permuted.add_edge(perm[u], perm[v]);
            }
        }
    }
    recompute_edge_count(g_permuted);
    return g_permuted;
}
bool test_certificate_authentication(const graph& g) {
    std::cout << "  Testing certificate authentication... ";
    gsp_sp_op_result result = GSP_SP_OP(g);
    bool auth_success = false;
    try {
        auth_success = result.authenticate(g);
    } catch (const std::exception& e) {
        std::cerr << "Authentication threw exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Authentication threw unknown exception" << std::endl;
        return false;
    }
    if (!auth_success) {
        std::cout << "FAILED - Overall authentication failed\n";
        return false;
    }
    try {
        if (result.gsp_reason && !result.gsp_reason->authenticate(g)) {
            std::cout << "FAILED - GSP certificate authentication failed\n";
            return false;
        }
        if (result.sp_reason && !result.sp_reason->authenticate(g)) {
            std::cout << "FAILED - SP certificate authentication failed\n";
            return false;
        }
        if (result.op_reason && !result.op_reason->authenticate(g)) {
            std::cout << "FAILED - OP certificate authentication failed\n";
            return false;
        }
    } catch (...) {
        std::cout << "FAILED - Individual certificate authentication threw exception\n";
        return false;
    }
    std::cout << "PASSED\n";
    return true;
}
bool test_labeling_and_ordering_invariance(const graph& g) {
    std::cout << "  Testing labeling and edge ordering invariance... ";
    gsp_sp_op_result orig_result = GSP_SP_OP(g);
    for (int trial = 0; trial < 3; ++trial) {
        std::vector<int> perm(g.n);
        std::iota(perm.begin(), perm.end(), 0);
        std::random_device rd;
        std::mt19937 rng(rd());
        std::shuffle(perm.begin(), perm.end(), rng);
        graph g_permuted = create_permuted_graph_copy(g, perm);
        gsp_sp_op_result r_permuted = GSP_SP_OP(g_permuted);
        if (r_permuted.is_gsp != orig_result.is_gsp ||
            r_permuted.is_sp != orig_result.is_sp ||
            r_permuted.is_op != orig_result.is_op) {
            std::cout << "FAILED - Different results after vertex relabeling (trial " << trial << ")\n";
            return false;
        }
        if (!r_permuted.authenticate(g_permuted)) {
            std::cout << "FAILED - Certificate authentication failed after relabeling (trial " << trial << ")\n";
            return false;
        }
        graph g_shuffled = g_permuted; 
        for (auto& adj : g_shuffled.adjLists) {
            std::shuffle(adj.begin(), adj.end(), rng);
        }
        gsp_sp_op_result r_shuffled = GSP_SP_OP(g_shuffled);
        if (r_shuffled.is_gsp != orig_result.is_gsp ||
            r_shuffled.is_sp != orig_result.is_sp ||
            r_shuffled.is_op != orig_result.is_op) {
            std::cout << "FAILED - Different results after edge reordering (trial " << trial << ")\n";
            return false;
        }
        if (!r_shuffled.authenticate(g_shuffled)) {
            std::cout << "FAILED - Certificate authentication failed after edge reordering (trial " << trial << ")\n";
            return false;
        }
    }
    std::cout << "PASSED\n";
    return true;
}
bool test_multiple_root_and_validation(const graph& g) {
    std::cout << "  Testing multiple root invariance and adjacency validation... ";
    for (int u = 0; u < g.n; ++u) {
        std::set<int> neighbors; 
        for (int v : g.adjLists[u]) {
            if (v < 0 || v >= g.n) {
                std::cout << "FAILED - Invalid vertex index " << v << " in adjacency list of " << u << "\n";
                return false;
            }
            if (u == v) {
                std::cout << "FAILED - Self-loop detected at vertex " << u << "\n";
                return false;
            }
            if (neighbors.count(v)) {
                std::cout << "FAILED - Multi-edge detected: " << u << " appears multiple times in adjacency list of " << v << "\n";
                return false;
            }
            neighbors.insert(v);
            bool reciprocal = std::find(g.adjLists[v].begin(), g.adjLists[v].end(), u) != g.adjLists[v].end();
            if (!reciprocal) {
                std::cout << "FAILED - Missing reciprocal edge for " << u << " <-> " << v << "\n";
                return false;
            }
        }
    }
    gsp_sp_op_result orig_result = GSP_SP_OP(g);
    for (int root_candidate = 0; root_candidate < std::min(g.n, 5); ++root_candidate) {
        std::vector<int> perm(g.n);
        std::iota(perm.begin(), perm.end(), 0);
        std::swap(perm[0], perm[root_candidate]);
        std::random_device rd;
        std::mt19937 rng(rd());
        std::shuffle(perm.begin() + 1, perm.end(), rng);
        graph g_reordered = create_permuted_graph_copy(g, perm);
        gsp_sp_op_result r = GSP_SP_OP(g_reordered);
        if (r.is_gsp != orig_result.is_gsp ||
            r.is_sp != orig_result.is_sp ||
            r.is_op != orig_result.is_op) {
            std::cout << "FAILED - Different results with root candidate " << root_candidate << "\n";
            return false;
        }
        if (!r.authenticate(g_reordered)) {
            std::cout << "FAILED - Authentication failed with root candidate " << root_candidate << "\n";
            return false;
        }
    }
    std::cout << "PASSED\n";
    return true;
}
bool test_and_create_visualization(const graph& g, const std::string& test_name) {
    std::cout << "  Creating visualization... ";
    if (g.n > 50) {
        std::cout << "SKIPPED (graph too large: " << g.n << " vertices)\n";
        return true;
    }
    try {
        gsp_sp_op_result result = GSP_SP_OP(g);
        if (!result.authenticate(g)) {
            std::cout << "FAILED - Result authentication failed\n";
            return false;
        }
        std::filesystem::create_directories("visualization_output");
        std::string base_filename = "visualization_output/" + test_name;
        create_complete_sp_visualization(g, result, base_filename);
        std::cout << "COMPLETED (files saved to visualization_output/)\n";
        return true;
    } catch (const std::exception& e) {
        std::cout << "FAILED - Exception during visualization: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cout << "FAILED - Unknown exception during visualization\n";
        return false;
    }
}
void test_random_graphs() {
    std::cout << "\n=== Task 1: Testing with Generated Random Graphs ===\n";
    struct TestParams {
        long nC, lC, nK, lK, three_edges;
        std::string description;
    };
    std::vector<TestParams> test_cases = {
        {3, 4, 0, 0, 0, "small_cycles"},
        {0, 0, 2, 4, 0, "small_cliques"},
        {2, 3, 1, 3, 0, "mixed_small"},
        {5, 5, 0, 0, 1, "cycles_3edges"}
    };
    for (size_t i = 0; i < test_cases.size(); ++i) {
        const auto& params = test_cases[i];
        std::cout << "\n--- Random Graph Test " << (i+1) << ": " << params.description << " ---\n";
        try {
            long seed = static_cast<long>(std::time(nullptr)) + static_cast<long>(i * 12345);
            graph g = generate_graph(params.nC, params.lC, params.nK, params.lK, params.three_edges, seed);
            recompute_edge_count(g);
            std::cout << "Generated graph: n=" << g.n << ", e=" << g.e << std::endl;
            bool all_passed = true;
            all_passed &= test_certificate_authentication(g);
            all_passed &= test_labeling_and_ordering_invariance(g);
            all_passed &= test_multiple_root_and_validation(g);
            all_passed &= test_and_create_visualization(g, params.description);
            std::cout << "Test " << (i+1) << " Result: " << (all_passed ? "ALL PASSED" : "SOME FAILED") << "\n";
        } catch (const std::exception& e) {
            std::cout << "Test " << (i+1) << " FAILED with exception: " << e.what() << std::endl;
        }
    }
}
int main(int argc, char* argv[]) {
    std::cout << "=== SP Graph Testing and Visualization Suite ===\n";
    std::cout << "Testing all 5 tasks:\n";
    std::cout << "  Task 1: Generate Random Graphs\n";
    std::cout << "  Task 2: Certificate Authentication\n";
    std::cout << "  Task 3: Output Same Everywhere on Graph\n";
    std::cout << "  Task 4: Output Same from Anywhere on Graph\n";
    std::cout << "  Task 5: Draw SP Graph & Decomposition Tree\n\n";
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_directory>\n";
        std::cout << "Running with generated graphs only...\n\n";
        test_random_graphs();
        return 0;
    }
    auto dir = std::filesystem::path{argv[1]};
    if (!std::filesystem::is_directory(std::filesystem::status(dir))) {
        std::cout << "Error: " << dir << " is not a directory\n";
        std::cout << "Running with generated graphs only...\n\n";
        test_random_graphs();
        return 1;
    }
    std::cout << "Testing graphs from directory: " << dir << "\n\n";
    int total_tests = 0;
    int passed_tests = 0;
    for (auto file : std::filesystem::directory_iterator{dir}) {
        if (file.path().extension() == ".txt") {
            std::string filename = file.path().filename().string();
            std::cout << "\n======= Testing File: " << filename << " =======\n";
            total_tests++;
            std::fstream fin{file.path()};
            if (!fin.is_open()) {
                std::cout << "Error: Cannot open file " << file.path() << "\n";
                continue;
            }
            graph g;
            try {
                fin >> g;
                recompute_edge_count(g);
            } catch (...) {
                std::cout << "Error: Failed to read graph from file\n";
                continue;
            }
            std::cout << "Graph loaded: n=" << g.n << ", e=" << g.e << std::endl;
            bool all_tests_passed = true;
            all_tests_passed &= test_certificate_authentication(g);
            all_tests_passed &= test_labeling_and_ordering_invariance(g);
            all_tests_passed &= test_multiple_root_and_validation(g);
            std::string clean_filename = filename;
            std::replace(clean_filename.begin(), clean_filename.end(), '.', '_');
            all_tests_passed &= test_and_create_visualization(g, clean_filename);
            if (all_tests_passed) {
                passed_tests++;
                std::cout << "*** ALL TESTS PASSED for " << filename << " ***\n";
            } else {
                std::cout << "*** SOME TESTS FAILED for " << filename << " ***\n";
            }
        }
    }
    std::cout << "\n=== File Testing Summary ===\n";
    std::cout << "Total files tested: " << total_tests << std::endl;
    std::cout << "Files passed all tests: " << passed_tests << std::endl;
    std::cout << "Files with failures: " << (total_tests - passed_tests) << std::endl;
    if (total_tests > 0) {
        double success_rate = 100.0 * passed_tests / total_tests;
        std::cout << "Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%\n";
    }
    test_random_graphs();
    std::cout << "\n=== Testing Complete ===\n";
    std::cout << "Check visualization_output/ directory for generated visualizations.\n";
    return (passed_tests == total_tests) ? 0 : 1;
}
