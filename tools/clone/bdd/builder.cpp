#include "builder.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "bdd/nodes/node.h"
#include "klee/Constraints.h"
#include "solver_toolbox.h"

using namespace BDD;

namespace Clone {

	/* Constructors and destructors */

	Builder::Builder(unique_ptr<BDD::BDD> bdd): bdd(move(bdd)) {
	}

	Builder::~Builder() = default;

	/* Private methods */

	void Builder::clone_node(BDD::BDDNode_ptr root, unsigned input_port) {
		assert(root);

		deque<BDDNode_ptr> q;
		stack<BDDNode_ptr> s;

		s.push(root);

		while(!s.empty()) {
			auto curr = s.top();
			s.pop();

			curr->update_id(curr->get_id() + 1);

			switch(curr->get_type()) {
				case Node::NodeType::BRANCH: {
					auto branch { static_cast<Branch*>(curr.get()) };
					assert(branch->get_on_true());
					assert(branch->get_on_false());

					auto prev = branch->get_prev();
					auto next_true = branch->get_on_true()->clone();
					auto next_false = branch->get_on_false()->clone();
					branch->disconnect();
					branch->add_prev(prev);

					const auto &condition_symbols { kutil::get_symbols(branch->get_condition()) };

					if (condition_symbols.size() == 1 && *condition_symbols.begin() == "VIGOR_DEVICE") {
						auto vigor_device { kutil::solver_toolbox.create_new_symbol("VIGOR_DEVICE", 32) };
						auto port { kutil::solver_toolbox.exprBuilder->Constant(input_port, vigor_device->getWidth()) };
						auto eq { kutil::solver_toolbox.exprBuilder->Eq(vigor_device, port) };
						auto cm = branch->get_node_constraints();
						cm.addConstraint(eq);
						branch->set_constraints(cm);
					}
					bool maybe_true = kutil::solver_toolbox.is_expr_maybe_true(branch->get_constraints(), branch->get_condition()) ;
					bool maybe_false = kutil::solver_toolbox.is_expr_maybe_false(branch->get_constraints(), branch->get_condition()) ;
					
					if(!maybe_true) {
						info("Trimming branch ", curr->get_id(), " to false branch");
						trim_branch(curr, next_false);
						s.push(next_false);
					}
					else if(!maybe_false) {
						info("Trimming branch ", curr->get_id(), " to true branch");
						trim_branch(curr, next_true);
						s.push(next_true);
					}
					else {
						branch->add_on_true(next_true);
						next_true->replace_prev(curr);
						s.push(next_true);

						branch->add_on_false(next_false);
						next_false->replace_prev(curr);
						s.push(next_false);
					}

					break;
				}
				case Node::NodeType::CALL: {
					auto call { static_cast<Call*>(curr.get()) };
					assert(call->get_next());

					auto next = call->get_next()->clone();
					call->replace_next(next);
					next->replace_prev(curr);
					s.push(next);

					break;
				}
				case Node::NodeType::RETURN_INIT: {
					auto ret { static_cast<ReturnInit*>(curr.get()) };

					break;
				}
				case Node::NodeType::RETURN_PROCESS: {
					auto ret { static_cast<ReturnProcess*>(curr.get()) };

					if(ret->get_return_operation() == ReturnProcess::Operation::FWD) {
						debug("Found a process tail ", ret->get_id());
						q.push_back(curr);
					}

					break;
				}
				case Node::NodeType::RETURN_RAW: {
					auto ret { static_cast<ReturnRaw*>(curr.get()) };
					
					break;
				}
			}
		}

		//root->recursive_update_ids(++counter);
	}

	void Builder::trim_branch(BDD::BDDNode_ptr curr, BDD::BDDNode_ptr next) {
		auto prev = curr->get_prev();

		if(prev->get_type() == Node::NodeType::BRANCH) {
			auto branch { static_cast<Branch*>(prev.get()) };
			auto on_true = branch->get_on_true();
			auto on_false = branch->get_on_false();

			if (on_true->get_id() == curr->get_id()) {
				branch->replace_on_true(next);
				next->replace_prev(prev);
				assert(branch->get_on_true()->get_prev()->get_id() == prev->get_id());
			}
			else if (on_false->get_id() == curr->get_id()) {
				branch->replace_on_false(next);
				next->replace_prev(prev);
				assert(branch->get_on_false()->get_prev()->get_id() == prev->get_id());
			}
			else {
				danger("Could not trim branch ", prev->get_id(), " to leaf ", curr->get_id());
			}
		}
		else {
			prev->replace_next(next);
			next->replace_prev(prev);
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

	bool Builder::is_init_merged(const std::string &nf_id) const {
		return merged_nf_inits.find(nf_id) != merged_nf_inits.end();
	}

	void Builder::add_merged_nf_init(const std::string &nf_id) {
		merged_nf_inits.insert(nf_id);
	}

	void Builder::join_init(const std::shared_ptr<const BDD::BDD> &other, unsigned input_port) {
		auto initRoot = other->get_init()->clone();

		if(is_init_empty()) {
			bdd->set_init(initRoot);
			assert(bdd->get_init() != nullptr);
		}

		clone_node(initRoot, input_port);
	}

	void Builder::join_process(const std::shared_ptr<const BDD::BDD> &other, unsigned input_port) {
		auto processRoot = other->get_process()->clone();

		if(is_process_empty()) {
			bdd->set_process(processRoot);
			assert(bdd->get_process() != nullptr);
		}

		clone_node(processRoot, input_port);
	}

	const std::unique_ptr<BDD::BDD>& Builder::get_bdd() const {
		return this->bdd;
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}