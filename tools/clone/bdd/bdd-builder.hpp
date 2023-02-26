#pragma once

#include <memory>

namespace Clone {
	class

	class BDDBuilder { 
	private:

	public:
		BDDBuilder(std::unique_ptr<BDD> bdd);
		~BDDBuilder();
	};
}