#ifndef __SP_VALIDATION_TESTS_HXX__
#define __SP_VALIDATION_TESTS_HXX__

#include "graph.hxx"
#include <string>


void run_validation_tests_for_graph(graph const& base_graph, const std::string &out_prefix);

void run_validation_tests_with_generator();

#endif
