#pragma once

#include <memory>

namespace BDD {
	class BDD;
	class Node;
}

namespace Clone {
	class Builder { 
	private:	
		std::unique_ptr<const BDD::BDD> bdd;	
	public:
		Builder();
		~Builder();

		bool is_empty() const;
		void init();
		void append(const BDD::Node *node);
	};
}