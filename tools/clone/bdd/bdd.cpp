#include "bdd.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "visitor.hpp"

namespace Clone {
	/* Constructors and destructors */

	BDD::BDD(unique_ptr<KleeBDD> bdd) : bdd(move(bdd)) {}

	BDD::~BDD() = default;


	/* Static methods */

	unique_ptr<BDD> BDD::create(const std::string &path) {
		return unique_ptr<BDD>(new BDD(unique_ptr<KleeBDD>(new KleeBDD(path)))); 
	}
	

	/* Public methods*/
	
	void BDD::visit(Visitor &visitor) const {
		auto kbdd = this->bdd.get();
		visitor.visit(*kbdd);
	}
}
