include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

gen_static_lib_variants(
  TARGET
    ripple_netif_nrf24_datalink
  SOURCES
    data_link_arp.cpp
    data_link_frame.cpp
    data_link_service.cpp
  PRV_LIBRARIES
    aurora_intf_inc
    chimera_intf_inc
    ripple_inc
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple/netif/datalink"
)
