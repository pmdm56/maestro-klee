#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include "call-paths-to-bdd.h"


namespace Clone {
	class Builder { 
	private:
		const std::unique_ptr<BDD::BDD> bdd;
		std::unordered_set<BDD::BDDNode_ptr> init_tails;
		std::unordered_set<BDD::BDDNode_ptr> process_tails;
		
		Builder(std::unique_ptr<BDD::BDD> bdd);

		void explore_branch(BDD::BDDNode_ptr node, std::unordered_set<BDD::BDDNode_ptr> &tails);
	public:
		~Builder();

		static std::unique_ptr<Builder> create();

		bool is_init_empty() const;
		bool is_process_empty() const;
		void populate_init(BDD::BDDNode_ptr node);
		void populate_process(BDD::BDDNode_ptr node);
		void append_init(BDD::BDDNode_ptr node);
		void append_process(BDD::BDDNode_ptr node);
		const std::unique_ptr<BDD::BDD>& get_bdd() const;
		void dump(std::string path);
	};
}