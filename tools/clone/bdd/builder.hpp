#pragma once

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <unordered_set>

#include "call-paths-to-bdd.h"


namespace Clone {
	class Builder { 
	private:
		const std::unique_ptr<BDD::BDD> bdd;
		std::unordered_set<std::string> merged_nf_inits;
		std::unordered_set<BDD::BDDNode_ptr> init_tails;
		std::unordered_set<BDD::BDDNode_ptr> process_tails;

		uint64_t counter = 1;
		Builder(std::unique_ptr<BDD::BDD> bdd);

		void clone_node(BDD::BDDNode_ptr node, unsigned input_port);
		void trim_branch(BDD::BDDNode_ptr curr, BDD::BDDNode_ptr next);
	public:
		~Builder();

		static std::unique_ptr<Builder> create();

		bool is_init_empty() const;
		bool is_process_empty() const;

		bool is_init_merged(const std::string &nf_id) const;
		void add_merged_nf_init(const std::string &nf_id);

		void join_init(const std::shared_ptr<const BDD::BDD> &other, unsigned input_port);
		void join_process(const std::shared_ptr<const BDD::BDD> &other, unsigned input_port);
		
		const std::unique_ptr<BDD::BDD>& get_bdd() const;
		void dump(std::string path);
	};
}