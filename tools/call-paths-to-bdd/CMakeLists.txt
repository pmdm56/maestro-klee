#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
file(GLOB_RECURSE call-paths-to-bdd-sources
  "*.cpp"
)

file(GLOB_RECURSE load-call-paths-sources
  "../load-call-paths/*.cpp"
)

file(GLOB_RECURSE util-sources
  "../util/*.cpp"
)

list(FILTER load-call-paths-sources EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER util-sources EXCLUDE REGEX ".*main\\.cpp$")

add_executable(call-paths-to-bdd
  ${call-paths-to-bdd-sources}
  ${load-call-paths-sources}
  ${util-sources}
)

set(KLEE_LIBS
  kleaverExpr
  kleeCore
)

target_include_directories(call-paths-to-bdd PRIVATE ../load-call-paths ../util)
target_link_libraries(call-paths-to-bdd ${KLEE_LIBS})

install(TARGETS call-paths-to-bdd RUNTIME DESTINATION bin)
