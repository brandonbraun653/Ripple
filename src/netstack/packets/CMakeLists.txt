include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

gen_static_lib_variants(
  TARGET
    ripple_packets
  SOURCES
    encoder.cpp
    fragment.cpp
    packet.cpp
  PRV_LIBRARIES
    aurora_intf_inc
    chimera_intf_inc
    nanopb_intf_inc
    ripple_inc
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple"
)
