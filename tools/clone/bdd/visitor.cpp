#include "visitor.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "call-paths-to-bdd.h"

#include "builder.hpp"

namespace Clone {
	enum Part {
		INIT,
		PROCESS
	} part;

	/* Constructors and destructors */
	Visitor::Visitor(vector<unsigned> &constraints, const unique_ptr<Builder> &builder) : 
						constraints(constraints), builder(builder) {		
	}

	Visitor::~Visitor() = default;


	/* Protected methods */

	void Visitor::visitInitRoot(const BDD::Node *root) {
		info("Visiting init root");
		
		part = INIT;
		if(builder->is_init_empty()) {
			info("Init is empty");
			builder->populate_init(root->clone());
			//root->get_next()->visit(*this);
		}
		else {
			//root->visit(*this);
		}
	}

	void Visitor::visitProcessRoot(const BDD::Node *root) {
		info("Visiting process root");

		part = PROCESS;
		if(builder->is_process_empty()) {
			info("Process is empty");
			builder->populate_process(root->clone());
			//root->get_next()->visit(*this);
		}
		else {
			//root->visit(*this);
		}
	}

	BDD::BDDVisitor::Action Visitor::visitBranch(const BDD::Branch *node) {
		info("Visiting branch");

		const auto condition { node->get_condition() };

		switch(part) {
			case INIT: {
				builder->append_init(node->clone());
			}
			case PROCESS: {
				builder->append_process(node->clone());
			}
		}
		
		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action Visitor::visitCall(const BDD::Call *node) {
		info("Visiting call");

		switch(part) {
			case INIT: {
				builder->append_init(node->clone());
			}
			case PROCESS: {
				builder->append_process(node->clone());
			}
		}

		return Action::VISIT_CHILDREN;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnInit(const BDD::ReturnInit *node) {
		info("Visiting return init");

		builder->append_init(node->clone());
		return Action::STOP;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnProcess(const BDD::ReturnProcess *node) {
		info("Visiting return process");
		builder->append_process(node->clone());
		return Action::STOP;
	}

	BDD::BDDVisitor::Action Visitor::visitReturnRaw(const BDD::ReturnRaw *node) {
		info("Visiting return raw");
		switch(part) {
			case INIT: {
				builder->append_init(node->clone());
			}
			case PROCESS: {
				builder->append_process(node->clone());
			}
		}
		return Action::STOP;
	}

	/* Public methods */

	void Visitor::visit(const BDD::BDD &bdd)  {
		info("Visiting BDD init");
		
		assert(bdd.get_init() != nullptr);
		const auto &initRoot { bdd.get_init().get() }; 

		visitInitRoot(initRoot);

		assert(bdd.get_process() != nullptr);
		const auto &processRoot { bdd.get_process().get() }; 

		visitProcessRoot(processRoot);
	}
}