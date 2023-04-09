#include "nf.hpp"
#include "../pch.hpp"

#include "call-paths-to-bdd.h"

namespace Clone {
	NF::NF(const std::string &id, const std::string &path) : id(id), path(path) {

	}

	NF::~NF() {
		
	}

	string NF::get_id() const {
		return id;
	}

	string NF::get_path() const {
		return path;
	}

	shared_ptr<const BDD::BDD> NF::get_bdd() const {
		return bdd;
	}

	void NF::set_bdd(shared_ptr<const BDD::BDD> bdd) {
		this->bdd = bdd;
	}

	void NF::print() const {
		debug("NF ", id, " from ", path);
	}
}
