#pragma once

#include <string>
#include <memory>

namespace Clone {
	class Network;
	
	/** 
	 * Parses a file containing the description of the network 
	 * 
	 * @param network_file the path to the file containing the network description
	 * @return a single Network object, representing the network described in the file
	*/
	std::unique_ptr<Network> parse(const std::string &input_file);
}
