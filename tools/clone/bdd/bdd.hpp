#pragma once

#include <memory>
#include <string>
#include <vector>

#include "call-paths-to-bdd.h"

using KleeBDD = BDD::BDD;

namespace Clone {
	class Visitor;

	/**
	 * @brief BDD object
	 * 
	 * @note This is a wrapper around the BDD object from KleeBDD
	 * 
	*/
	class BDD {
		private:
			/* Member variables */
			const std::unique_ptr<KleeBDD> bdd;

			/* Constructors and destructors */
			BDD(std::unique_ptr<KleeBDD> bdd);
		public:
			/* Constructors and destructors */
			~BDD();
			
			/* Static methods */
			static std::unique_ptr<BDD> create(const std::string &path);

			/* Public methods */
			void visit(Visitor &visitor) const;

	};
}