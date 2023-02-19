#include "visitor.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Synergio {
	Visitor::Visitor() = default;

	/* Protected methods */

	Action Visitor::visitBranch(const BDD::Branch *node) {
		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitCall(const BDD::Call *node) {
		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitReturnInit(const BDD::ReturnInit *node) {
		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitReturnProcess(const BDD::ReturnProcess *node) {
		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitReturnRaw(const BDD::ReturnRaw *node) {
		return Action::VISIT_CHILDREN;
	}
}