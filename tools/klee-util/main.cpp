#include "klee-util.h"

int main() {
	kutil::solver_toolbox.build();

	auto A = kutil::solver_toolbox.create_new_symbol("A", 64);
	std::cerr << "A " << kutil::expr_to_string(A) << "\n";

	return 0;
}
