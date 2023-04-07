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

	/** Typedefs **/

	typedef std::shared_ptr<Node> NodePtr;
	/* Value: Link */
	typedef std::vector<std::unique_ptr<const Link>> Links;
	
	/* Key: NF name/identifier; Value: NF */
	typedef std::unordered_map<std::string, std::shared_ptr<NF>> NFs;

	/* Key: Device name/identifier; Value: Device */
	typedef std::unordered_map<std::string, std::unique_ptr<const Device>> Devices;

	/* Key: path; Value: BDD */
	typedef std::unordered_map<std::string, std::shared_ptr<const BDD::BDD>> BDDs;

	/* Key: Node name/identifier; Value: Node */
	typedef std::unordered_map<std::string, NodePtr> Nodes;

	typedef std::unordered_set<NodePtr> NodeSet;


	struct NodeTransition {
		unsigned input_port;
		NodePtr node;
		BDD::BDDNode_ptr tail;

		NodeTransition(unsigned input_port, std::shared_ptr<Node> node, BDD::BDDNode_ptr tail)
			: input_port(input_port), node(node), tail(tail) {}
	};

	/**
	 * This class represents a network.
	 * 
	 * It contains all the devices, NFs and links.
	*/
	class Network {
	private:
		const NFs nfs;
		const Links links;
		const Devices devices;
		const BDDs bdds;

		/* All the nodes in the network */
		Nodes nodes;

		/* Nodes where traffic can start */
		NodeSet sources;

		/* Nodes where traffic can end */
		NodeSet sinks;

		NodeSet visited;

		const std::unique_ptr<Builder> builder = Builder::create();

	
		Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds);
		
		void build_graph();
		void explore_source(const std::shared_ptr<Node> &origin);
		void traverse(unsigned input_port, const std::shared_ptr<Node> &origin);
		void print_graph() const;
	public:
		~Network();

		static std::unique_ptr<Network> create(Devices &&devices, NFs &&nfs, Links &&links);
		
		void consolidate();
		void print() const;
	};
}
