#pragma once

#include <string>

namespace Synergio {
	class Device {
	private:
		std::string id = "";
	public:
		Device(const std::string &id);
		~Device();

		std::string get_id() const;

		void print();
	};
}