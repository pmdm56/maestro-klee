#pragma once

#include <string>
#include <memory>

#include "../models/network.hpp"
#include "call-paths-to-bdd.h"

namespace Synergio {
	namespace Loader {
		/**
		 * Loads a BDD from a file.
		 * 
		 * @param path The path to the file.
		 * @return The BDD object.
		*/
		std::unique_ptr<const BDD::BDD> load(const std::string &path);
	}
}
