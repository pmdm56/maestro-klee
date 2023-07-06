#pragma once

#include <vector>
#include <string>

namespace BDD {
	constexpr char INIT_CONTEXT_MARKER[] = "start_time";
	constexpr char MAGIC_SIGNATURE[] = "===== VIGOR_BDD_SIG =====";

	extern std::vector<std::string> skip_conditions_with_symbol;
}