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
		BDDVisitor::Action visitBranch(const Branch *node);
		BDDVisitor::Action visitCall(const Call *node);
		BDDVisitor::Action visitReturnInit(const ReturnInit *node);
		BDDVisitor::Action visitReturnProcess(const ReturnProcess *node);
		BDDVisitor::Action visitReturnRaw(const ReturnRaw *node);

		void visitInitRoot(const Node *root);
		void visitProcessRoot(const Node *root);
	public:
		Visitor();
		~Visitor();
		
		void visit(const Branch *node);
		void visit(const Call *node);
		void visit(const ReturnInit *node);
		void visit(const ReturnProcess *node);
		void visit(const ReturnRaw *node);
		void visit(const KleeBDD::BDD &bdd);

	};
}