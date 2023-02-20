#pragma once

#include "call-paths-to-bdd.h"

namespace Clone {
	class Visitor: public BDD::BDDVisitor {
		protected:
			virtual Action visitBranch(const BDD::Branch *node) override;
			virtual Action visitCall(const BDD::Call *node) override;
			virtual Action visitReturnInit(const BDD::ReturnInit *node) override;
			virtual Action visitReturnProcess(const BDD::ReturnProcess *node) override;
			virtual Action visitReturnRaw(const BDD::ReturnRaw *node) override;
		public:
			Visitor();
	};
}