#include "bdd.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

#include "visitor.hpp"

namespace Clone {
	BDD::BDD(const string &path) {
		bdd = unique_ptr<KleeBDD::BDD>(new KleeBDD::BDD(path));
	}

	void BDD::traverse(const KleeBDD::BDDNode_ptr &node) {
		if (node == nullptr) {
			return;
		}

		const auto &node_type = node->get_type();

		info("Node type ", node_type, " at ", node->get_id());
		
		switch (node_type) {
			case KleeBDD::Node::NodeType::BRANCH:
				break;
			case KleeBDD::Node::NodeType::CALL:
				traverse(node->get_next());
				break;
			case KleeBDD::Node::NodeType::RETURN_INIT:
				break;
			case KleeBDD::Node::NodeType::RETURN_PROCESS:
				break;
			case KleeBDD::Node::NodeType::RETURN_RAW:
				break;
		}
	}

	void BDD::init(int constraint) {
		const auto &node = bdd->get_init();
		unique_ptr<Visitor> visitor(new Visitor());
		
		traverse(node);
	}

	void BDD::process(int constraint) {

	}
}