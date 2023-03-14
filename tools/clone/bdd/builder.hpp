#pragma once

#include <memory>
#include <string>

namespace BDD {
	class BDD;
	class Node;
}

namespace Clone {
	class Builder { 
	private:
		const std::unique_ptr<BDD::BDD> bdd;
		Builder(std::unique_ptr<BDD::BDD> bdd);
	public:
		~Builder();

		static std::unique_ptr<Builder> create();

		bool is_init_empty() const;
		bool is_process_empty() const;
		void append(BDD::Node *node);
		void dump(std::string path);
	};
}