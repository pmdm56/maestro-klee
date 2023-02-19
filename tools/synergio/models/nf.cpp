#include "nf.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "call-paths-to-bdd.h"

namespace Synergio {
	NF::NF(const std::string &id, const std::string &path) : id(id), path(path) {}

	NF::~NF() {}

	string NF::get_id() const {
		return id;
	}

	string NF::get_path() const {
		return path;
	}

	void NF::set_bdd(unique_ptr<BDD::BDD> bdd) {
		this->bdd = move(bdd);
	}

	void NF::print() {
		debug("NF ", id, " from ", path);
	}
}
