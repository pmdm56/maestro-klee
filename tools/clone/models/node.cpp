#include "node.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Clone {
	Node::Node(const std::string &name, NodeType node_type): name(name), node_type(node_type), children(), parents() {
		debug("Creating node for ", name);
	}

	Node::~Node() {
		info("Node destroyed");
	}

	string Node::get_name() const {
		return name;
	}

	unordered_map<unsigned, std::shared_ptr<Node>> Node::get_parents() const {
		return parents;
	}

	unordered_map<unsigned, std::shared_ptr<Node>> Node::get_children() const {
		return children;
	}

	void Node::add_parent(unsigned port, const shared_ptr<Node> &node) {
		parents[port] = node;
	}

	void Node::add_child(unsigned port, const shared_ptr<Node> &node) {
		children[port] = node;
	}

	void Node::print() const {
		cout << "Node(" << name << ")" << endl;

		for(auto &neighbour: this->get_children()) {
			cout << " - Port " << neighbour.first << ": " << neighbour.second->get_name() << std::endl;
		}
	}
}
