#include "builder.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"


namespace Clone {

	/* Constructors and destructors */

	Builder::Builder() = default;
	Builder::~Builder() = default;

	/* Public methods */
	bool Builder::is_empty() const {
		return true;
	}

	void Builder::append(const BDD::Node *node) {
		debug("Appending node");
	}
}