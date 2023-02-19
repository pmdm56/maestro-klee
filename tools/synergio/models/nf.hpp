#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "call-paths-to-bdd.h"

namespace Synergio {
	class NF {
	private:
		std::string id {""};
		std::string path {""};
		
		std::unique_ptr<BDD::BDD> bdd {nullptr};
	public:
		NF(const std::string &id, const std::string &path);
		~NF();

		std::string get_id() const;
		std::string get_path() const;
		
		void set_bdd(std::unique_ptr<BDD::BDD> bdd);

		void print();
	};

	typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;
}
