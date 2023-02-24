#pragma once

#include <memory>
#include <string>

#include "call-paths-to-bdd.h"

namespace KleeBDD = BDD; // alias to avoid conflicts with the class name

namespace Clone {
	typedef std::unique_ptr<KleeBDD::BDD> kBDD;

	class BDD {
	/**
	 * @brief BDD object
	 * 
	 * @note This is a wrapper around the BDD object from KleeBDD
	 * 
	 * @todo Add more methods to this class
	 */

		private:
			kBDD bdd;

		public:
			BDD(const std::string &path);
			~BDD() = default;

			void init(int constraint);
			void process(int constraint);
	};
}