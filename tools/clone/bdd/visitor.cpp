#include "visitor.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "solver-toolbox.h"
#include "klee/Constraints.h"
#include "call-paths-to-bdd.h"
#include "klee/Constraints.h"
#include "klee/ExprBuilder.h"

#include "builder.hpp"

using BDD::solver_toolbox;

namespace Clone {
	/* Constructors and destructors */
	Visitor::Visitor(vector<unsigned> &constraints, const unique_ptr<Builder> &builder) : 
						constraints(constraints), builder(builder) {		
	}

	Visitor::~Visitor() = default;


	/* Protected methods */

	void Visitor::visitInitRoot(const BDD::Node *root) {
		info("Visiting init root");

		
		root->visit(*this);
	}

	void Visitor::visitProcessRoot(const BDD::Node *root) {
		info("Visiting process root");

		root->visit(*this);
	}

	BDD::BDDVisitor::Action Visitor::visitBranch(const BDD::Branch *node) {
		info("Visiting branch");

		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action Visitor::visitCall(const BDD::Call *node) {
		info("Visiting call");

		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnInit(const BDD::ReturnInit *node) {
		info("Visiting return init");
		return Action::STOP;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnProcess(const BDD::ReturnProcess *node) {
		info("Visiting return process");
		return Action::STOP;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnRaw(const BDD::ReturnRaw *node) {
		info("Visiting return raw");
		return Action::STOP;
	}


	/* Public methods */

	void Visitor::visit(const BDD::BDD &bdd)  {
		info("Visiting BDD init");
		
		if(builder->is_empty()) {
			builder->init(&bdd);
		}

		assert(bdd.get_init() != nullptr);
		const auto &initRoot = bdd.get_init().get(); 

		visitInitRoot(initRoot);
	}
}