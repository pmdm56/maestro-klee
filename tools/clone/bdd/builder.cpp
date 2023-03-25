#include "builder.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "klee/Constraints.h"
#include "solver_toolbox.h"

using namespace BDD;

namespace Clone {

	/* Constructors and destructors */

	Builder::Builder(unique_ptr<BDD::BDD> bdd): bdd(move(bdd)) {
	}

	Builder::~Builder() = default;

	/* Private methods */

	void Builder::trim_branch(BDD::BDDNode_ptr curr, BDD::BDDNode_ptr next) {
		auto prev = curr->get_prev();

		if(prev->get_type() == Node::NodeType::BRANCH) {
			auto branch { static_cast<Branch*>(prev.get()) };
			auto on_true = branch->get_on_true();
			auto on_false = branch->get_on_false();

			if (on_true->get_id() == curr->get_id()) {
				branch->replace_on_true(next);
			}
			else if (on_false->get_id() == curr->get_id()) {
				branch->replace_on_false(next);
			}
			else {
				danger("Could not trim branch ", prev->get_id(), " to leaf ", curr->get_id());
			}
		}
		else {
			prev->replace_next(next);
		}

		next->replace_prev(prev);
	}

	void Builder::explore_node(BDD::BDDNode_ptr curr, unsigned input_port) {
		assert(curr);
		
		curr->update_id(counter++);

		switch(curr->get_type()) {
			case Node::NodeType::BRANCH: {
				auto branch { static_cast<Branch*>(curr.get()) };

				assert(branch->get_on_true());
				assert(branch->get_on_false());
				BDDNode_ptr next_true = branch->get_on_true()->clone();
				BDDNode_ptr next_false = branch->get_on_false()->clone();
				
				const auto &condition_symbols { kutil::get_symbols(branch->get_condition()) };

				if (condition_symbols.size() == 1 && *condition_symbols.begin() == "VIGOR_DEVICE") {
				 	auto symbol { kutil::solver_toolbox.create_new_symbol("VIGOR_DEVICE", 32) };
				 	auto one { kutil::solver_toolbox.exprBuilder->Constant(0, symbol->getWidth()) };
				 	auto eq { kutil::solver_toolbox.exprBuilder->Eq(symbol, one) };
					auto cm = branch->get_constraints();
					cm.addConstraint(eq);
					branch->set_constraints(cm);
				}

				bool maybe_true = kutil::solver_toolbox.is_expr_maybe_true(branch->get_constraints(), branch->get_condition()) ;
				bool maybe_false = kutil::solver_toolbox.is_expr_maybe_false(branch->get_constraints(), branch->get_condition()) ;
				
				if(!maybe_true) {
					info("Trimming branch ", curr->get_id(), " to false branch");
					trim_branch(curr, next_false);
				}
				else if(!maybe_false) {
					info("Trimming branch ", curr->get_id(), " to true branch");
					trim_branch(curr, next_true);
				}
				else {
					branch->replace_on_true(next_true);
					next_true->replace_prev(curr);
					explore_node(next_true, input_port);

					branch->replace_on_false(next_false);
					next_false->replace_prev(curr);
					explore_node(next_false, input_port);
				}

				break;
			}
			case Node::NodeType::CALL: {
				auto call { static_cast<Call*>(curr.get()) };

				assert(call->get_next());
				BDDNode_ptr next { call->get_next()->clone() };

				curr->replace_next(next);
				next->replace_prev(curr);
				explore_node(next, input_port);
				break;
			}	
			case Node::NodeType::RETURN_INIT: {
				auto ret { static_cast<ReturnInit*>(curr.get()) };
				if(ret->get_return_value() == ReturnInit::ReturnType::SUCCESS) {
					init_tails.insert(curr);
				}
				break;
			}
			case Node::NodeType::RETURN_PROCESS: {
				auto ret { static_cast<ReturnProcess*>(curr.get()) };
				if(ret->get_return_value() == ReturnProcess::Operation::FWD) {
					process_tails.insert(curr);
				}
				break;
			}
			case Node::NodeType::RETURN_RAW: {
				auto ret { static_cast<ReturnRaw*>(curr.get()) };
				process_tails.insert(curr);
				break;
			}
		}
	}

	/* Static methods */

	std::unique_ptr<Builder> Builder::create() {
		auto builder { new Builder(unique_ptr<BDD::BDD>(new BDD::BDD())) };
		return std::unique_ptr<Builder>(move(builder));
	}

	/* Public methods */
	bool Builder::is_init_empty() const {
		return bdd->get_init() == nullptr;
	}

	bool Builder::is_process_empty() const {
		return bdd->get_process() == nullptr;
	}

	void Builder::join_bdd(const std::shared_ptr<const BDD::BDD> &other, unsigned input_port) {
		debug("Joining BDD");

		auto init { other->get_init()->clone() };
		auto process { other->get_process()->clone() };

		if(is_init_empty()) {
			bdd->set_init(init);
			assert(bdd->get_init() != nullptr);
		}

		if(is_process_empty()) {
			bdd->set_process(process);
			assert(bdd->get_process() != nullptr);
		}

		explore_node(init, input_port);
		explore_node(process, input_port);

		BDD::GraphvizGenerator::visualize(*bdd, true, false);
	}

	const std::unique_ptr<BDD::BDD>& Builder::get_bdd() const {
		return this->bdd;
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}