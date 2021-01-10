include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

# ====================================================
# Common
# ====================================================
set(LINK_LIBS
  aurora_intf_inc        # Aurora public headers
  Boost::boost      # Boost public headers
  chimera_intf_inc       # Chimera public headers
  ripple_inc
)

# ====================================================
# Interface Library
# ====================================================
function(build_library variant)
  set(DRIVER ripple_datalink${variant})
  add_library(${DRIVER} STATIC
    data_link_arp.cpp
    data_link_driver.cpp
    data_link_service.cpp
  )
  target_link_libraries(${DRIVER} PRIVATE ${LINK_LIBS} prj_device_target prj_build_target${variant})
  export(TARGETS ${DRIVER} FILE "${PROJECT_BINARY_DIR}/Ripple/datalink/${DRIVER}.cmake")
endfunction()

add_target_variants(build_library)
