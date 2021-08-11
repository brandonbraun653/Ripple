include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

gen_static_lib_variants(
  TARGET
    ripple_user_intf
  SOURCES
    user_init.cpp
    user_transaction.cpp
  PRV_LIBRARIES
    aurora_intf_inc
    chimera_intf_inc
    ripple_inc
    project_inc
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple/physical"
)
