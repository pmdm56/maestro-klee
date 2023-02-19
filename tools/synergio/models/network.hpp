#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

class Device;
class NF;
class Link;

typedef std::unordered_map<std::string, std::unique_ptr<Device>> Devices;
typedef std::unordered_map<std::string, std::unique_ptr<NF>> NFs;
typedef std::vector<std::unique_ptr<Link>> Topology;

class Network {
private:
	Devices devices;
	NFs nfs;
 	Topology topology;

public:
	Network(Devices devices, NFs nfs, Topology links);
	~Network();

	void load();
	void print();
};
