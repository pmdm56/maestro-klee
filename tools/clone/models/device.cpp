#include "device.hpp"

#include "../pch.hpp"


namespace Clone {
	Device::Device(const std::string &id) : id(id) {}

	Device::~Device() = default;

	void Device::print() const {
		debug("Device ", id);
	}
}
