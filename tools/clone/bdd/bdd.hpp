#pragma once

#include <memory>
#include <string>

#include "call-paths-to-bdd.h"

namespace KleeBDD = BDD; // alias to avoid conflicts with the class name

namespace Clone {
	typedef const std::unique_ptr<KleeBDD::BDD> kBDD;

	/**
	 * @brief BDD object
	 * 
	 * @note This is a wrapper around the BDD object from KleeBDD
	 * 
	*/
	class BDD {
		private:
			kBDD bdd;
			BDD(std::unique_ptr<KleeBDD::BDD> bdd);
		public:
			~BDD();
			
			static std::unique_ptr<BDD> create(const std::string &path);

			void init(int constraint);
			void process(int constraint);
	};
}