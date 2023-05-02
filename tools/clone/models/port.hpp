#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "device.hpp"

namespace Clone {
	using std::string;
	using std::unique_ptr;
	using std::unordered_map;
	using std::runtime_error;
	using std::shared_ptr;


	class Port {
	private:
		const DevicePtr device;
		const unsigned local_port;
		const unsigned global_port;

	public:
		Port(const DevicePtr device, unsigned local_port, unsigned global_port);
		~Port();
		
		inline DevicePtr get_device() const {
			return device;
		}

		inline unsigned get_local_port() const {
			return local_port;
		}

		inline unsigned get_global_port() const {
			return global_port;
		}
	};

	typedef shared_ptr<Port> PortPtr;
	typedef unordered_map<unsigned, PortPtr> PortMap;
}