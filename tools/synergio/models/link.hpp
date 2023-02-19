#pragma once

#include <string>

namespace Synergio {
	class Link {
	private:
		static size_t id_counter;

		const size_t id;
		const std::string node1;
		const std::string node2;
		const unsigned port1;
		const unsigned port2;

	public:
		Link(const std::string &node1, const unsigned port1, const std::string &node2, const unsigned port2);
		~Link();

		size_t get_id() const;
		std::string get_node1() const;
		std::string get_node2() const;
		unsigned get_port1() const;
		unsigned get_port2() const;

		void print();
	};
}
