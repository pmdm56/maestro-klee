#include "builder.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "call-paths-to-bdd.h"

namespace Clone {

	/* Constructors and destructors */

	Builder::Builder() : bdd(nullptr) {
		debug("Builder created");
	}

	Builder::~Builder() = default;

	/* Public methods */
	bool Builder::is_empty() const {
		return bdd == nullptr;
	}

	void Builder::init(const BDD::BDD *bdd) {
		debug("Initializing BDD");
		this->bdd = unique_ptr<BDD::BDD>(new BDD::BDD(*bdd));
	}

	void Builder::append(BDD::Node *node) {
		debug("Appending node");
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}