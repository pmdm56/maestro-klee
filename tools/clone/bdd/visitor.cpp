#include "visitor.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Clone {
	/* Constructors and destructors */

	Visitor::Visitor() = default;
	Visitor::Visitor(vector<unsigned> &constraints) : constraints(constraints) {}
	Visitor::~Visitor() = default;


	/* Protected methods */

	void Visitor::visitInitRoot(const Node *root) {
		debug("Visiting init root");
		root->visit(*this);
	}

	void Visitor::visitProcessRoot(const Node *root) {
		debug("Visiting process root");
		root->visit(*this);
	}

	Action Visitor::visitBranch(const Branch *node) {
		debug("Visiting branch");
		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitCall(const Call *node) {
		debug("Visiting call");
		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitReturnInit(const ReturnInit *node) {
		debug("Visiting return init");
		return Action::STOP;
	}

	Action Visitor::visitReturnProcess(const ReturnProcess *node) {
		debug("Visiting return process");
		return Action::STOP;
	}

	Action Visitor::visitReturnRaw(const ReturnRaw *node) {
		debug("Visiting return raw");
		return Action::STOP;
	}


	/* Public methods */

	void Visitor::visit(const KleeBDD &bdd)  {
		assert(bdd.get_init() != nullptr);
		const auto &initRoot = bdd.get_init().get(); 
		
		visitInitRoot(initRoot);
	}
}