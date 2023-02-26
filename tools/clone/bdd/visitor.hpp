#pragma once

#include <vector>

#include "call-paths-to-bdd.h"

namespace Clone {	
	class Visitor : public BDD::BDDVisitor {
	private:
		std::vector<unsigned> constraints;
	protected:
		BDD::BDDVisitor::Action visitBranch(const BDD::Branch *node) override;
		BDD::BDDVisitor::Action visitCall(const BDD::Call *node) override;
		BDD::BDDVisitor::Action visitReturnInit(const BDD::ReturnInit *node) override;
		BDD::BDDVisitor::Action visitReturnProcess(const BDD::ReturnProcess *node) override;
		BDD::BDDVisitor::Action visitReturnRaw(const BDD::ReturnRaw *node) ;

		void visitInitRoot(const BDD::Node *root) ;
		void visitProcessRoot(const BDD::Node *root) ;
	public:
		Visitor();
		Visitor(std::vector<unsigned> &constraintInputPort);
		~Visitor();
		
		void visit(const BDD::BDD &bdd) override;
	};
}