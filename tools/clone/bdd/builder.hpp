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

		void append(const BDD::Node *node);
	};
}