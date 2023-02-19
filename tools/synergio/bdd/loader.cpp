#include "loader.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"


namespace Synergio {
	namespace Loader {
		unique_ptr<const BDD::BDD> load(const std::string &path) {
			unique_ptr<const BDD::BDD> bdd = unique_ptr<BDD::BDD>(new BDD::BDD(path));

			info("Loaded BDD from ", path, " with ID ", bdd->get_id());

			return bdd;	
		}
	}
}
