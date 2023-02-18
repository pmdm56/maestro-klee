#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
file(GLOB_RECURSE synergio-sources
  "*.cpp"
)

add_executable(synergio
  ${synergio-sources}
)

set(KLEE_LIBS
  kleaverExpr
  kleeCore
)

target_include_directories(synergio PRIVATE)
target_link_libraries(synergio ${KLEE_LIBS})

install(TARGETS synergio RUNTIME DESTINATION bin)
