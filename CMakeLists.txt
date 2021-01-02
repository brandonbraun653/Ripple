include("${COMMON_TOOL_ROOT}/cmake/utility/embedded.cmake")

add_subdirectory(src/datalink)
add_subdirectory(src/network)
add_subdirectory(src/physical)
add_subdirectory(src/shared)

# ====================================================
# All Drivers
# ====================================================
set(all_libraries
  ripple_datalink
  ripple_physical
  ripple_shared
  ripple_network
)

# ====================================================
# This function generates the following targets:
#   ripple_core
#   ripple_core_rel
#   ripple_core_rel_dbg
#   ripple_core_dbg
#
# Each target builds the appropriate variant of the
# libraries listed above in "all_libraries".
# ====================================================
function(build_ripple_core variant)

  # Add the variant flag to each library target
  set(all_libraries_tmp${variant} "")
  foreach(lib ${all_libraries})
    list(APPEND all_libraries_tmp${variant} ${lib}${variant})
  endforeach()

  # Create interface library for the appropriate variant
  set(RIPPLE${variant} ripple_core${variant})
  add_library("${RIPPLE${variant}}" INTERFACE)
  target_link_libraries("${RIPPLE${variant}}" INTERFACE "${all_libraries_tmp${variant}}")
  export(TARGETS "${RIPPLE${variant}}" FILE "${PROJECT_BINARY_DIR}/Ripple/${RIPPLE${variant}}.cmake")

endfunction()
add_target_variants(build_ripple_core)


# ====================================================
# Various exports
# ====================================================
# Public include location
add_library(ripple_inc INTERFACE)
target_include_directories(ripple_inc INTERFACE "./../" "./inc")
target_link_libraries(ripple_inc INTERFACE ctre_inc)
export(TARGETS ripple_inc FILE "${PROJECT_BINARY_DIR}/Ripple/ripple_inc.cmake")
