#include "parser.hpp"

#include "../pch.hpp"
#include "../constants.hpp"

#include "../util/logger.hpp"

#include "../models/device.hpp"
#include "../models/nf.hpp"
#include "../models/link.hpp"
#include "../models/network.hpp"


namespace Synergio {
	/** 
	 * Parses a file containing the description of the network 
	 * 
	 * @param network_file the path to the file containing the network description
	 * @return a single Network object, representing the network described in the file
	*/
	unique_ptr<Network> parse(const string &network_file) {
		info("Parsing network " + network_file);

		ifstream fstream;
		fstream.open(network_file);

		if(!fstream.is_open()) {
			danger("Could not open input file " + network_file);
		}

		Devices devices;
		NFs nfs;
		Topology topology;
		
		string line;
		
		while(getline(fstream, line)) {
			stringstream ss(line);
			string word;
			vector<string> words;

			while(ss >> word) {
				words.push_back(word);
			}

			if(words.size() == 0) {
				warn("Empty line found while parsing input");
				continue;
			}

			const string &type = words[0];

			if(type == STRING_DEVICE) {
				if(words.size() != LENGTH_DEVICE_INPUT) {
					danger("Invalid device line: " + line);
				}
				string id { words[1] };
				devices[id] = unique_ptr<Device>(new Device(id));
			} 
			else if(type == STRING_NF) {
				if(words.size() != LENGTH_NF_INPUT) {
					danger("Invalid nf line: " + line);
				}

				string id { words[1]} ;
				string path { words[2] };
				nfs[id] = unique_ptr<NF>(new NF(id, path));
			} 
			else if(type == STRING_LINK) {
				if(words.size() != LENGTH_LINK_INPUT) {
					danger("Invalid link line: " + line);
				}
				
				string node1 {words[1]};

				if (nfs.find(node1) == nfs.end() && devices.find(node1) == devices.end()) { 
					danger("Could not find node " + node1 + " for link " + line);
				}

				string sport1 {words[2]};
				int port1 {stoi(sport1)};

				string node2 {words[3]};


				if (nfs.find(node2) == nfs.end() && devices.find(node2) == devices.end()) { 
					danger("Could not find node " + node2 + " for link " + line);
				}
				string sport2 {words[4]};
				int port2 {stoi(sport2)};

				topology.push_back(unique_ptr<Link>(new Link(node1, port1, node2, port2)));
			} 
			else {
				danger("Invalid line: " + line);
			}
		}

		success("Parsed network " + network_file);
		
		unique_ptr<Network> network = unique_ptr<Network>(new Network(move(devices), move(nfs), move(topology)));
		return network;
	}
}
