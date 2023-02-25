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

using Action = BDD::BDDVisitor::Action;

namespace Clone {
	class Visitor : public BDDVisitor {
	protected:
		Action visitBranch(const Branch *node) override;
		Action visitCall(const Call *node) override;
		Action visitReturnInit(const ReturnInit *node) override;
		Action visitReturnProcess(const ReturnProcess *node) override;
		Action visitReturnRaw(const ReturnRaw *node) ;

		void visitInitRoot(const Node *root) override;
		void visitProcessRoot(const Node *root) override;
	public:
		Visitor();
		~Visitor();
		
		void visit(const KleeBDD &bdd) override;
		void visit(const Branch *node);
  		void visit(const Call *node);
  		void visit(const ReturnInit *node);
  		void visit(const ReturnProcess *node);
  		void visit(const ReturnRaw *node);
	};
}