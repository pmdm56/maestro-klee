#include "builder.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "call-paths-to-bdd.h"

namespace Clone {

	/* Constructors and destructors */

	Builder::Builder(): bdd(nullptr) {
		debug("Builder created");
	}

	Builder::~Builder() = default;

	/* Public methods */
	bool Builder::is_empty() const {
		return bdd == nullptr;
	}

	void Builder::init() {
		bdd = unique_ptr<const BDD::BDD>(new BDD::BDD(0));

	}

	void Builder::append(const BDD::Node *node) {
		debug("Appending node");
	}
}