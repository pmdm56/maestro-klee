#pragma once

#include "../../../heuristics/heuristic.h"
#include "../../../log.h"
#include "../../../search_space.h"
#include "../../execution_plan.h"
#include "../../modules/modules.h"
#include "../visitor.h"

#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#define VISIT_PRINT_MODULE_NAME(M)                                             \
  void visit(const M *node) override {                                         \
    function_call(node->get_target(), node->get_target_name(),                 \
                  node->get_name());                                           \
  }

namespace synapse {

class Graphviz : public ExecutionPlanVisitor {
private:
  std::ofstream ofs;
  std::string fpath;

  const SearchSpace *search_space;
  std::string search_space_fpath;
  std::vector<std::string> bdd_fpaths;

  std::map<TargetType, std::string> node_colors;

  constexpr static int fname_len = 15;
  constexpr static const char *prefix = "/tmp/";
  constexpr static const char *alphanum = "0123456789"
                                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                          "abcdefghijklmnopqrstuvwxyz";

  std::string get_rand_fname() const {
    std::stringstream ss;
    static unsigned counter = 1;

    ss << prefix;

    srand((unsigned)std::time(NULL) * getpid() + counter);

    for (int i = 0; i < fname_len; i++) {
      ss << alphanum[rand() % (strlen(alphanum) - 1)];
    }

    ss << ".gv";

    counter++;
    return ss.str();
  }

  void open() {
    std::string file_path = __FILE__;
    std::string dir_path = file_path.substr(0, file_path.rfind("/"));
    std::string script = "open_graph.sh";
    std::string cmd = dir_path + "/" + script + " " + fpath;

    static int counter = 0;

    for (auto bdd_fpath : bdd_fpaths) {
      cmd += " " + bdd_fpath;
      counter++;
    }

    if (search_space) {
      cmd += " " + search_space_fpath;
    }

    system(cmd.c_str());
  }

public:
  Graphviz(const std::string &path, const SearchSpace *_search_space)
      : fpath(path), search_space(_search_space) {
    node_colors = std::map<TargetType, std::string>{
        {TargetType::BMv2, "darkolivegreen2"},
        {TargetType::Tofino, "cornflowerblue"},
        {TargetType::Netronome, "gold"},
        {TargetType::FPGA, "coral1"},
        {TargetType::x86_BMv2, "firebrick2"},
        {TargetType::x86_Tofino, "firebrick2"},
    };

    ofs.open(fpath);
    assert(ofs);
  }

  Graphviz(const std::string &path) : Graphviz(path, nullptr) {}

private:
  Graphviz() : Graphviz(get_rand_fname()) {}
  Graphviz(const SearchSpace *_search_space)
      : Graphviz(get_rand_fname(), _search_space) {
    search_space_fpath = get_rand_fname();
  }

  void function_call(TargetType target, std::string target_name,
                     std::string label) {
    assert(node_colors.find(target) != node_colors.end());
    ofs << "[label=\"" << /*target_name << "::" << */ label << "\", ";
    ofs << "color=" << node_colors[target] << "];";
    ofs << "\n";
  }

  struct rgb_t {
    int r;
    int g;
    int b;
  };

  rgb_t get_color(float f) const {
    rgb_t rgb;

    // float to RGB colormap : long rainbow
    // source: https://www.particleincell.com/2014/colormap/

    float group, color_value;
    color_value = 255 * modf(f * 5.0f, &group);

    int int_group = (int)group;
    int int_color_value = (int)color_value;

    switch (int_group) {
    case 0:
      rgb.r = 255;
      rgb.g = int_color_value;
      rgb.b = 0;
      break;
    case 1:
      rgb.r = 255 - int_color_value;
      rgb.g = 255;
      rgb.b = 0;
      break;
    case 2:
      rgb.r = 0;
      rgb.g = 255;
      rgb.b = int_color_value;
      break;
    case 3:
      rgb.r = 0;
      rgb.g = 255 - int_color_value;
      rgb.b = 255;
      break;
    case 4:
      rgb.r = int_color_value;
      rgb.g = 0;
      rgb.b = 255;
      break;
    case 5:
      rgb.r = 255;
      rgb.g = 0;
      rgb.b = 255;
      break;
    }

    return rgb;
  }

