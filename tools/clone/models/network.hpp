#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

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

		Nodes nodes;
		
		enum class NodeType {
			DEVICE,
			NF
		};

		Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds);

		NodeType get_node_type(const std::string &node_str) const;
		
		void build_graph();
	public:
		~Network();

		static std::unique_ptr<Network> create(Devices &&devices, NFs &&nfs, Links &&links);
		
		void consolidate();
		void print() const;
	};
}
