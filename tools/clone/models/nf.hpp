#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace BDD {
	class BDD;
}

namespace Clone {
	class NF {
	private:
		const std::string id;
		const std::string path;
		std::shared_ptr<const BDD::BDD> bdd { nullptr };
	public:
		NF(const std::string &id, const std::string &path);
		~NF();

		std::string get_id() const;
		std::string get_path() const;

		std::shared_ptr<const BDD::BDD> get_bdd() const;
		void set_bdd(std::shared_ptr<const BDD::BDD> bdd);
		
		void print() const;
	};

	typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;
}
