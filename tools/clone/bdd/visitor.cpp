#include "visitor.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Clone {
	/* Constructors and destructors */

	Visitor::Visitor() = default;
	Visitor::~Visitor() = default;


	/* Protected methods */

	void Visitor::visitInitRoot(const Node *root) {
		root->visit(*this);
	}

	void Visitor::visitProcessRoot(const Node *root) {
		root->visit(*this);
	}

	Action Visitor::visitBranch(const Branch *node) {

		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitCall(const Call *node) {
		return Action::VISIT_CHILDREN;
	}

	Action Visitor::visitReturnInit(const ReturnInit *node) {
		return Action::STOP;
	}

	Action Visitor::visitReturnProcess(const ReturnProcess *node) {
		return Action::STOP;
	}

	Action Visitor::visitReturnRaw(const ReturnRaw *node) {
		return Action::STOP;
	}


	/* Public methods */

	void Visitor::visit(const KleeBDD &bdd)  {
		info("Visiting init root");
		
		assert(bdd.get_init() != nullptr);
		visitInitRoot(bdd.get_init().get());
	}

	void Visitor::visit(const Branch *node) {
		info("Visiting branch");
		BDDVisitor::visit(node);
	}

	void Visitor::visit(const Call *node) {
		info("Visiting call");
		BDDVisitor::visit(node);
	}

	void Visitor::visit(const ReturnInit *node) {
		info("Visiting return init");
		BDDVisitor::visit(node);
	}

	void Visitor::visit(const ReturnProcess *node) {
		info("Visiting return process");
		BDDVisitor::visit(node);
	}

	void Visitor::visit(const ReturnRaw *node) {
		info("Visiting return raw");
		BDDVisitor::visit(node);
	}
}