#include "node.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Clone {
	Node::Node(const std::string &name, NodeType node_type): name(name), node_type(node_type), children() {}

	Node::~Node() {}

	string Node::get_name() const {
		return name;
	}

	NodeType Node::get_node_type() const {
		return node_type;
	}

	unordered_map<unsigned, std::pair<unsigned, std::shared_ptr<Node>>> Node::get_children() const {
		return children;
	}

	pair<unsigned, std::shared_ptr<Node>> Node::get_child(unsigned port) const {
		if(children.find(port) != children.end()) {
			return children.at(port);
		}
		else {
			return make_pair(0, nullptr);
		}
	}

	void Node::add_child(unsigned port_src, unsigned port_dst, const shared_ptr<Node> &node) {
		children[port_src] = std::make_pair(port_dst, node);
	}

	void Node::print() const {
		cout << "Node(" << name << ")" << endl;

		for(auto &child: this->get_children()) {
			unsigned port_src = child.first;
			unsigned port_dst = child.second.first;
			auto &child_node = child.second.second;
			cout << " - Port " << port_src << " -> " << child_node->get_name() << ":" << port_dst << std::endl;
		}
	}
}
