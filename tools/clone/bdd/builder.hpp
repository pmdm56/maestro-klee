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
		std::unique_ptr<BDD::BDD> bdd;	
	public:
		Builder();
		~Builder();

		bool is_init_empty() const;
		bool is_process_empty() const;
		void append(BDD::Node *node);
		void dump(std::string path);
	};
}