  void dump_bdd(const BDD::BDD &bdd,
                const std::unordered_set<uint64_t> &processed,
                const BDD::Node *next) {
    std::string leaf_fpath = get_rand_fname();
    bdd_fpaths.push_back(leaf_fpath);
    std::ofstream leaf_ofs;

    leaf_ofs.open(leaf_fpath);

    leaf_ofs << "digraph bdd_next {\n";
    leaf_ofs << "layout=\"dot\";\n";
    // leaf_ofs << "ratio=\"fill\";\n";
    // leaf_ofs << "size=\"12,12!\";\n";
    // leaf_ofs << "margin=0;\n";
    leaf_ofs << "node [shape=box,style=filled];\n";

    BDD::GraphvizGenerator bdd_graphviz(leaf_ofs, processed, next);

    assert(bdd.get_process());
    bdd.get_process()->visit(bdd_graphviz);
    leaf_ofs << "}\n";

    leaf_ofs.flush();
    leaf_ofs.close();
  }

  std::string get_bdd_node_name(const BDD::Node *node) const {
    assert(node);
    std::stringstream ss;

    switch (node->get_type()) {
    case BDD::Node::NodeType::BRANCH: {
      auto branch = static_cast<const BDD::Branch *>(node);
      ss << "if(";
      ss << kutil::expr_to_string(branch->get_condition(), true);
      ss << ")";
      break;
    }
    case BDD::Node::NodeType::CALL: {
      auto call = static_cast<const BDD::Call *>(node);
      ss << call->get_call().function_name;
      int i = 0;
      for (auto arg : call->get_call().args) {
        if (i > 0) {
          ss << ", ";
        }
        ss << kutil::expr_to_string(arg.second.expr, true);
        i++;
      }
      break;
    }
    case BDD::Node::NodeType::RETURN_PROCESS: {
      auto return_process = static_cast<const BDD::ReturnProcess *>(node);

      switch (return_process->get_return_operation()) {
      case BDD::ReturnProcess::Operation::BCAST: {
        ss << "broadcast()";
        break;
      }
      case BDD::ReturnProcess::Operation::DROP: {
        ss << "drop()";
        break;
      }
      case BDD::ReturnProcess::Operation::FWD: {
        ss << "forward(";
        ss << return_process->get_return_value();
        ss << ")";
        break;
      }
      default:
        assert(false);
      }

      break;
    }
    case BDD::Node::NodeType::RETURN_INIT:
      Log::err() << "return init\n";
      [[fallthrough]];
    case BDD::Node::NodeType::RETURN_RAW:
      Log::err() << "return raw\n";
      assert(false);
    }

    return ss.str();
  }

