#pragma once

#include <string>
#include <unordered_map>
#include <memory>

class NF {
private:
	std::string id {""};
	std::string path {""};
public:
	NF(const std::string &id, const std::string &path);
	~NF();

	std::string get_id() const;
	std::string get_path() const;
	
	void load();
	void print();
};

typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;
