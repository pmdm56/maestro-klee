#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include "../models/device.hpp"
#include "../models/nf.hpp"
#include "../models/link.hpp"
#include "../models/port.hpp"
#include "../bdd/builder.hpp"
#include "bdd/nodes/branch.h"
#include "bdd/nodes/node.h"
#include "bdd/nodes/return_process.h"
#include "node.hpp"

namespace BDD {
	class BDD;
}

namespace Clone {
	class Node;

	using std::string;
	using std::vector;
	using std::shared_ptr;
	using std::unique_ptr;
	using std::unordered_set;
	using std::unordered_map;
	using std::shared_ptr;
	using BDD::BDDNode_ptr;
	using BDD::Branch;
	using BDD::ReturnProcess;


	typedef unordered_map<string, shared_ptr<const BDD::BDD>> BDDs;

	struct NodeTransition {
		unsigned input_port;
		NodePtr node;
		BDDNode_ptr tail;

		NodeTransition(unsigned input_port, const NodePtr &node, BDDNode_ptr tail)
			: input_port(input_port), node(node), tail(tail) {}
	};

	typedef shared_ptr<NodeTransition> NodeTransitionPtr;

	class Network {
	private:
		const NFMap nfs;
		const LinkList links;
		const DeviceMap devices;
		const PortMap ports;

		NodeMap nodes;
		NodeSet sources;

		shared_ptr<Builder> builder;

		Network(DeviceMap &&devices, NFMap &&nfs, LinkList &&links, PortMap &&ports);

		void build_graph();
		void explore_source(const NodePtr &origin);
		void traverse(unsigned device_port, NodePtr origin, unsigned nf_port);
		void print_graph() const;
	public:
		~Network();

		static unique_ptr<Network> create(DeviceMap &&devices, NFMap &&nfs, LinkList &&links, PortMap &&ports);

		void consolidate();
		void print() const;
	};
}
