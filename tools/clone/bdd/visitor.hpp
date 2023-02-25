#pragma once

#include "call-paths-to-bdd.h"

namespace KleeBDD = BDD; // alias to avoid conflicts with the class name

using KleeBDD::Branch;
using KleeBDD::Call;
using KleeBDD::ReturnInit;
using KleeBDD::ReturnProcess;
using KleeBDD::Node;
using KleeBDD::ReturnRaw;
using KleeBDD::BDD;
using KleeBDD::BDDVisitor;

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

		void visit(const KleeBDD::BDD &bdd) override;
	};
}