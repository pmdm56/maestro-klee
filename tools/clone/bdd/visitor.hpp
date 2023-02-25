#pragma once

#include "call-paths-to-bdd.h"

using KleeBDD = BDD::BDD;

using BDD::Branch;
using BDD::Call;
using BDD::ReturnInit;
using BDD::ReturnProcess;
using BDD::Node;
using BDD::ReturnRaw;
using BDD::BDDVisitor;

namespace Clone {
	class Visitor : public BDDVisitor {
	protected:
		BDDVisitor::Action visitBranch(const Branch *node) override;
		BDDVisitor::Action visitCall(const Call *node) override;
		BDDVisitor::Action visitReturnInit(const ReturnInit *node) override;
		BDDVisitor::Action visitReturnProcess(const ReturnProcess *node) override;
		BDDVisitor::Action visitReturnRaw(const ReturnRaw *node) ;

		void visitInitRoot(const Node *root) override;
		void visitProcessRoot(const Node *root) override;
	public:
		Visitor();
		~Visitor();

		void visit(const KleeBDD &bdd) override;
	};
}