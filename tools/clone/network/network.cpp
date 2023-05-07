#include "network.hpp"
#include "../pch.hpp"

#include "bdd/nodes/node.h"
#include "call-paths-to-bdd.h"

#include "node.hpp"

namespace Clone {
	/* Constructors and destructors */
	Network::Network(DeviceMap &&devices, NFMap &&nfs, LinkList &&links, PortMap &&ports): 
						devices(move(devices)), nfs(move(nfs)), links(move(links)), ports(move(ports)) {}
	
	Network::~Network() = default;

	/* Private methods */
	void Network::build_graph() {
		for(const auto &link: links) {
			const string &node1_str = link->get_node1();
			const string &node2_str = link->get_node2();

			const NodeType node1_type = devices.find(node1_str) != devices.end() ? NodeType::DEVICE : NodeType::NF;
			const NodeType node2_type = devices.find(node2_str) != devices.end() ? NodeType::DEVICE : NodeType::NF;

			if(node1_type == NodeType::DEVICE && node2_type == NodeType::DEVICE) {
				continue;
			}

			const unsigned port1 = link->get_port1();
			const unsigned port2 = link->get_port2();

			if(nodes.find(node1_str) == nodes.end()) {
				nodes.emplace(node1_str, NodePtr(new Node(node1_str, node1_type)));
			}

			if(nodes.find(node2_str) == nodes.end()) {
				nodes.emplace(node2_str, NodePtr(new Node(node2_str, node2_type)));
			}

			const NodePtr node1 = nodes.at(node1_str);
			const NodePtr node2 = nodes.at(node2_str);

			node1->add_child(port1, port2, node2);
			
			if(node1_type == NodeType::DEVICE) {
				sources.insert(node1);
			}
		}

		if(sources.size() == 0) {
			danger("No sources found");
		}
	}

	void Network::explore_source(const NodePtr &origin) {
		assert(origin->get_node_type() == NodeType::DEVICE);

		for(const auto &child: origin->get_children()) {
			const unsigned local_port = child.first;
			info("Traversing from ", origin->get_name(), " port ", local_port, " to ", child.second.second->get_name(), " port ", child.second.first);
			assert(devices.find(origin->get_name()) != devices.end() && "Missing device port mapping");
			DevicePtr device = devices.at(origin->get_name());
			const int global_port = device->get_port(local_port);
			assert(global_port != -1 && "Missing local port mapping");
			traverse(global_port, child.second.second, child.second.first);
		}
	}

	void Network::traverse(unsigned global_port, NodePtr source, unsigned nf_port) {
		/* Input port | Node */
		assert(nfs.find(source->get_name()) != nfs.end());
		const NFPtr nf = nfs.at(source->get_name());
		
		builder->add_init_branch(global_port);
		builder->add_process_branch(global_port);

		deque<NodeTransitionPtr> q_transitions;
		const BDDNode_ptr root = builder->get_process_root();
		const Branch* branch = static_cast<Branch*>(root.get());
		const NodeTransitionPtr transition = make_shared<NodeTransition>(nf_port, source, branch->get_on_true());
		q_transitions.push_front(transition);

		while(!q_transitions.empty()) {
			const unsigned port = q_transitions.front()->input_port;
			const NodePtr node = q_transitions.front()->node;
			const BDDNode_ptr tail = q_transitions.front()->tail;
			assert(nfs.find(node->get_name()) != nfs.end());
			const NFPtr nf = nfs.at(node->get_name());
			assert(nf != nullptr);

			q_transitions.pop_front();

			builder->join_init(nf);
			Tails tails = builder->join_process(nf, port, tail);

			while(!tails.empty()) {
				const BDDNode_ptr tail = tails.front();
				tails.pop_front();
				ReturnProcess* return_process = static_cast<ReturnProcess*>(tail.get());
				unsigned port_next = return_process->get_return_value();

				if(node->has_child(port_next)) {
					const pair<unsigned, NodePtr> &child = node->get_child(port_next);
					const unsigned local_port = child.first;
					const NodePtr node = child.second;

					if(node->get_node_type() == NodeType::NF) {
						q_transitions.push_front(make_shared<NodeTransition>(local_port, node, tail));
					} 
					else {
						assert(devices.find(node->get_name()) != devices.end());
						const DevicePtr device = devices.at(node->get_name());
						const unsigned global_port = device->get_port(local_port);
						return_process->set_return_value(global_port);
					}
				} 
				else {
					builder->replace_with_drop(tail);
				}
			}
		}
		BDD::GraphvizGenerator::visualize(*builder->get_bdd(), false, false);
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
 	unique_ptr<Network> Network::create(DeviceMap &&devices, NFMap &&nfs, LinkList &&links, PortMap &&ports) {
		if(devices.size() == 0) danger("No devices found");
		if(nfs.size() == 0)	danger("No nfs found");
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
		
		return unique_ptr<Network>(new Network(move(devices), move(nfs), move(links), move(ports)));
	}

	/* Public methods */
	void Network::consolidate() {
		build_graph();

		builder = Builder::create();

		for(const shared_ptr<Node> &source: sources) {			
			assert(source->get_node_type() == NodeType::DEVICE);
			info("Traversing source: ", source->get_name());
			explore_source(source);
		}
		builder->dump("network.bdd");
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
