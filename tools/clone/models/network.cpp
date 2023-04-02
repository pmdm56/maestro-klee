#include "network.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "bdd/nodes/node.h"
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

	void Network::explore_all_sources(const shared_ptr<Node> &origin) {
		assert(origin->get_node_type() == NodeType::DEVICE);

		for(auto &child: origin->get_children()) {
			traverse(child.second.first, child.second.second);
		}
	}

	void Network::traverse(unsigned input_port, const std::shared_ptr<Node> &origin) {
		/* Input port | Node */
		assert(nfs.find(origin->get_name()) != nfs.end());
		const auto &nf = nfs.at(origin->get_name());
		assert(nf->get_bdd() != nullptr);
		const auto &bdd = nf->get_bdd();

		builder->add_process_branch(input_port);

		deque<pair<pair<unsigned, const shared_ptr<NF> &>, Tails>> q_roots;

		auto p = make_pair(input_port, nf);

		Tails tails;
		tails.push_front(builder->get_process_root());
		q_roots.push_back(make_pair(p, tails));

		while(!q_roots.empty()) {
			const unsigned port = q_roots.front().first.first;
			const auto &nf = q_roots.front().first.second;
			const auto &roots = q_roots.front().second;
			q_roots.pop_front();

			builder->join_init(nf);
			Tails tails = builder->join_process(nf, port, roots);

			while(!tails.empty()) {
				auto &tail = tails.front();
				tails.pop_front();
				auto return_process = static_cast<BDD::ReturnProcess*>(tail.get());
				unsigned port_next = return_process->get_return_value();

				
			}
		}
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
			auto &nf { it->second };

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
				
		info("Total of ", sources.size(), " sources");
		for(auto &source: sources) {			
			assert(source.first->get_node_type() == NodeType::DEVICE);
			info("Traversing source: ", source.first->get_name(), " port: ", source.second);
			explore_all_sources(source.first);
			visited.clear();
		}

		const auto &bdd { builder->get_bdd().get() };
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
