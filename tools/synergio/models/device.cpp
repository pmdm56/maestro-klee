#include "device.hpp"

#include "../pch.hpp"

#include "../util/logger.hpp"

Device::Device(const std::string &id) : id(id) {}

Device::~Device() = default;

string Device::get_id() const {
	return id;
}

void Device::print() {
	debug("Device ", id);
}
