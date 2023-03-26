#include "network.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "bdd/visitors/graphviz-generator.h"
#include "call-paths-to-bdd.h"

#include "device.hpp"
#include "nf.hpp"
#include "link.hpp"
#include "node.hpp"
#include "../bdd/builder.hpp"
#include <cstdlib>

namespace Clone {

	/* Constructors and destructors */
	Network::Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds): 
						devices(move(devices)), nfs(move(nfs)), links(move(links)), bdds(move(bdds)) {}
	
	Network::~Network() = default;

	/* Private methods */
	void Network::build_graph() {
		for(auto &link: links) {
			const string &node1_str { link->get_node1() };
			const string &node2_str { link->get_node2() };

			const NodeType &node1_type { devices.find(node1_str) != devices.end() ? NodeType::DEVICE : NodeType::NF };
			const NodeType &node2_type { devices.find(node2_str) != devices.end() ? NodeType::DEVICE : NodeType::NF };

			const unsigned &port1 { link->get_port1() };
			const unsigned &port2 { link->get_port2() };

			if(nodes.find(node1_str) == nodes.end()) {
				nodes.emplace(node1_str, shared_ptr<Node>(new Node(node1_str, node1_type)));
			}

			if(nodes.find(node2_str) == nodes.end()) {
				nodes.emplace(node2_str, shared_ptr<Node>(new Node(node2_str, node2_type)));
			}

			const shared_ptr<Node> &node1 { nodes.at(node1_str) };
			const shared_ptr<Node> &node2 { nodes.at(node2_str) };

			node1->add_child(port1, port2, node2);
			
			if(node1_type == NodeType::DEVICE) {
				sources.push_back(make_pair(node1, port1));
			}

			if(node2_type == NodeType::DEVICE) {
				sinks.insert(node2);
			}
		}

		if(sources.size() == 0) {
			danger("No sources found");
		}

		if(sinks.size() == 0) {
			danger("No sinks found");
		}
	}

	void Network::traverse_all_flows() {
		const unique_ptr<Builder> builder { Builder::create() };
		
		info("Total of ", sources.size(), " sources");
		for(auto &source: sources) {			
			assert(source.first->get_node_type() == NodeType::DEVICE);
			info("Traversing source: ", source.first->get_name(), " port: ", source.second);
			explore_node(source.first, builder, 0);

			visited.clear();
		}

		const auto &bdd { builder->get_bdd().get() };
	}


	void Network::explore_node(const shared_ptr<Node> &node, const std::unique_ptr<Builder> &builder, unsigned input_port) {
		switch(node->get_node_type()) {
		case NodeType::DEVICE: {
			for(auto &p: node->get_children()) {
				const unsigned port_src { p.first };
				const unsigned port_dst { p.second.first };
				const auto& child { p.second.second };

				explore_node(child, builder, port_dst);
			}
			break;
		}
		case NodeType::NF: {
			assert(nfs.find(node->get_name()) != nfs.end());
			const auto& nf { nfs.at(node->get_name()) };

			assert(nf->get_bdd() != nullptr);
			const auto&bdd { nf->get_bdd() };

			if(!builder->is_init_merged(nf->get_id())) {
				builder->join_init(bdd, input_port);
				builder->add_merged_nf_init(nf->get_id());
			}
			debug("Input port for ", nf->get_id(), ": ", input_port);
			builder->join_process(bdd, input_port);

			BDD::GraphvizGenerator::visualize(*bdd, true, false);
			exit(0);
			break;
		}
		}
		visited.insert(node);
	}

	void Network::print_graph() const {
		for(auto &node: nodes) {
			node.second->print();
		}

		debug("Sources: ");
		for(auto &source: sources) {
			debug(source.first->get_name());
		}

		debug("Sinks: ");
		for(auto &sink: sinks) {
			debug(sink->get_name());
		}
	}

	/* Static methods */
 	unique_ptr<Network> Network::create(Devices &&devices, NFs &&nfs, Links &&links) {
		if(devices.size() == 0) danger("No devices found");
		if(nfs.size() == 0)	danger("No NFs found");
		if(links.size() == 0) danger("No links found");
		
		BDDs bdds;

		for (auto it = nfs.begin(); it != nfs.end(); ++it) {
			auto & nf { it->second };

			const string &path { nf->get_path() };

			if(bdds.find(path) == bdds.end()) {
				bdds.emplace(path, shared_ptr<BDD::BDD>(new BDD::BDD(path)));
			}

			nf->set_bdd(bdds.at(path));
		}
		
		return unique_ptr<Network>(new Network(move(devices), move(nfs), move(links), move(bdds)));
	}


	/* Public methods */
	void Network::consolidate() {
		build_graph();
		traverse_all_flows();
	}

	void Network::print() const {
		debug("Printing Network");

		for (auto it = devices.begin(); it != devices.end(); ++it) {
			it->second->print();
		}

		for (auto it = nfs.begin(); it != nfs.end(); ++it) {
			it->second->print();
		}

		for (auto& link : links) {
			link->print();
		}
	}
}
