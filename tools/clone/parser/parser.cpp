#include "parser.hpp"

/** Common headers **/
#include "../pch.hpp"
#include "../constants.hpp"
#include "../util/logger.hpp"

/** Model **/
#include "../models/device.hpp"
#include "../models/nf.hpp"
#include "../models/link.hpp"
#include "../models/network.hpp"

namespace Clone {
	ifstream open_file(const string &path) {
		ifstream fstream;
		fstream.open(path);

		if(!fstream.is_open()) {
			danger("Could not open input file ",  path);
		}

		return fstream;
	}

	unique_ptr<Device> parse_device(const vector<string> &words) {
		if(words.size() != LENGTH_DEVICE_INPUT) {
			throw runtime_error("Invalid device at line ");
		}

		const string id { words[1] };

		return unique_ptr<Device>(new Device(id));
	}

	unique_ptr<NF> parse_nf(const vector<string> &words) {
		if(words.size() != LENGTH_NF_INPUT) {
			throw runtime_error("Invalid network function at line ");
		}

		const string id { words[1] };
		const string path { words[2] };

		return unique_ptr<NF>(new NF(id, path));;
	}

	unique_ptr<Link> parse_link(const vector<string> &words, const Devices &devices, const NFs &nfs) {
		if(words.size() != LENGTH_LINK_INPUT) {
			throw runtime_error("Invalid link at line: ");
		}

		const string node1 { words[1] };
		const string sport1 { words[2] };
		const unsigned port1 { stoul(sport1) };

		if (nfs.find(node1) == nfs.end() && devices.find(node1) == devices.end()) { 
			throw runtime_error("Could not find node " + node1 + " at line: ");
		}

		const string node2 { words[3] };
		const string sport2 { words[4] };
		const unsigned port2 { stoul(sport2) };

		if (nfs.find(node2) == nfs.end() && devices.find(node2) == devices.end()) { 
			throw runtime_error("Could not find node " + node2 + " at line: ");
		}

		return unique_ptr<Link>(new Link(node1, port1, node2, port2));
	}

	unique_ptr<Network> parse(const string &network_file) {
		info("Parsing network ", network_file);

		ifstream fstream = open_file(network_file);

		NFs nfs;
		BDDs bdds;
		Links links;
		Devices devices;
		
		string line;
		
		while(getline(fstream, line)) {
			stringstream ss{line};
			vector<string> words;
			string word;

			while(ss >> word) {
				words.push_back(word);
			}

			if(words.size() == 0) {
				warn("Empty line found while parsing input");
				continue;
			}

			const string type { words[0] };

			try {
				if(type == STRING_DEVICE) {
					auto device = parse_device(words);
					devices[device->get_id()] = move(device);
				} 
				else if(type == STRING_NF) {
					auto nf = parse_nf(words);	
					nfs[nf->get_id()] = move(nf);
				} 
				else if(type == STRING_LINK) {
					auto link = parse_link(words, devices, nfs);
					links.push_back(move(link));
				} 
				else {
					danger("Invalid line: ", line);
				}
			}
			catch(const invalid_argument &e) {
				danger("Provide a valid port at line ->", line);
			}
			catch(const out_of_range &e) {
				danger("Provide a valid port at line ->", line);
			}
			catch(const runtime_error &e) {
				danger(e.what(), line);
			}
			catch(const exception &e) {
				danger(e.what(), line);
			}
			catch(...) {
				danger("Unknown error while parsing line:", line);
			}
		}

		success("Parsed network " + network_file);

		return Network::create(move(devices), move(nfs), move(links));
	}
}
