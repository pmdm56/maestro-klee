#include "visitor.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Clone {
	Visitor::Visitor() = default;

	/* Protected methods */

	BDD::BDDVisitor::Action Visitor::visitBranch(const BDD::Branch *node) {
		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action  Visitor::visitCall(const BDD::Call *node) {
		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action  Visitor::visitReturnInit(const BDD::ReturnInit *node) {
		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action  Visitor::visitReturnProcess(const BDD::ReturnProcess *node) {
		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action  Visitor::visitReturnRaw(const BDD::ReturnRaw *node) {
		return Action::VISIT_CHILDREN;
	}
}