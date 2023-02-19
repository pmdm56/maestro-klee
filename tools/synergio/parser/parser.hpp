#pragma once

#include <string>
#include <memory>

namespace Synergio {
	class Network;
	
	std::unique_ptr<Network> parse(const std::string &input_file);
}
