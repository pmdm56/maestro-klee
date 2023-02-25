#include "nf.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "../bdd/bdd.hpp"

namespace Clone {
	NF::NF(const std::string &id, const std::string &path) : id(id), path(path) {}

	NF::~NF() {}

	string NF::get_id() const {
		return id;
	}

	string NF::get_path() const {
		return path;
	}

	shared_ptr<BDD> NF::get_bdd() const {
		return bdd;
	}

	void NF::set_bdd(shared_ptr<BDD> bdd) {
		this->bdd = bdd;
	}

	void NF::print() {
		debug("NF ", id, " from ", path);
	}
}
