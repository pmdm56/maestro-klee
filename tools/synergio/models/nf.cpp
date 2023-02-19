#include "nf.hpp"

#include "../pch.hpp"

#include "../util/logger.hpp"

NF::NF(const std::string &id, const std::string &path) : id(id), path(path) {}

NF::~NF() {}

std::string NF::get_id() const {
	return id;
}

std::string NF::get_path() const {
	return path;
}

void NF::load() {
	info("Loading BDD ", id, " from ", path);
	success("BDD ", id, " loaded");
	
}

void NF::print() {
	debug("NF ", id, " from ", path);
}
