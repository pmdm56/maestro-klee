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

		void explore_node(BDD::BDDNode_ptr node, std::vector<unsigned> &constraints);
	public:
		~Builder();

		static std::unique_ptr<Builder> create();

		bool is_init_empty() const;
		bool is_process_empty() const;

		void join_bdd(const std::shared_ptr<const BDD::BDD> &other, std::vector<unsigned> &constraints);

		
		const std::unique_ptr<BDD::BDD>& get_bdd() const;
		void dump(std::string path);
	};
}