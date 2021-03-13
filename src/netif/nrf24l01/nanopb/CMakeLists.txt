include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

project(NanoPB_Netif_NRF24 C)

nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS shockburst.proto)
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)


gen_static_lib_variants(
  TARGET
    ripple_netif_nrf24_protobuf
  SOURCES
    shockburst.pb.c
  PRV_INCLUDES
    .
    ${NANOPB_INCLUDE_DIRS}
  EXPORT_DIR
    "${PROJECT_BINARY_DIR}/Ripple/netif/protobuf"
)
