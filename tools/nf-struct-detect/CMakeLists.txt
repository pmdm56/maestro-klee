# ===------------------------------------------------------------------------===#
#
# The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
# ===------------------------------------------------------------------------===#
file(GLOB_RECURSE nf-struct-detect-sources
  "*.cpp"
)

file(GLOB_RECURSE nf-comb-sources
  "../nf-comb/*.cpp"
)

file(GLOB_RECURSE klee-util-sources
  "../klee-util/*.cpp"
)

file(GLOB_RECURSE load-call-paths-sources
  "../load-call-paths/*.cpp"
)

list(FILTER klee-util-sources EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER load-call-paths-sources EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER nf-comb-sources EXCLUDE REGEX ".*main\\.cpp$")

add_executable(nf-struct-detect
  ${nf-struct-detect-sources}
  ${nf-comb-sources}
  ${klee-util-sources}
  ${load-call-paths-sources}
)

set(KLEE_LIBS
  kleaverExpr
  kleeCore
)

target_include_directories(nf-struct-detect PRIVATE ../klee-util ../load-call-paths ../nf-comb)
target_link_libraries(nf-struct-detect ${KLEE_LIBS})

install(TARGETS nf-struct-detect RUNTIME DESTINATION bin)

