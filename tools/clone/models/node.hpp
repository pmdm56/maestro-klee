#pragma once

#include <string>
#include <memory>
#include <unordered_map>


namespace Clone {
	enum class NodeType {
		DEVICE,
		NF
	};

	class Node {
	private:
		const std::string name;
		const NodeType node_type;

		std::unordered_map<unsigned, std::pair<unsigned, std::shared_ptr<Node>>> children;

	public:
		Node(const std::string &name, NodeType node_type);
		~Node();

		std::string get_name() const;
		NodeType get_node_type() const;

		std::unordered_map<unsigned, std::pair<unsigned, std::shared_ptr<Node>>> get_children() const;
		std::pair<unsigned, std::shared_ptr<Node>> get_child(unsigned port) const;


		void add_child(unsigned port_from, unsigned port_to, const std::shared_ptr<Node> &node);

		void print() const;
	};
}
