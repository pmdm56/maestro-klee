#pragma once

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <unordered_set>

#include "call-paths-to-bdd.h"

#include "../models/nf.hpp"

namespace Clone {
	typedef std::deque<BDD::BDDNode_ptr> Tails;

	class Builder { 
	private:
		const std::unique_ptr<BDD::BDD> bdd;
		std::unordered_set<std::string> merged_inits;

		BDD::BDDNode_ptr init_tail = nullptr;
		BDD::BDDNode_ptr process_root = nullptr;

		uint64_t counter = 1;
		Builder(std::unique_ptr<BDD::BDD> bdd);

		Tails clone_node(BDD::BDDNode_ptr node, uint32_t input_port);
		void trim_node(BDD::BDDNode_ptr curr, BDD::BDDNode_ptr next);
	public:
		~Builder();

		static std::unique_ptr<Builder> create();

		bool is_init_empty() const;
		bool is_process_empty() const;

		void replace_with_drop(BDD::BDDNode_ptr node);

		void add_process_branch(unsigned input_port);

		void initialise_init(const std::shared_ptr<const BDD::BDD> &bdd);
		void initialise_process(const std::shared_ptr<const BDD::BDD> &bdd);

		void join_init(const std::shared_ptr<NF> &nf);
		Tails join_process(const std::shared_ptr<NF> &nf, unsigned input_port, const BDD::BDDNode_ptr &tail);

		BDD::BDDNode_ptr get_process_root() const;
		
		const std::unique_ptr<BDD::BDD>& get_bdd() const;
		void dump(std::string path);
	};
}
