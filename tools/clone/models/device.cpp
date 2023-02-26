#include "device.hpp"

#include "../pch.hpp"
#include "../util/logger.hpp"


namespace Clone {
	Device::Device(const std::string &id) : id(id) {}

	Device::~Device() = default;

	string Device::get_id() const {
		return id;
	}

	void Device::print() const {
		debug("Device ", id);
	}
}
