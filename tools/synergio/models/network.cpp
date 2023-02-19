#include "network.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "device.hpp"
#include "nf.hpp"
#include "link.hpp"

#include "call-paths-to-bdd.h"

namespace Synergio {
	Network::Network(Devices &&devices, NFs &&nfs, Topology &&topology): devices(move(devices)), nfs(move(nfs)), topology(move(topology)) {}

	Network::~Network() = default;

	void Network::load() {
		info("Loading all BDDs");

		for (auto it = nfs.begin(); it != nfs.end(); ++it) {
			auto& nf = it->second;

			if(bdds.find(nf->get_path()) == bdds.end()) {
				bdds.emplace(nf->get_path(), unique_ptr<BDD::BDD>(new BDD::BDD(nf->get_path())));
			}
		}
		
		success("BDDs loaded");
	}

	void Network::print() {
		debug("Printing Network");

		for (auto it = devices.begin(); it != devices.end(); ++it) {
			auto& device = it->second;
			device->print();
		}

		for (auto it = nfs.begin(); it != nfs.end(); ++it) {
			auto& nf = it->second;
			nf->print();
		}

		for (auto& link : topology) {
			link->print();
		}
	}
}
