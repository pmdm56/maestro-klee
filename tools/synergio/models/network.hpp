#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

#include "call-paths-to-bdd.h"

namespace Synergio {
	class Device;
	class NF;
	class Link;

	typedef std::unordered_map<std::string, std::unique_ptr<Device>> Devices;
	typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;
	typedef std::vector<std::unique_ptr<Link>> Topology;

	class Network {
	private:
		Devices devices;
		NFs nfs;
		Topology topology;

		std::unordered_map<std::string, std::unique_ptr<BDD::BDD>> bdds;

	public:
		Network(Devices &&devices, NFs &&nfs, Topology &&links);
		~Network();

		void load();
		void print();
	};
}
