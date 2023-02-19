#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

#include "call-paths-to-bdd.h"

namespace Synergio {
	class NF;
	class Link;
	class Device;

	typedef std::vector<std::unique_ptr<Link>> Links;
	typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;
	typedef std::unordered_map<std::string, std::shared_ptr<BDD::BDD>> BDDs;
	typedef std::unordered_map<std::string, std::unique_ptr<Device>> Devices;

	class Network {
	private:
		const NFs nfs;
		const BDDs bdds;
		const Devices devices;
		const Links links;

		enum class NodeType {
			DEVICE,
			NF
		};

		Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds);

		NodeType get_node_type(const std::string &node_str) const;

		void consolidate_device_to_device(const std::string &device_from, const unsigned port_from, const std::string &device_to, const unsigned port_to);
		void consolidate_nf_to_device(const std::string &nf_from, const unsigned port_from, const std::string &device_to, const unsigned port_to);
		void consolidate_device_to_nf(const std::string &device_from, const unsigned port_from, const std::string &nf_to, const unsigned port_to);
		void consolidate_nf_to_nf(const std::string &nf_from, const unsigned port_from, const std::string &nf_to, const unsigned port_to);
	public:
		~Network();

		static std::unique_ptr<Network> create(Devices &&devices, NFs &&nfs, Links &&links);

		void consolidate();

		void print() const;
	};
}
