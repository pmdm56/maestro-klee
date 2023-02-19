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
		NFs nfs;
		BDDs bdds;
		Devices devices;
		Links links;

		Network(Devices &&devices, NFs &&nfs, Links &&links, BDDs &&bdds);
	public:
		~Network();

		static std::unique_ptr<Network> create(Devices &&devices, NFs &&nfs, Links &&links);

		void print();
	};
}
