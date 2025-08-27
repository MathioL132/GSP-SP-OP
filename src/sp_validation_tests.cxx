#include "sp_validation_tests.hxx"
#include "GraphGenerator.hxx"
#include "gsp-sp-op.hxx"
#include "logging.hxx"
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <chrono>

using namespace std;

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
    // add each undirected edge once
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
    static std::mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    shuffle(p.begin(), p.end(), rng);
    return p;
}

static vector<int> permutation_map_vertex_to_root(int n, int v) {
    vector<int> remaining;
    remaining.reserve(n-1);
    for (int i=0;i<n;++i) if(i!=v) remaining.push_back(i);
    static std::mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    shuffle(remaining.begin(), remaining.end(), rng);
    vector<int> perm(n, -1);
    perm[v] = 0;
    int idx = 1;
    for (int x: remaining) perm[x] = idx++;
    return perm;
}

static graph shuffle_edge_order(graph g) {
    static std::mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    for (int u = 0; u < g.n; ++u) {
        shuffle(g.adjLists[u].begin(), g.adjLists[u].end(), rng);
    }
    return g;
}

bool reduction_is_sp_from_graph(graph const& g) {
    int n = g.n;
    if (n < 2) return false;
    vector<unordered_map<int,int>> maddj((size_t)n);
    for (int u = 0; u < n; ++u) {
        for (int v : g.adjLists[u]) {
            if (u == v) continue;
            maddj[u][v] += 1;
        }
    }
    vector<char> alive((size_t)n, 1);
    int alive_count = n;
    bool changed = true;
    int iterations = 0;
    while (changed && iterations < 10*n + 1000) {
        changed = false;
        ++iterations;
        // Parallel reduction: collapse multiplicity
        for (int u = 0; u < n; ++u) {
            if (!alive[u]) continue;
            for (auto &kv : maddj[u]) {
                int v = kv.first;
                if (!alive[v]) continue;
                if (kv.second > 1) {
                    kv.second = 1;
                    maddj[v][u] = 1;
                    changed = true;
                }
            }
        }
        int x = -1;
        for (int v = 0; v < n; ++v) {
            if (!alive[v]) continue;
            int deg = 0;
            for (auto &kv : maddj[v]) if (alive[kv.first]) deg += kv.second;
            if (deg == 2) { x = v; break; }
        }
        if (x != -1) {
            vector<int> neigh;
            for (auto &kv : maddj[x]) {
                int w = kv.first;
                if (!alive[w]) continue;
                for (int t=0;t<kv.second;++t) neigh.push_back(w);
            }
            if (neigh.size() >= 2) {
                int u = neigh[0], v = neigh[1];
                if (u != v) {
                    maddj[u][v] += 1;
                    maddj[v][u] += 1;
                    for (auto &kv : maddj[x]) {
                        int w = kv.first;
                        maddj[w].erase(x);
                    }
                    maddj[x].clear();
                    alive[x] = 0;
                    --alive_count;
                    changed = true;
                } else {
                    break;
                }
            } else break;
        }
    }
    int remaining_vertices = 0;
    for (int i=0;i<n;++i) if (alive[i]) ++remaining_vertices;
    int remaining_edges = 0;
    for (int u=0; u<n; ++u) if (alive[u]) {
        for (auto &kv : maddj[u]) if (alive[kv.first]) remaining_edges += kv.second;
    }
    remaining_edges /= 2;
    return (remaining_vertices == 2 && remaining_edges == 1);
}

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

bool vertex_labeling_invariance_test(graph const& g, int trials=8) {
    auto base = GSP_SP_OP(g);
    bool base_sp = base.is_sp;
    for (int t=0;t<trials;++t) {
        auto perm = random_permutation((int)g.n);
        graph g_perm = relabel_graph_with_perm(g, perm);
        auto res = GSP_SP_OP(g_perm);
        if (res.is_sp != base_sp) {
            cerr << "[vertex_labeling_invariance] mismatch trial " << t << "\n";
            return false;
        }
        bool auth = false;
        try { auth = res.authenticate(g_perm); } catch(...) { auth = false; }
        if (!auth) {
            cerr << "[vertex_labeling_invariance] certificate failed to authenticate on permuted graph (trial " << t << ")\n";
            return false;
        }
    }
    return true;
}

