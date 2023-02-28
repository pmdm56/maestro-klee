#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
file(GLOB_RECURSE util-sources
  "*.cpp"
)

file(GLOB_RECURSE load-call-paths-sources
  "../load-call-paths/*.cpp"
)

list(FILTER load-call-paths-sources EXCLUDE REGEX ".*main\\.cpp$")

add_executable(util
  ${util-sources}
  ${load-call-paths-sources}
)

set(KLEE_LIBS
  kleaverExpr
  kleeCore
)

target_include_directories(util PRIVATE ../load-call-paths)
target_link_libraries(util ${KLEE_LIBS})

install(TARGETS util RUNTIME DESTINATION bin)
