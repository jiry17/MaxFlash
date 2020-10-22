//
// Created by pro on 2020/1/21.
//

#include "context.h"
#include "specification.h"
#include "specification_parser.h"
#include "minimal_context_graph.h"
#include "solver.h"

#include <ctime>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>

DEFINE_string(spec, "", "The path of the specification");
DEFINE_string(model,  "", "The path of the probabilistic model");
DEFINE_string(oup, "", "The path of the output file");
DEFINE_string(log, "", "The path of the log file");
DEFINE_string(type, "string", "The type of the benchmark (string/matrix)");

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    std::string spec_file = FLAGS_spec;
    std::string model_file = FLAGS_model;
    std::string output_file = FLAGS_oup;
    std::string log_file = FLAGS_log;
    std::string benchmark_type = FLAGS_type;
    if (benchmark_type == "matrix") {
        global::isMatrix = true;
    }

    if (!log_file.empty()) {
        google::SetLogDestination(google::GLOG_INFO, log_file.c_str());
    } else {
        FLAGS_logtostderr = true;
    }

    LOG(INFO) << "Parsing the specification from " << spec_file << " with type " << benchmark_type << std::endl;
    Specification* spec = parser::loadSpecification(spec_file, benchmark_type);
    LOG(INFO) << "Finished. Example num: " << spec->example_space.size() << std::endl;

    LOG(INFO) << "Parsing the topdown prediction model from " << model_file << std::endl;
    auto* info_map = new ContextInfoMap(model_file);
    LOG(INFO) << "Finished. Context num: " << info_map->info_list.size() << std::endl;

    LOG(INFO) << "Building the transition graph of contexts and calculating the initial heuristic value" << std::endl;
    auto* graph = new MinimalContextGraph(spec->start_terminal, new TopDownContextMaintainer(), info_map);
    LOG(INFO) << "Finished" << std::endl;

    LOG(INFO) << "Synthesizing" << std::endl;
    auto start_time = clock();
    SynthesisTask task(graph, spec);
    auto* result = task.solve();
    double time_cost = (clock() - start_time) * 1.0 / CLOCKS_PER_SEC;
    LOG(INFO) << "Result: " << result->toString() << std::endl;
    LOG(INFO) << "Time Cost: " << time_cost << std::endl;
    if (!output_file.empty()) {
        auto *F = std::fopen(output_file.c_str(), "w");
        fprintf(F, "%s\n", result->toString().c_str());
        fprintf(F, "%.10lf\n", time_cost);
    } else if (!log_file.empty()) {
        std::cout << "Result: " << result->toString() << std::endl;
        std::cout << "Time Cost: " << time_cost << std::endl;
    }
} 

