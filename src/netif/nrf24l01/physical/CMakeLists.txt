include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

gen_static_lib_variants(
  TARGET
    ripple_netif_nrf24_physical
  SOURCES
    phy_device_driver.cpp
    phy_device_driver_virtual.cpp
    phy_device_internal.cpp
    phy_fsm_controller.cpp
  PRV_LIBRARIES
    aurora_intf_inc
    chimera_intf_inc
    ripple_inc
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple/netif/physical"
)
