#include "pch.hpp"

#include "parser/parser.hpp"
#include "network/network.hpp"

void usage() {
	cout << "Usage: clone [input-file]" << endl;
}

int main(int argc, char **argv) {
	if(argc != 2) {
		usage();
		return EXIT_FAILURE;
	}

	const string input_file {argv[1]};
	auto network = Clone::parse(input_file);

	network->consolidate();

	return EXIT_SUCCESS;
}
