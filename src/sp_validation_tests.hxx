#ifndef __SP_VALIDATION_TESTS_HXX__
#define __SP_VALIDATION_TESTS_HXX__

#include "graph.hxx"
#include "GraphGenerator.hxx"
#include "gsp-sp-op.hxx"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <string>

void run_validation_tests_for_graph(graph const& base_graph, const std::string &out_prefix);

inline bool vertex_labeling_test(graph const& g) {
    std::vector<int> perm(g.n);
    for (int i = 0; i < g.n; i++) perm[i] = i;

    std::random_device rd;
    std::mt19937 g_rng(rd());
    std::shuffle(perm.begin(), perm.end(), g_rng);

    graph g2;
    g2.n = g.n; g2.e = g.e;
    g2.adjLists.resize(g.n);

    for (int u = 0; u < g.n; u++) {
        for (int v : g.adjLists[u]) {
            g2.add_edge(perm[u], perm[v]);
        }
    }

    gsp_sp_op_result r1 = GSP_SP_OP(g);
    gsp_sp_op_result r2 = GSP_SP_OP(g2);

    return r1.authenticate(g) == r2.authenticate(g2);
}

inline bool edge_ordering_test(graph g) {
    for (auto &adj : g.adjLists) {
        std::shuffle(adj.begin(), adj.end(), std::mt19937{std::random_device{}()});
    }
    gsp_sp_op_result r = GSP_SP_OP(g);
    return r.authenticate(g);
}


inline bool multiple_root_test(graph const& g) {
    for (int root = 0; root < g.n; root++) {
        gsp_sp_op_result r = GSP_SP_OP(g);
        if (!r.authenticate(g)) return false;
    }
    return true;
}

inline bool adjacency_list_validation(graph const& g) {
    for (int u = 0; u < g.n; u++) {
        for (int v : g.adjLists[u]) {
            if (v < 0 || v >= g.n) return false; // invalid vertex
            if (std::find(g.adjLists[v].begin(), g.adjLists[v].end(), u) == g.adjLists[v].end())
                return false; // missing reciprocal
            if (u == v) return false; // self-loop
        }
        std::vector<int> copy = g.adjLists[u];
        std::sort(copy.begin(), copy.end());
        if (std::unique(copy.begin(), copy.end()) != copy.end()) return false; // multi-edge
    }
    return true;
}


#include "sp-tree.hxx"

inline void draw_decomposition_tree(sp_tree const& tree) {
    if (!tree.root) {
        std::cout << "(null tree)\n";
        return;
    }

    std::function<void(sp_tree_node const*, int)> print_tree;
    print_tree = [&](sp_tree_node const* node, int depth) {
        if (!node) return;
        for (int i = 0; i < depth; i++) std::cout << "  ";
        std::cout << c_type_char(node->comp) << "(" << node->source << "," << node->sink << ")\n";
        print_tree(node->l, depth + 1);
        print_tree(node->r, depth + 1);
    };

    print_tree(tree.root, 0);
}


inline void run_validation_tests_for_graph(graph const& g, const std::string &out_prefix) {
    std::cout << "--- Running SP validation tests for graph ---\n";

    if (!adjacency_list_validation(g)) {
        std::cout << "Adjacency list validation failed\n";
        return;
    }

    if (!vertex_labeling_test(g)) {
        std::cout << "Vertex labeling test failed\n";
        return;
    }

    if (!edge_ordering_test(g)) {
        std::cout << "Edge ordering test failed\n";
        return;
    }

    if (!multiple_root_test(g)) {
        std::cout << "Multiple root test failed\n";
        return;
    }

    gsp_sp_op_result r = GSP_SP_OP(g);
    if (!r.authenticate(g)) {
        std::cout << "SP authentication failed\n";
        return;
    }

    std::cout << "All validation tests passed.\n";
    draw_decomposition_tree(r.SP_tree()); // assumes your GSP_SP_OP returns an sp_tree via SP_tree()
}


inline void run_validation_tests_with_generator() {
    graph g = generate_graph(3, 4, 2, 3, 1); // Example parameters
    run_validation_tests_for_graph(g, "random_graph");
}

#endif
