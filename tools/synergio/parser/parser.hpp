#pragma once

#include <string>
#include <memory>

class Network;

std::unique_ptr<Network> parse(const std::string &input_file);
