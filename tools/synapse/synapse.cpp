#include "klee/ExprBuilder.h"
#include "klee/perf-contracts.h"
#include "klee/util/ArrayCache.h"
#include "klee/util/ExprSMTLIBPrinter.h"
#include "klee/util/ExprVisitor.h"
#include "llvm/Support/CommandLine.h"

#include <klee/Constraints.h>
#include <klee/Solver.h>

#include <algorithm>
#include <chrono>
#include <dlfcn.h>
#include <expr/Parser.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stack>
#include <utility>
#include <vector>

#include "call-paths-to-bdd.h"
#include "load-call-paths.h"

#include "code_generator.h"
#include "execution_plan/execution_plan.h"
#include "execution_plan/visitors/graphviz/graphviz.h"
#include "heuristics/heuristics.h"
#include "log.h"
#include "search.h"

namespace {
llvm::cl::list<std::string> InputCallPathFiles(llvm::cl::desc("<call paths>"),
                                               llvm::cl::Positional);

llvm::cl::OptionCategory SyNAPSE("SyNAPSE specific options");

llvm::cl::list<synapse::TargetType> TargetList(
    llvm::cl::desc("Available targets:"), llvm::cl::Required,
    llvm::cl::OneOrMore,
    llvm::cl::values(
        clEnumValN(synapse::TargetType::x86_BMv2, "x86-bmv2",
                   "x86 controller for BMv2 (C)"),
        clEnumValN(synapse::TargetType::BMv2, "bmv2",
                   "BMv2 Lab 5: Apoio ao projeto -- mE1(P4)"),
        clEnumValN(synapse::TargetType::FPGA, "fpga", "FPGA (veriLog)"),
        clEnumValN(synapse::TargetType::Netronome, "netronome",
                   "Netronome (micro C)"),
        clEnumValN(synapse::TargetType::Tofino, "tofino", "Tofino (P4)"),
        clEnumValN(synapse::TargetType::x86_Tofino, "x86-tofino",
                   "x86 controller for Tofino (C)"),
        clEnumValEnd),
    llvm::cl::cat(SyNAPSE));

llvm::cl::opt<std::string>
    InputBDDFile("in", llvm::cl::desc("Input file for BDD deserialization."),
                 llvm::cl::cat(SyNAPSE));

llvm::cl::opt<std::string>
    Out("out", llvm::cl::desc("Output directory for every generated file."),
        llvm::cl::cat(SyNAPSE));

llvm::cl::opt<int> MaxReordered(
    "max-reordered",
    llvm::cl::desc(
        "Maximum number of reordenations on the BDD (-1 for unlimited)."),
    llvm::cl::Optional, llvm::cl::init(-1), llvm::cl::cat(SyNAPSE));

llvm::cl::opt<bool> ShowEP("s", llvm::cl::desc("Show winner Execution Plan."),
                           llvm::cl::ValueDisallowed, llvm::cl::init(false),
                           llvm::cl::cat(SyNAPSE));

llvm::cl::opt<bool> ShowSS("ss", llvm::cl::desc("Show the entire search space."),
                           llvm::cl::ValueDisallowed, llvm::cl::init(false),
                           llvm::cl::cat(SyNAPSE));

llvm::cl::opt<bool> Verbose("v", llvm::cl::desc("Verbose mode."),
                            llvm::cl::ValueDisallowed, llvm::cl::init(false),
                            llvm::cl::cat(SyNAPSE));
} // namespace

BDD::BDD build_bdd() {
  assert((InputBDDFile.size() != 0 || InputCallPathFiles.size() != 0) &&
         "Please provide either at least 1 call path file, or a bdd file");

  if (InputBDDFile.size() > 0) {
    return BDD::BDD(InputBDDFile);
  }

  std::vector<call_path_t *> call_paths;

  for (auto file : InputCallPathFiles) {
    std::cerr << "Loading: " << file << std::endl;

    call_path_t *call_path = load_call_path(file);
    call_paths.push_back(call_path);
  }

  return BDD::BDD(call_paths);
}

std::pair<synapse::ExecutionPlan, synapse::SearchSpace>
search(const BDD::BDD &bdd) {
  synapse::SearchEngine search_engine(bdd, MaxReordered);

  for (unsigned i = 0; i != TargetList.size(); ++i) {
    auto target = TargetList[i];
    search_engine.add_target(target);
  }

  synapse::Biggest biggest;
  synapse::DFS dfs;
  synapse::MostCompact most_compact;
  synapse::LeastReordered least_reordered;
  synapse::MaximizeSwitchNodes maximize_switch_nodes;

  // auto winner = search_engine.search(biggest);
  // auto winner = search_engine.search(least_reordered);
  // auto winner = search_engine.search(dfs);
  // auto winner = search_engine.search(most_compact);
  auto winner = search_engine.search(maximize_switch_nodes);
  const auto &ss = search_engine.get_search_space();

  return {winner, ss};
}

void synthesize(const synapse::ExecutionPlan &ep) {
  synapse::CodeGenerator code_generator(Out);

  for (unsigned i = 0; i != TargetList.size(); ++i) {
    auto target = TargetList[i];
    code_generator.add_target(target);
  }

  code_generator.generate(ep);
}

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  if (Verbose) {
    synapse::Log::MINIMUM_LOG_LEVEL = synapse::Log::Level::DEBUG;
  } else {
    synapse::Log::MINIMUM_LOG_LEVEL = synapse::Log::Level::LOG;
  }

  BDD::BDD bdd = build_bdd();

  auto start_search = std::chrono::steady_clock::now();
  auto search_results = search(bdd);
  auto end_search = std::chrono::steady_clock::now();

  auto search_dt = std::chrono::duration_cast<std::chrono::seconds>(
                       end_search - start_search)
                       .count();

  if (ShowEP) {
    synapse::Graphviz::visualize(search_results.first);
  }

  if (ShowSS) {
    synapse::Graphviz::visualize(search_results.second);
  }

  int64_t synthesis_dt = -1;

  if (Out.size()) {
    auto start_synthesis = std::chrono::steady_clock::now();
    synthesize(search_results.first);
    auto end_synthesis = std::chrono::steady_clock::now();

    synthesis_dt = std::chrono::duration_cast<std::chrono::seconds>(
                       end_synthesis - start_synthesis)
                       .count();
  }

  synapse::Log::log() << "Search time:     " << search_dt << " sec\n";

  if (synthesis_dt >= 0) {
    synapse::Log::log() << "Generation time: " << synthesis_dt << " sec\n";
  }

  return 0;
}
