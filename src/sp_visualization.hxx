#ifndef __SP_VISUALIZATION_HXX__
#define __SP_VISUALIZATION_HXX__

#include "graph.hxx"
#include "sp-tree.hxx"
#include <string>

void draw_graph_dot(graph const& g, const std::string &filename_png, int node_limit = 15);

void draw_decomposition_certificate_dot(gsp_sp_op_result const& result, const std::string &filename_png);

#endif 
