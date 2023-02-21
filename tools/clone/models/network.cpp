#include "network.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "device.hpp"
#include "nf.hpp"
#include "link.hpp"
#include "node.hpp"

#include "../bdd/visitor.hpp"

#include "call-paths-to-bdd.h"

namespace Clone {

	/* Constructors and destructors */
	Network::Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds): devices(move(devices)), nfs(move(nfs)), links(move(links)), bdds(move(bdds)) {}
	
	Network::~Network() = default;

	/* Private methods */
	Network::NodeType Network::get_node_type(const string &node_str) const {
		if(devices.find(node_str) != devices.end()) {
			return NodeType::DEVICE;
		}
		else if(nfs.find(node_str) != nfs.end()) {
			return NodeType::NF;
		}
		else {
			throw runtime_error("Node " + node_str + " not found");
		}
	}

	void Network::build_graph() {
		for(auto &link: links) {
			const string &node1_str = link->get_node1();
			const string &node2_str = link->get_node2();

			const NodeType node1_type = get_node_type(node1_str);
			const NodeType node2_type = get_node_type(node2_str);

			const unsigned port1 = link->get_port1();
			const unsigned port2 = link->get_port2();

			if(nodes.find(node1_str) == nodes.end()) {
				nodes.emplace(node1_str, shared_ptr<Node>(new Node(node1_str)));
			}

			if(nodes.find(node2_str) == nodes.end()) {
				nodes.emplace(node2_str, shared_ptr<Node>(new Node(node2_str)));
			}

			shared_ptr<Node> node1 = nodes.at(node1_str);
			shared_ptr<Node> node2 = nodes.at(node2_str);

			node1->add_neighbour(port1, node2);
			
			if(node1_type == NodeType::DEVICE) {
				sources.insert(node1);
			}

			if(node2_type == NodeType::DEVICE) {
				sinks.insert(node2);
			}
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
		if(devices.size() == 0) {
			danger("No devices found");
		}

		if(nfs.size() == 0) {
			danger("No NFs found");
		}

		if(links.size() == 0) {
			danger("No links found");
		}

		info("Loading all BDDs");
		
		BDDs bdds;

		for (auto it = nfs.begin(); it != nfs.end(); ++it) {
			auto& nf = it->second;

			const string &path = nf->get_path();

			if(bdds.find(path) == bdds.end()) {
				bdds.emplace(path, shared_ptr<BDD::BDD>(new BDD::BDD(path)));
				info("BDD loaded for NF ", nf->get_id(), " at ", path);
			}
			else {
				info("BDD already loaded for NF ", nf->get_id(), " at ", path);
			}

			nf->set_bdd(bdds.at(path));
		}
		
		success("BDDs loaded");

		return unique_ptr<Network>(new Network(move(devices), move(nfs), move(links), move(bdds)));
	}


	/* Public methods */
	void Network::consolidate() {
		info("Starting network consolidation");

		build_graph();
		print_graph();

		success("Finished network consolidation");
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
