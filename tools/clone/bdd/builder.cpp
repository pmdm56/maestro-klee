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

	void Builder::explore_branch(BDDNode_ptr node, std::unordered_set<BDD::BDDNode_ptr> &tails) {
		if(node == nullptr) return;

		if(node->get_type() == Node::NodeType::BRANCH) {
			auto branch { static_cast<Branch*>(node.get()) };
			assert(branch->get_on_true() != nullptr);
			assert(branch->get_on_false() != nullptr);
			BDDNode_ptr true_branch { branch->get_on_true()->clone() };
			BDDNode_ptr false_branch { branch->get_on_false()->clone() };

			explore_branch(true_branch, tails);
			explore_branch(false_branch, tails);
		}
		else {
			tails.insert(node);
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

	void Builder::populate_init(BDD::BDDNode_ptr node) {
		debug("Initializing init");
		node->disconnect();
		this->bdd->set_init(node);
		init_tails.insert(node);
	}

	void Builder::populate_process(BDD::BDDNode_ptr node) {
		debug("Initializing process");
		node->disconnect();
		this->bdd->set_process(node);
		process_tails.insert(node);
	}
	

	void Builder::append_init(BDD::BDDNode_ptr node) {
		for(auto &tail : this->init_tails) {
			BDDNode_ptr node_clone { node->clone() };
			BDDNode_ptr empty;
			node_clone->replace_prev(tail);
			tail->replace_next(node_clone);

			this->init_tails.erase(tail);

			// For branches add both sides of the branch to the tails and make sure its not a branch
			if(node_clone->get_type() == Node::NodeType::BRANCH) {
				explore_branch(node_clone, this->init_tails);
			}
			else {
				node_clone->replace_next(empty);
				this->init_tails.insert(node_clone);
			}
		}
	}

	void Builder::append_process(BDD::BDDNode_ptr node) {
		for(auto &tail : this->process_tails) {
			BDDNode_ptr node_clone { node->clone() };
			BDDNode_ptr empty;
			node_clone->replace_next(empty);
			node_clone->replace_prev(tail);
			tail->replace_next(node_clone);

			this->process_tails.erase(tail);

			// For branches add both sides of the branch to the tails and make sure its not a branch
			if(node_clone->get_type() == Node::NodeType::BRANCH) {
				explore_branch(node_clone, this->process_tails);
			}
			else {
				this->process_tails.insert(node_clone);
			}
		}
	}

	const std::unique_ptr<BDD::BDD>& Builder::get_bdd() const {
		return this->bdd;
	}

	void Builder::dump(std::string path) {
		debug("Dumping BDD to file");
		this->bdd->serialize(path);
	}
}