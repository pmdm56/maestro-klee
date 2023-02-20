#include "network.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "device.hpp"
#include "nf.hpp"
#include "link.hpp"

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

	void Network::consolidate_device_to_device(const string &device_from, const unsigned port_from, const string &device_to, const unsigned port_to) {
		info("Consolidating device to device");
	}

	void Network::consolidate_nf_to_device(const string &nf_from, const unsigned port_from, const string &device_to, const unsigned port_to) {
		info("Consolidating NF to device");

		const auto &nf = nfs.at(nf_from);
		const auto &device = devices.at(device_to);

		const auto &bdd = nf->get_bdd();

		auto init = bdd->get_init();

		Visitor visitor();

	}

	void Network::consolidate_device_to_nf(const string &device_from, const unsigned port_from, const string &nf_to, const unsigned port_to) {
		info("Consolidating device to NF");
	}

	void Network::consolidate_nf_to_nf(const string &nf_from, const unsigned port_from, const string &nf_to, const unsigned port_to) {
		info("Consolidating NF to NF");
	}

	/* Static methods */
 	std::unique_ptr<Network> Network::create(Devices &&devices, NFs &&nfs, Links &&links) {
		info("Loading all BDDs");

		BDDs bdds;

		// Loading all BDDs onto a map where the key is the path to the BDD file and the value is the BDD itself
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

		return std::unique_ptr<Network>(new Network(move(devices), move(nfs), move(links), move(bdds)));
	}


	/* Public methods */
	void Network::consolidate() {
		info("Starting network consolidation");

		const unique_ptr<BDD::BDD> global_bdd { nullptr };

		if(links.size() == 0) {
			danger("No links found");
		}

		for(auto &link: links) {
			const string &node1_str = link->get_node1();
			const string &node2_str = link->get_node2();

			const NodeType node1_type = get_node_type(node1_str);
			const NodeType node2_type = get_node_type(node2_str);

			const unsigned port1 = link->get_port1();
			const unsigned port2 = link->get_port2();

			if(node1_type == NodeType::DEVICE && node2_type == NodeType::DEVICE) {
				consolidate_device_to_device(node1_str, port1, node2_str, port2);
			}
			else if(node1_type == NodeType::NF && node2_type == NodeType::NF) {
				consolidate_nf_to_nf(node1_str, port1, node2_str, port2);
			}
			else if(node1_type == NodeType::DEVICE && node2_type == NodeType::NF) {
				consolidate_device_to_nf(node1_str, port1, node2_str, port2);
			}
			else if(node1_type == NodeType::NF && node2_type == NodeType::DEVICE) {
				consolidate_nf_to_device(node1_str, port1, node2_str, port2);
			}
			else {
				danger("Should never happen");
			}
		}

		success("Network consolidated");
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
