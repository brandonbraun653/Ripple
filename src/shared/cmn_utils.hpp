/********************************************************************************
 *  File Name:
 *    cmn_utils.hpp
 *
 *  Description:
 *    Shared utility functions
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_COMMON_UTILITIES_HPP
#define RIPPLE_COMMON_UTILITIES_HPP

/* STL Includes */
#include <string_view>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>

#include <Ripple/src/network/network_types.hpp>
#include <Ripple/src/session/session_types.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Checks if the session pointer contains valid data
   *
   *  @param[in]  session       The handle to be checked
   *  @return bool
   */
  bool validateContext( Session::Context session );

  /**
   *  Converts series of numbers into the appropriate IP address
   *
   *  @param[in]  a             First octet
   *  @param[in]  b             Second octet
   *  @param[in]  c             Third octet
   *  @param[in]  d             Fourth octet
   *  @return IPAddress
   */
  Network::IPAddress constructIP( const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d );
}  // namespace Ripple

#endif  /* !RIPPLE_COMMON_UTILITIES_HPP */
