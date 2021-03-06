include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

# ====================================================
# Common
# ====================================================
set(LINK_LIBS
  aurora_intf_inc        # Aurora public headers
        # Boost public headers
  chimera_intf_inc       # Chimera public headers
  ripple_inc
)

# ====================================================
# Interface Library
# ====================================================
function(build_library variant)
  set(DRIVER ripple_shared${variant})
  add_library(${DRIVER} STATIC
    cmn_utils.cpp
  )
  target_link_libraries(${DRIVER} PRIVATE ${LINK_LIBS} prj_device_target prj_build_target${variant})
  export(TARGETS ${DRIVER} FILE "${PROJECT_BINARY_DIR}/Ripple/shared/${DRIVER}.cmake")
endfunction()

add_target_variants(build_library)
