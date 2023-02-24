#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace Clone {
	class BDD;


	class NF {
	private:
		const std::string id;
		const std::string path;
		
		std::shared_ptr<BDD> bdd {nullptr};
	public:
		NF(const std::string &id, const std::string &path);
		~NF();

		std::string get_id() const;
		std::string get_path() const;
		
		std::shared_ptr<BDD> get_bdd() const;
		void set_bdd(std::shared_ptr<BDD> bdd);

		void process_bdd(int constraint);
		void print();
	};

	typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;
}