bool edge_ordering_invariance_test(graph const& g, int trials=8) {
    auto base = GSP_SP_OP(g);
    bool base_sp = base.is_sp;
    for (int t=0;t<trials;++t) {
        graph g_shuf = shuffle_edge_order(g);
        auto res = GSP_SP_OP(g_shuf);
        if (res.is_sp != base_sp) {
            cerr << "[edge_ordering_invariance] mismatch trial " << t << "\n";
            return false;
        }
        bool auth = false;
        try { auth = res.authenticate(g_shuf); } catch(...) { auth = false; }
        if (!auth) {
            cerr << "[edge_ordering_invariance] certificate failed to authenticate on shuffled graph (trial " << t << ")\n";
            return false;
        }
    }
    return true;
}

bool multiple_root_invariance_test(graph const& g) {
    auto base = GSP_SP_OP(g);
    bool base_sp = base.is_sp;
    for (int v=0; v<g.n; ++v) {
        auto perm = permutation_map_vertex_to_root(g.n, v);
        graph g_perm = relabel_graph_with_perm(g, perm);
        auto res = GSP_SP_OP(g_perm);
        if (res.is_sp != base_sp) {
            cerr << "[multiple_root_invariance] mismatch when mapping vertex " << v << " to root\n";
            return false;
        }
    }
    return true;
}

void run_validation_tests_for_graph(graph const& base_graph, const string &out_prefix) {
    cout << "=== run_validation_tests_for_graph (n=" << base_graph.n << ") ===\n";
    bool adj_ok = adjacency_list_validation(base_graph);
    cout << "adjacency list validation: " << (adj_ok ? "OK" : "FAIL") << "\n";

    bool red_sp = reduction_is_sp_from_graph(base_graph);
    cout << "reduction-based is_sp: " << (red_sp ? "SP" : "NOT SP") << "\n";

    bool lab_ok = vertex_labeling_invariance_test(base_graph);
    cout << "vertex labeling invariance: " << (lab_ok ? "OK" : "FAIL") << "\n";

    bool order_ok = edge_ordering_invariance_test(base_graph);
    cout << "edge ordering invariance: " << (order_ok ? "OK" : "FAIL") << "\n";

    bool root_ok = multiple_root_invariance_test(base_graph);
    cout << "multiple-root invariance: " << (root_ok ? "OK" : "FAIL") << "\n";

    cout << "Summary: adj=" << adj_ok << " reduction_sp=" << red_sp
         << " label=" << lab_ok << " edge_order=" << order_ok << " root=" << root_ok << "\n";

    if (!(adj_ok && lab_ok && order_ok && root_ok)) {
        string fname = out_prefix + "_failed_graph.txt";
        ofstream fout(fname);
        if (fout) {
            finalize_graph_counts(const_cast<graph&>(base_graph)); // ensure e is correct in dump
            fout << base_graph.n << " " << base_graph.e << "\n";
            for (int u = 0; u < base_graph.n; ++u) {
                for (int v : base_graph.adjLists[u]) if (u < v) fout << u << " " << v << "\n";
            }
            fout.close();
            cerr << "[run_validation_tests_for_graph] dumped failing graph to " << fname << "\n";
        }
    }
}

void run_validation_tests_with_generator() {
    cout << "=== run_validation_tests_with_generator ===\n";
    // We'll run three types of generators:
    // 1) Mostly-cycle components (nC>0, nK=0) -> lots of cycles
    // 2) Mostly-clique components (nK>0) -> dense biconnected pieces
    // 3) three_edges=1 to produce 3-edge connections (more complex)
    vector<tuple<long,long,long,long,long>> params = {
        {10, 6, 0, 0, 0},  // many cycles of size 6
        {5, 3, 5, 4, 0},   // mix cycles and cliques
        {8, 4, 4, 3, 1}    // three edge attachments allowed
    };

    int idx = 0;
    for (auto &t : params) {
        long nC, lC, nK, lK, three;
        tie(nC,lC,nK,lK,three) = t;
        long seed = (long)chrono::high_resolution_clock::now().time_since_epoch().count() + idx*7919;
        graph g = generate_graph(nC, lC, nK, lK, three, seed);
        finalize_graph_counts(g);
        string prefix = "gen_test_" + to_string(idx);
        cout << "\n--- Test " << idx << " (nC="<<nC<<",lC="<<lC<<",nK="<<nK<<",lK="<<lK<<",three="<<three<<") ---\n";
        run_validation_tests_for_graph(g, prefix);
        ++idx;
    }

    {
        long nC = 30, lC = 4, nK = 0, lK = 0, three = 0;
        graph g = generate_graph(nC, lC, nK, lK, three, (long)chrono::high_resolution_clock::now().time_since_epoch().count() + 999);
        finalize_graph_counts(g);
        cout << "\n--- Large-ish test n="<<g.n<<" ---\n";
        run_validation_tests_for_graph(g, "large_gen_test");
    }
}
