include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

add_subdirectory(src/shared)
add_subdirectory(src/netif)
add_subdirectory(src/netstack)
add_subdirectory(src/userif)

# ====================================================
# Various exports
# ====================================================
gen_intf_lib(
  TARGET
    ripple_inc
  INTF_INCLUDES
    ./../
    ./inc
    ./lib/zeromq
  INTF_LIBRARIES
    ctre_inc
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple"
)
