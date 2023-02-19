#pragma once

#include <string>

class Link {
private:
	static size_t id_counter;

	const size_t id;
	const std::string node1;
	const std::string node2;
	const int port1;
	const int port2;

public:
	Link(const std::string &node1, const int port1, const std::string &node2, const int port2);
	~Link();

	size_t get_id() const;
	std::string get_node1() const;
	std::string get_node2() const;
 	int get_port1() const;
	int get_port2() const;

	void print();
};
