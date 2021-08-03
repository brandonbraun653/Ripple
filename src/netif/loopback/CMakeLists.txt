include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

gen_static_lib_variants(
  TARGET
    ripple_netif_loopback
  SOURCES
    lb_adapter.cpp
  PRV_LIBRARIES
    aurora_intf_inc
    chimera_intf_inc
    nanopb_intf_inc
    ripple_inc
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple/netif/loopback"
)
