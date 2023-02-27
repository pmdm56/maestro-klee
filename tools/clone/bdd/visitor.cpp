#include "visitor.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "klee/Constraints.h"
#include "call-paths-to-bdd.h"

#include "bdd-builder.hpp"

namespace Clone {
	/* Constructors and destructors */

	Visitor::Visitor() = default;
	Visitor::Visitor(vector<unsigned> &constraints) : constraints(constraints) {}
	Visitor::~Visitor() = default;


	/* Protected methods */

	void Visitor::visitInitRoot(const BDD::Node *root) {
		debug("Visiting init root");

		// TODO: add constraints (or not)
		root->visit(*this);
	}

	void Visitor::visitProcessRoot(const BDD::Node *root) {
		debug("Visiting process root");

		// TODO: add constraints
		root->visit(*this);
	}

	BDD::BDDVisitor::Action Visitor::visitBranch(const BDD::Branch *node) {
		debug("Visiting branch");
		//node->clone();

		auto &constraints_list = node->get_constraints();

		for(auto &constraints: constraints_list) {
			for(auto &constraint: constraints) {

			}
		}

		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action Visitor::visitCall(const BDD::Call *node) {
		debug("Visiting call");

		info(node->get_call().function_name); // TODO: check if its a function that reads input (?)


		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnInit(const BDD::ReturnInit *node) {
		debug("Visiting return init");
		return Action::STOP;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnProcess(const BDD::ReturnProcess *node) {
		debug("Visiting return process");
		return Action::STOP;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnRaw(const BDD::ReturnRaw *node) {
		debug("Visiting return raw");
		return Action::STOP;
	}


	/* Public methods */

	void Visitor::visit(const BDD::BDD &bdd)  {
		info("Visiting BDD init");
		assert(bdd.get_init() != nullptr);
		const auto &initRoot = bdd.get_init().get(); 
		visitInitRoot(initRoot);
	}
}