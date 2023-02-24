#include "bdd.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"


namespace Clone {
	BDD::BDD(const string &path) {
		bdd = unique_ptr<KleeBDD::BDD>(new KleeBDD::BDD(path));
	}

	void BDD::init(int constraint) {
		bdd->get_init();
	}

	void BDD::process(int constraint) {

	}
}