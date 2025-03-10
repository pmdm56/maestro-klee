#include "nf.hpp"
#include "../pch.hpp"

#include "call-paths-to-bdd.h"

namespace Clone {
	NF::NF(const string &id, const string &path) : id(id), path(path) {}

	NF::~NF() = default;

	void NF::print() const {
		debug("NF ", id, " from ", path);
	}
}
