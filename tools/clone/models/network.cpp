#include "network.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "device.hpp"
#include "nf.hpp"
#include "link.hpp"
#include "node.hpp"

#include "../bdd/bdd.hpp"

namespace Clone {

	/* Constructors and destructors */
	Network::Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds): devices(move(devices)), nfs(move(nfs)), links(move(links)), bdds(move(bdds)) {}
	
	Network::~Network() = default;

	/* Private methods */

	void Network::build_graph() {
		for(auto &link: links) {
			const string &node1_str = link->get_node1();
			const string &node2_str = link->get_node2();

			const NodeType node1_type = devices.find(node1_str) != devices.end() ? NodeType::DEVICE : NodeType::NF;
			const NodeType node2_type = devices.find(node2_str) != devices.end() ? NodeType::DEVICE : NodeType::NF;

			const unsigned port1 = link->get_port1();
			const unsigned port2 = link->get_port2();

			if(nodes.find(node1_str) == nodes.end()) {
				nodes.emplace(node1_str, shared_ptr<Node>(new Node(node1_str, node1_type)));
			}

			if(nodes.find(node2_str) == nodes.end()) {
				nodes.emplace(node2_str, shared_ptr<Node>(new Node(node2_str, node2_type)));
			}

			shared_ptr<Node> node1 = nodes.at(node1_str);
			shared_ptr<Node> node2 = nodes.at(node2_str);

			node1->add_child(port1, port2, node2);
			
			if(node1_type == NodeType::DEVICE) {
				sources.insert(node1);
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

	void Network::traverse_node(const shared_ptr<Node> &node, int constraint) {
		if(visited.find(node) != visited.end()) {
			return;
		}

		/* Only process BDD if it's a Network Function */
		if(node->get_node_type() == NodeType::NF) {
			auto &nf =nfs.at(node->get_name());
			nf->traverse_bdd(constraint);
		}

		visited.insert(node);

		for(auto &p: node->get_children()) {
			unsigned port_src = p.first;
			unsigned port_dst = p.second.first;
			auto &child = p.second.second;

			traverse_node(child, port_dst);
		}
	}
	
	void Network::traverse_all_sources() {
		for(auto &source: sources) {
			traverse_node(source, NO_CONSTRAINT);

			visited.clear();
		}
	}


	void Network::print_graph() const {
		for(auto &node: nodes) {
			node.second->print();
		}

		debug("Sources: ");
		for(auto &source: sources) {
			debug(source->get_name());
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
			auto& nf = it->second;

			const string &path = nf->get_path();

			if(bdds.find(path) == bdds.end()) {
				bdds.emplace(path, shared_ptr<BDD>(BDD::create(path)));
			}

			nf->set_bdd(bdds.at(path));
		}
		
		return unique_ptr<Network>(new Network(move(devices), move(nfs), move(links), move(bdds)));
	}


	/* Public methods */
	void Network::consolidate() {
		build_graph();
		traverse_all_sources();
	}

	void Network::print() const {
		debug("Printing Network");

		for (auto it = devices.begin(); it != devices.end(); ++it) {
			auto& device = it->second;
			device->print();
		}

		for (auto it = nfs.begin(); it != nfs.end(); ++it) {
			auto& nf = it->second;
			nf->print();
		}

		for (auto& link : links) {
			link->print();
		}
	}
}
