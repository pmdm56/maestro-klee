#pragma once

#include <vector>
#include <memory>

#include "call-paths-to-bdd.h"

using BDD::Branch;
using BDD::Call;
using BDD::ReturnInit;
using BDD::ReturnProcess;
using BDD::ReturnRaw;
using BDD::Node;


namespace Clone {	
	class Builder;
	class Visitor : public BDD::BDDVisitor {
	private:
		std::vector<unsigned> constraints;
		const std::unique_ptr<Builder> &builder;
	protected:
		BDD::BDDVisitor::Action visitBranch(const Branch *node) override;
		BDD::BDDVisitor::Action visitCall(const Call *node) override;
		BDD::BDDVisitor::Action visitReturnInit(const ReturnInit *node) override;
		BDD::BDDVisitor::Action visitReturnProcess(const ReturnProcess *node) override;
		BDD::BDDVisitor::Action visitReturnRaw(const ReturnRaw *node) override;

		void visitInitRoot(const BDD::Node *root) ;
		void visitProcessRoot(const BDD::Node *root) ;
	public:
		Visitor(std::vector<unsigned> &constraintInputPort, const std::unique_ptr<Builder> &builder);
		~Visitor();
		
		void visit(const BDD::BDD &bdd) override;
	};
}