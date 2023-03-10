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

		bool is_empty() const;
		void init(const BDD::BDD *bdd);
		void append(BDD::Node *node);

		void dump(std::string path);
	};
}