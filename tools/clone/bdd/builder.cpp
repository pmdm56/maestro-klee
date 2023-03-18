#include "builder.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "klee/Constraints.h"

using namespace BDD;

namespace Clone {

	/* Constructors and destructors */

	Builder::Builder(unique_ptr<BDD::BDD> bdd): bdd(move(bdd)) {
		debug("Builder created");
	}

	Builder::~Builder() = default;

	/* Private methods */

	void Builder::explore_node(BDD::BDDNode_ptr curr, vector<unsigned> &constraints) {
		assert(curr);
		
		switch(curr->get_type()) {
			case Node::NodeType::BRANCH: {
				auto branch { static_cast<Branch*>(curr.get()) };

				assert(branch->get_on_true());
				assert(branch->get_on_false());
				BDDNode_ptr next_true = branch->get_on_true()->clone();
				BDDNode_ptr next_false = branch->get_on_false()->clone();

				branch->replace_on_true(next_true);
				next_true->replace_prev(curr);
				explore_node(next_true, constraints);

				branch->replace_on_false(next_false);
				next_false->replace_prev(curr);
				explore_node(next_false, constraints);
				
			}
			case Node::NodeType::CALL: {
				auto call { static_cast<Call*>(curr.get()) };

				assert(call->get_next());
				BDDNode_ptr next = call->get_next()->clone();

				curr->replace_next(next);
				next->replace_prev(curr);
				explore_node(next, constraints);
			}	
			case Node::NodeType::RETURN_INIT: {

			}
			case Node::NodeType::RETURN_PROCESS: {
				
			}
			case Node::NodeType::RETURN_RAW: {
				
			}
		}
	}

	/* Static methods */

	std::unique_ptr<Builder> Builder::create() {
		debug("Creating builder");
		auto builder { new Builder(unique_ptr<BDD::BDD>(new BDD::BDD())) };
		return std::unique_ptr<Builder>(move(builder));
	}

	/* Public methods */
	bool Builder::is_init_empty() const {
		return this->init_tails.empty();
	}

	bool Builder::is_process_empty() const {
		return this->process_tails.empty();
	}

	void Builder::join_bdd(const std::shared_ptr<const BDD::BDD> &other, vector<unsigned> &constraints) {
		debug("Joining BDDs");

		auto init { other->get_init()->clone() };
		auto process { other->get_process()->clone() };
		
		explore_node(init, constraints);
		explore_node(process, constraints);

		BDD::PrinterDebug printer;
		printer.visitInitRoot(init.get());
		printer.visitProcessRoot(process.get());
	}

	const std::unique_ptr<BDD::BDD>& Builder::get_bdd() const {
		return this->bdd;
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}