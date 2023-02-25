#include "bdd.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "visitor.hpp"

namespace Clone {
	BDD::BDD(unique_ptr<KleeBDD::BDD> bdd) : bdd(move(bdd)) {}

	BDD::~BDD() = default;

	unique_ptr<BDD> BDD::create(const std::string &path) {
		return unique_ptr<BDD>(new BDD(unique_ptr<KleeBDD::BDD>(new KleeBDD::BDD(path))));
	}

	void BDD::init(int constraint) {
		const auto &node = bdd->get_init();
		//unique_ptr<Visitor> visitor(new Visitor());

		//visitor->visit(bdd.get())
	}

	void BDD::process(int constraint) {

	}
}