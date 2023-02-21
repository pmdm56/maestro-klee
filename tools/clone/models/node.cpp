#include "node.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"

namespace Clone {
	Node::Node(const std::string &name): name(name), neighbours() {
		debug("Creating node for ", name);
	}

	Node::~Node() {
		info("Node destroyed");
	}

	string Node::get_name() const {
		return name;
	}

	unordered_map<unsigned, shared_ptr<Node>> Node::get_neighbours() const {
		return neighbours;
	}

	void Node::add_neighbour(unsigned port, const shared_ptr<Node> &neighbour) {
		neighbours[port] = neighbour;
	}

	void Node::print() const {
		cout << "Node(" << name << ")" << endl;

		for(auto &neighbour: this->get_neighbours()) {
			cout << " - Port " << neighbour.first << ": " << neighbour.second->get_name() << std::endl;
		}
	}
}
