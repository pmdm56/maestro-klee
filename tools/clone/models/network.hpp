#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "../bdd/builder.hpp"

namespace BDD {
	class BDD;
}

namespace Clone {
	class NF;
	class Link;
	class Device;
	class Node;

	typedef std::vector<std::unique_ptr<const Link>> Links;
	typedef std::unordered_map<std::string, std::shared_ptr<NF>> NFs;
	typedef std::unordered_map<std::string, std::unique_ptr<const Device>> Devices;

	typedef std::unordered_map<std::string, std::shared_ptr<const BDD::BDD>> BDDs;

	typedef std::shared_ptr<Node> NodePtr;
	typedef std::unordered_set<NodePtr> NodeSet;
	typedef std::unordered_map<std::string, NodePtr> NodeMap;


	struct NodeTransition {
		unsigned input_port;
		NodePtr node;
		BDD::BDDNode_ptr tail;

		NodeTransition(unsigned input_port, std::shared_ptr<Node> node, BDD::BDDNode_ptr tail)
			: input_port(input_port), node(node), tail(tail) {}
	};

	class Network {
	private:
		const NFs nfs;
		const Links links;
		const Devices devices;

		NodeMap nodes;
		NodeSet sources;

		const std::unique_ptr<Builder> builder = Builder::create();

		Network(Devices &&devices, NFs &&nfs, Links &&links);
		
		void build_graph();
		void explore_source(const std::shared_ptr<Node> &origin);
		void traverse(unsigned device_port, const std::shared_ptr<Node> &origin, unsigned nf_port);
		void print_graph() const;
	public:
		~Network();

		static std::unique_ptr<Network> create(Devices &&devices, NFs &&nfs, Links &&links);
		
		void consolidate();
		void print() const;
	};
}
