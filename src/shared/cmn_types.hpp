/********************************************************************************
 *  File Name:
 *    cmn_types.hpp
 *
 *  Description:
 *    Common type definitions and declarations shared across Ripple
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_COMMON_TYPES_HPP
#define RIPPLE_COMMON_TYPES_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using IPAddress = uint32_t;
  using Port      = uint8_t;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct BootConfig
  {

  };

}  // namespace Ripple

#endif  /* !RIPPLE_COMMON_TYPES_HPP */
