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
		
		curr->update_id(counter++);

		switch(curr->get_type()) {
			case Node::NodeType::BRANCH: {
				auto branch { static_cast<Branch*>(curr.get()) };

				assert(branch->get_on_true());
				assert(branch->get_on_false());
				BDDNode_ptr next_true = branch->get_on_true()->clone();
				BDDNode_ptr next_false = branch->get_on_false()->clone();
				
				// for(auto &constraint: branch->get_constraints()) {
				// 	kutil::RetrieveSymbols retriever(true);
    			// 	retriever.visit(constraint);

				//     auto symbols = retriever.get_retrieved_strings();

				// 	if (symbols.size() != 1 || *symbols.begin() != "VIGOR_DEVICE") {
      			// 		continue;
    			// 	}
				// 	else {
				// 		info("Found constraint at node ", curr->get_id());
				// 		exit(1);
				// 	}
				// }

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
				BDDNode_ptr next { call->get_next()->clone() };

				curr->replace_next(next);
				next->replace_prev(curr);
				explore_node(next, constraints);
			}	
			case Node::NodeType::RETURN_INIT: {
				init_tails.insert(curr);
			}
			case Node::NodeType::RETURN_PROCESS: {
				process_tails.insert(curr);
			}
			case Node::NodeType::RETURN_RAW: {
				process_tails.insert(curr);
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
		return bdd->get_init() == nullptr;
	}

	bool Builder::is_process_empty() const {
		return bdd->get_process() == nullptr;
	}

	void Builder::join_bdd(const std::shared_ptr<const BDD::BDD> &other, vector<unsigned> &constraints) {
		debug("Joining BDDs");

		auto init { other->get_init()->clone() };
		auto process { other->get_process()->clone() };
		explore_node(init, constraints);
		explore_node(process, constraints);

		if(is_init_empty()) {
			bdd->set_init(init);
			assert(bdd->get_init() != nullptr);
		}
		else {
			/*auto &tail = *init_tails.begin();
			auto &prev = tail->get_prev();
		
			if(prev->get_type() == Node::NodeType::BRANCH) {
				auto branch { static_cast<Branch*>(prev.get()) };
				auto on_true = branch->get_on_true();
				auto on_false = branch->get_on_false();

				if(on_true->get_id() == tail->get_id()) {
					branch->replace_on_true(init);
				}
				else if (on_false->get_id() == tail->get_id()) {
					branch->replace_on_false(init);
				}
				else {
					assert(false);
				}

			}
			else {
				prev->replace_next(init);
			}
			init->replace_prev(prev);*/
		}

		if(is_process_empty()) {
			bdd->set_process(process);
			assert(bdd->get_process() != nullptr);
		}

		auto file = std::ofstream("output.gv");
      	assert(file.is_open());

		BDD::GraphvizGenerator gv(file);
		debug(bdd->get_number_of_nodes(bdd->get_process()) + bdd->get_number_of_nodes(bdd->get_init()));
		gv.visit(*bdd);

		exit(0);
	}

	const std::unique_ptr<BDD::BDD>& Builder::get_bdd() const {
		return this->bdd;
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}