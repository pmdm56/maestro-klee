#pragma once

#include <memory>

namespace BDD {
	class Node;
}

namespace Clone {
	class Builder { 
	private:		
	public:
		Builder();
		~Builder();

		bool is_empty() const;
		void append(const BDD::Node *node);
	};
}