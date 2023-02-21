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

	typedef std::unordered_set<std::shared_ptr<Node>> NodeSet;

	/**
	 * This class represents a network.
	 * 
	 * It contains all the devices, NFs and links.
	*/
	class Network {
	private:
		enum class NodeType {
			DEVICE,
			NF
		};

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
	
		Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds);

		NodeType get_node_type(const std::string &node_str) const;
		
		void build_graph();
		void print_graph() const;
	public:
		~Network();

		static std::unique_ptr<Network> create(Devices &&devices, NFs &&nfs, Links &&links);
		
		void consolidate();
		void print() const;
	};
}
