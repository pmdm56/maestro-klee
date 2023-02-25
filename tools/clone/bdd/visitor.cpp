#include "visitor.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Clone {
	Visitor::Visitor() = default;
	Visitor::~Visitor() = default;

	BDDVisitor::Action Visitor::visitBranch(const Branch *node) {
		return Action::VISIT_CHILDREN;
	}

	BDDVisitor::Action Visitor::visitCall(const Call *node) {
		return Action::VISIT_CHILDREN;
	}

	BDDVisitor::Action Visitor::visitReturnInit(const ReturnInit *node) {
		return Action::STOP;
	}

	BDDVisitor::Action Visitor::visitReturnProcess(const ReturnProcess *node) {
		return Action::STOP;
	}

	BDDVisitor::Action Visitor::visitReturnRaw(const ReturnRaw *node) {
		return Action::STOP;
	}
	
	void Visitor::visitInitRoot(const Node *root) {
	}

	void Visitor::visitProcessRoot(const Node *root) {
	}

	void Visitor::visit(const Branch *node) {
	}

	void Visitor::visit(const Call *node) {
	}

	void Visitor::visit(const ReturnInit *node) {
	}

	void Visitor::visit(const ReturnProcess *node) {
	}

	void Visitor::visit(const ReturnRaw *node) {
	}

	void Visitor::visit(const KleeBDD::BDD &bdd) {
	}

}