#include "network.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

//#include "call-paths-to-bdd.h"

#include "device.hpp"
#include "nf.hpp"
#include "link.hpp"
#include "node.hpp"
#include "../bdd/builder.hpp"

namespace Clone {
	/* Constructors and destructors */
	Network::Network(Devices &&devices, NFs &&nfs, Links &&links): 
						devices(move(devices)), nfs(move(nfs)), links(move(links)) {}
	
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
				sources.insert(node1);
			}
		}

		if(sources.size() == 0) {
			danger("No sources found");
		}
	}

	void Network::explore_source(const shared_ptr<Node> &origin) {
		assert(origin->get_node_type() == NodeType::DEVICE);

		for(auto &child: origin->get_children()) {
			info("Traversing from ", origin->get_name(), " port ", child.first, " to ", child.second.second->get_name(), " port ", child.second.first);
			traverse(child.first, child.second.second, child.second.first);
		}
	}

	void Network::traverse(unsigned device_port, const std::shared_ptr<Node> &origin, unsigned nf_port) {
		/* Input port | Node */
		assert(nfs.find(origin->get_name()) != nfs.end());
		const auto &nf = nfs.at(origin->get_name());

		builder->add_process_branch(device_port);

		deque<shared_ptr<NodeTransition>> q_transitions;
		const auto &root = builder->get_process_root();
		const auto &branch = static_cast<BDD::Branch*>(root.get());
		q_transitions.push_front(std::make_shared<NodeTransition>(nf_port, origin, branch->get_on_true()));

		while(!q_transitions.empty()) {
			//debug("Queue size: ", q_transitions.size());
			const unsigned port = q_transitions.front()->input_port;
			const auto node = q_transitions.front()->node;
			const auto tail = q_transitions.front()->tail;
			//();
			assert(nfs.find(node->get_name()) != nfs.end());
			const auto nf = nfs.at(node->get_name());
			assert(nf != nullptr);

			q_transitions.pop_front();

			builder->join_init(nf);
			Tails tails = builder->join_process(nf, port, tail);

			while(!tails.empty()) {
				auto &tail = tails.front();
				tails.pop_front();
				auto return_process = static_cast<BDD::ReturnProcess*>(tail.get());
				unsigned port_next = return_process->get_return_value();
				const auto &child = node->get_child(port_next);

				if(child.second != nullptr) {
					const unsigned port = child.first;
					const auto &node = child.second;

					if(node->get_node_type() == NodeType::NF) {
						q_transitions.push_front(make_shared<NodeTransition>(port, node, tail));
					} 
					else {
						// then the packet just returns ?
					}
				} 
				else {
					builder->replace_with_drop(tail);
					// replace with drop
				}
			}

		}
		auto file = std::ofstream("graph.gv");
		BDD::GraphvizGenerator g(file);
		g.visualize(*builder->get_bdd(), false, false);
	}

	void Network::print_graph() const {
		for(auto &node: nodes) {
			node.second->print();
		}

		debug("Sources: ");
		for(auto &source: sources) {
			debug(source->get_name());
		}
	}

	/* Static methods */
 	unique_ptr<Network> Network::create(Devices &&devices, NFs &&nfs, Links &&links) {
		if(devices.size() == 0) danger("No devices found");
		if(nfs.size() == 0)	danger("No NFs found");
		if(links.size() == 0) danger("No links found");
		
		unordered_map<string, shared_ptr<const BDD::BDD>> bdds;

		for (auto it = nfs.begin(); it != nfs.end(); ++it) {
			auto &nf { it->second };

			const string &path { nf->get_path() };

			if(bdds.find(path) == bdds.end()) {
				bdds.emplace(path, shared_ptr<BDD::BDD>(new BDD::BDD(path)));
			}

			nf->set_bdd(bdds.at(path));
		}
		
		return unique_ptr<Network>(new Network(move(devices), move(nfs), move(links)));
	}


	/* Public methods */
	void Network::consolidate() {
		build_graph();
				
		for(auto &source: sources) {			
			assert(source->get_node_type() == NodeType::DEVICE);
			info("Traversing source: ", source->get_name());
			explore_source(source);
		}
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
