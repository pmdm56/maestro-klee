#include "builder.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"
#include "klee/Constraints.h"

#include "call-paths-to-bdd.h"

using namespace BDD;

namespace Clone {

	/* Constructors and destructors */

	Builder::Builder() : bdd(unique_ptr<BDD::BDD>(new BDD::BDD(0))) {
		debug("Builder created");
	}

	Builder::~Builder() = default;

	/* Public methods */
	bool Builder::is_init_empty() const {
		return bdd->get_init() == nullptr;
	}

	bool Builder::is_process_empty() const {
		return bdd->get_process() == nullptr;
	}


	void Builder::append(BDD::Node *node) {
		debug("Appending node");
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}