  void dump_search_space() const {
    assert(search_space);
    assert(search_space->get_root());

    std::ofstream search_space_ofs;

    search_space_ofs.open(search_space_fpath);

    search_space_ofs << "digraph SearchSpace {\n";
    search_space_ofs << "layout=\"twopi\";";
    // search_space_ofs << "ratio=\"fill\";\n";
    // search_space_ofs << "size=\"12,12!\";\n";
    // search_space_ofs << "margin=0;\n";
    search_space_ofs << "node [shape=ellipse,style=filled];\n";

    std::vector<search_space_node_t *> nodes;
    nodes.push_back(search_space->get_root().get());

    while (nodes.size()) {
      auto node = nodes[0];
      nodes.erase(nodes.begin());

      search_space_ofs << node->execution_plan_id;
      // search_space_ofs << " [color=\"#";
      // auto color = get_color(node->score);
      // search_space_ofs << std::setw(2) << std::setfill('0') << std::hex;
      // search_space_ofs << color.r;
      // search_space_ofs << std::setw(2) << std::setfill('0') << std::hex;
      // search_space_ofs << color.g;
      // search_space_ofs << std::setw(2) << std::setfill('0') << std::hex;
      // search_space_ofs << color.b;
      // search_space_ofs << std::dec;
      // search_space_ofs << "\"";

      search_space_ofs << " [label=\"";
      search_space_ofs << node->score;
      search_space_ofs << "\"";

      if (node->m) {
        assert(node->m->get_node());
        search_space_ofs << ", tooltip=\""
                         << get_bdd_node_name(node->m->get_node().get())
                         << " -> " << node->m->get_target_name()
                         << "::" << node->m->get_name() << "\"";
        // search_space_ofs << ", label=\"" << node->m->get_target_name()
        //                  << "::" << node->m->get_name() << "\"";
      }
      search_space_ofs << "];\n";

      if (node->prev) {
        search_space_ofs << node->prev->execution_plan_id << " -> "
                         << node->execution_plan_id << ";\n";
      }

      for (auto leaf : node->space) {
        nodes.push_back(leaf.get());
      }
    }

    search_space_ofs << "}\n";

    search_space_ofs.close();
  }

public:
  static void visualize(const ExecutionPlan &ep, bool interrupt = true) {
    if (ep.get_root()) {
      Graphviz gv;
      ep.visit(gv);
      gv.open();

      if (interrupt) {
        std::cout << "Press Enter to continue ";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    }
  }

  static void visualize(const ExecutionPlan &ep, SearchSpace &_search_space,
                        bool interrupt = true) {
    if (!ep.get_root()) {
      return;
    }

    Graphviz gv(&_search_space);
    ep.visit(gv);
    gv.open();

    if (interrupt) {
      std::cout << "\nPress Enter to continue ";
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
  }

  ~Graphviz() { ofs.close(); }

  void visit(ExecutionPlan ep) override {
    ofs << "digraph ExecutionPlan {\n";
    // ofs << "ratio=\"fill\";\n";
    ofs << "layout=\"dot\";";
    // ofs << "size=\"12,12!\";\n";
    // ofs << "margin=0;\n";
    ofs << "node [shape=record,style=filled];\n";

    ExecutionPlanVisitor::visit(ep);

    ofs << "}\n";
    ofs.flush();

    auto bdd = ep.get_bdd();
    auto processed = ep.get_processed_bdd_nodes();
    const BDD::Node *next_node = nullptr;

    if (ep.get_next_node()) {
      next_node = ep.get_next_node().get();
    }

    bdd_fpaths.clear();
    dump_bdd(bdd, processed, next_node);

    if (search_space) {
      dump_search_space();
    }
  }

  void visit(const ExecutionPlanNode *ep_node) override {
    auto mod = ep_node->get_module();
    auto next = ep_node->get_next();
    auto id = ep_node->get_id();

    ofs << id << " ";
    ExecutionPlanVisitor::visit(ep_node);

    for (auto branch : next) {
      ofs << id << " -> " << branch->get_id() << ";"
          << "\n";
    }
  }

  void log(const ExecutionPlanNode *ep_node) const override {
    // do nothing
  }

  /********************************************
   *
   *                x86 BMv2
   *
   ********************************************/

  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::MapGet)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::CurrentTime)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::PacketBorrowNextChunk)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::PacketGetMetadata)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::PacketReturnChunk)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::If)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::Then)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::Else)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::Forward)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::Broadcast)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::Drop)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::ExpireItemsSingleMap)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::RteEtherAddrHash)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::DchainRejuvenateIndex)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::VectorBorrow)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::VectorReturn)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::DchainAllocateNewIndex)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::MapPut)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::PacketGetUnreadLength)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::SetIpv4UdpTcpChecksum)
  VISIT_PRINT_MODULE_NAME(targets::x86_bmv2::DchainIsIndexAllocated)

  /********************************************
   *
   *                   BMv2
   *
   ********************************************/

  VISIT_PRINT_MODULE_NAME(targets::bmv2::SendToController)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::Ignore)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::SetupExpirationNotifications)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::If)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::Then)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::Else)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::EthernetConsume)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::EthernetModify)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::TableLookup)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::IPv4Consume)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::IPv4Modify)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::TcpUdpConsume)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::TcpUdpModify)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::IPOptionsConsume)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::IPOptionsModify)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::Drop)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::Forward)
  VISIT_PRINT_MODULE_NAME(targets::bmv2::VectorReturn)

  /********************************************
   *
   *                  Tofino
   *
   ********************************************/

  VISIT_PRINT_MODULE_NAME(targets::tofino::Ignore)
  VISIT_PRINT_MODULE_NAME(targets::tofino::If)
  VISIT_PRINT_MODULE_NAME(targets::tofino::Then)
  VISIT_PRINT_MODULE_NAME(targets::tofino::Else)
  VISIT_PRINT_MODULE_NAME(targets::tofino::Forward)
  VISIT_PRINT_MODULE_NAME(targets::tofino::EthernetConsume)
  VISIT_PRINT_MODULE_NAME(targets::tofino::EthernetModify)
  VISIT_PRINT_MODULE_NAME(targets::tofino::TableLookup)
  VISIT_PRINT_MODULE_NAME(targets::tofino::Drop)
  VISIT_PRINT_MODULE_NAME(targets::tofino::SendToController)

  /********************************************
   *
   *                x86 Tofino
   *
   ********************************************/

  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::Ignore)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::CurrentTime)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::PacketParseCPU)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::PacketParseEthernet)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::If)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::Then)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::Else)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::Drop)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::PacketModifyEthernet)
  VISIT_PRINT_MODULE_NAME(targets::x86_tofino::ForwardThroughTofino)
};
} // namespace synapse
