#pragma once

#include <vector>
#include <memory>

#include "call-paths-to-bdd.h"

namespace Clone {	
	class BDDBuilder;
	class Visitor : public BDD::BDDVisitor {
	private:
		std::vector<unsigned> constraints;
		const std::unique_ptr<BDDBuilder> &builder;
	protected:
		BDD::BDDVisitor::Action visitBranch(const BDD::Branch *node) override;
		BDD::BDDVisitor::Action visitCall(const BDD::Call *node) override;
		BDD::BDDVisitor::Action visitReturnInit(const BDD::ReturnInit *node) override;
		BDD::BDDVisitor::Action visitReturnProcess(const BDD::ReturnProcess *node) override;
		BDD::BDDVisitor::Action visitReturnRaw(const BDD::ReturnRaw *node) ;

		void visitInitRoot(const BDD::Node *root) ;
		void visitProcessRoot(const BDD::Node *root) ;
	public:
		Visitor(std::vector<unsigned> &constraintInputPort, const std::unique_ptr<BDDBuilder> &builder);
		~Visitor();
		
		void visit(const BDD::BDD &bdd) override;
	};
}