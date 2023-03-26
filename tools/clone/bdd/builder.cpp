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

	unsigned Builder::clone_node(BDD::BDDNode_ptr root, unsigned input_port) {
		assert(root);

		stack<BDDNode_ptr> s;

		s.push(root);

		while(!s.empty()) {
			auto curr = s.top();
			s.pop();

			curr->update_id(counter++);

			switch(curr->get_type()) {
				case Node::NodeType::BRANCH: {
					auto branch { static_cast<Branch*>(curr.get()) };
					assert(branch->get_on_true());
					assert(branch->get_on_false());

					auto next_true = branch->get_on_true()->clone();
					auto next_false = branch->get_on_false()->clone();
					branch->set_on_true(nullptr);
					branch->set_on_false(nullptr);
					branch->replace_on_true(next_true);
					branch->replace_on_false(next_false);
					s.push(next_true);
					s.push(next_false);

					break;
				}
				case Node::NodeType::CALL: {
					auto call { static_cast<Call*>(curr.get()) };
					assert(call->get_next());

					auto next = call->get_next()->clone();
					call->replace_next(next);
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
						return ret->get_return_value();
					}

					break;
				}
				case Node::NodeType::RETURN_RAW: {
					auto ret { static_cast<ReturnRaw*>(curr.get()) };
					
					break;
				}
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

	unsigned Builder::join_process(const std::shared_ptr<const BDD::BDD> &other, unsigned input_port) {
		auto processRoot = other->get_process()->clone();

		if(is_process_empty()) {
			bdd->set_process(processRoot);
			assert(bdd->get_process() != nullptr);
		}

		return clone_node(processRoot, input_port);
	}

	const std::unique_ptr<BDD::BDD>& Builder::get_bdd() const {
		return this->bdd;
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}