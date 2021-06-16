include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

add_subdirectory(packets)
add_subdirectory(util)

gen_static_lib_variants(
  TARGET
    ripple_network_stack
  SOURCES
    context.cpp
    socket.cpp
  PRV_LIBRARIES
    aurora_intf_inc
    chimera_intf_inc
    ripple_inc
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple/netstack"
)
