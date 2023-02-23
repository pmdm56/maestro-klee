#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include "call-paths-to-bdd.h"

namespace Clone {
	class NF;
	class Link;
	class Device;
	class Node;

	/** Typedefs **/

	/* Value: Link */
	typedef std::vector<std::unique_ptr<Link>> Links;
	
	/* Key: NF name/identifier; Value: NF */
	typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;

	/* Key: Device name/identifier; Value: Device */
	typedef std::unordered_map<std::string, std::unique_ptr<Device>> Devices;

	/* Key: path; Value: BDD */
	typedef std::unordered_map<std::string, std::shared_ptr<BDD::BDD>> BDDs;

	/* Key: Node name/identifier; Value: Node */
	typedef std::unordered_map<std::string, std::shared_ptr<Node>> Nodes;

	/* Set of nodes */
	typedef std::unordered_set<std::shared_ptr<Node>> NodeSet;

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
	
		Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds);
		
		void build_graph();
		void traverse_node(const std::shared_ptr<Node> &node, std::vector<unsigned> &input_ports);
		void traverse_all_sources();
		void print_graph() const;
	public:
		~Network();

		static std::unique_ptr<Network> create(Devices &&devices, NFs &&nfs, Links &&links);
		
		void consolidate();
		void print() const;
	};
}
