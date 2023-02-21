#pragma once

#include <string>
#include <memory>
#include <unordered_map>


namespace Clone {
	class Node {
	private:
		const std::string name;
		std::unordered_map<unsigned, std::shared_ptr<Node>> neighbours;

	public:
		Node(const std::string &name);
		~Node();

		std::string get_name() const;
		std::unordered_map<unsigned, std::shared_ptr<Node>> get_neighbours() const;

		void add_neighbour(unsigned port, const std::shared_ptr<Node> &neighbour);


		void print() const;
	};
}
