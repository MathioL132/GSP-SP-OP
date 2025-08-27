#include "gsp-sp-op.hxx"
#include "GraphGenerator.hxx"
#include "sp_validation_tests.hxx"
#include "sp_visualization.hxx"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <random>

void recompute_edge_count(graph& g) {
    g.e = 0;
    for (auto& adj : g.adjLists) g.e += adj.size();
    g.e /= 2; // undirected
}

bool test_labeling_and_ordering(graph g, gsp_sp_op_result& orig_result) {
    // Vertex labeling test
    std::vector<int> perm(g.n);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), std::mt19937{std::random_device{}()});

    graph g_permuted;
    g_permuted.n = g.n;
    g_permuted.adjLists.resize(g.n);

    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u < v) g_permuted.add_edge(perm[u], perm[v]);
        }
    }
    recompute_edge_count(g_permuted);

    gsp_sp_op_result r2 = GSP_SP_OP(g_permuted);
    if (!r2.authenticate(g_permuted)) return false;

    for (auto& adj : g_permuted.adjLists) {
        std::shuffle(adj.begin(), adj.end(), std::mt19937{std::random_device{}()});
    }
    gsp_sp_op_result r3 = GSP_SP_OP(g_permuted);
    if (!r3.authenticate(g_permuted)) return false;

    return true;
}

bool multiple_root_test(const graph& g) {
    gsp_sp_op_result orig_result = GSP_SP_OP(g);

    for (int root = 0; root < g.n; ++root) {
        gsp_sp_op_result r = GSP_SP_OP(g); // pass as-is; our classifier does not need root arg
        if (!r.authenticate(g)) return false;
    }

    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adjLists[u]) {
            if (v < 0 || v >= g.n) {
                std::cerr << "Invalid vertex index in adjacency list\n";
                return false;
            }
            bool reciprocal = std::find(g.adjLists[v].begin(), g.adjLists[v].end(), u) != g.adjLists[v].end();
            if (!reciprocal) {
                std::cerr << "Missing reciprocal edge for " << u << " <-> " << v << "\n";
                return false;
            }
            if (u == v) {
                std::cerr << "Self-loop detected at " << u << "\n";
                return false;
            }
        }
    }
    return true;
}

void visualize_sp(graph& g) {
    if (g.n > 15) return; // skip large graphs
    gsp_sp_op_result r = GSP_SP_OP(g);
    if (!r.authenticate(g)) return;

    draw_sp_graph(g, "images/sp_graph.png");
    draw_decomposition_tree(r, "images/decomp_tree.png");
}

int main(int argc, char * argv[]) {
    if (argc < 2) return 1;

    auto dir = std::filesystem::path{argv[1]};

    if (!std::filesystem::is_directory(std::filesystem::status(dir))) {
        std::cout << "nondir\n";
        return 1;
    }

    std::cout << "dir " << dir << "\n\n";

    for (auto file : std::filesystem::directory_iterator{dir}) {
        if (file.path().extension() == ".txt") {
            std::cout << "\n======= FILE " << file << " =======\n";

            std::fstream fin{file.path()};
            if (!fin.is_open()) {
                std::cout << "opening error\n";
                continue;
            }

            graph g;
            fin >> g;
            recompute_edge_count(g); // replace finalize_graph_counts

            V_LOG(g)

            gsp_sp_op_result r = GSP_SP_OP(g);
            bool win = r.authenticate(g);
            if (!win) {
                std::cout << "UH OH YOU'VE GONE AND MESSED IT UP YOU ABSOLUTE BUFFOON\n";
                break;
            }

            if (!test_labeling_and_ordering(g, r)) {
                std::cout << "Labeling / edge-ordering test failed\n";
            }

            if (!multiple_root_test(g)) {
                std::cout << "Multiple root / adjacency validation failed\n";
            }

            visualize_sp(g);
        }
    }

    return 0;
}
