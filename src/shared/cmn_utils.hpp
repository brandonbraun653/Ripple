/********************************************************************************
 *  File Name:
 *    cmn_utils.hpp
 *
 *  Description:
 *    Shared utility functions
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_COMMON_UTILITIES_HPP
#define RIPPLE_COMMON_UTILITIES_HPP

/* STL Includes */
#include <string_view>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Converts series of numbers into the appropriate IP address
   *
   *  @param[in]  a             First octet
   *  @param[in]  b             Second octet
   *  @param[in]  c             Third octet
   *  @param[in]  d             Fourth octet
   *  @return IPAddress
   */
  IPAddress constructIP( const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d );

  /**
   *  Instruct a Ripple task to halt execution until told to continue
   *  via some task message from another thread.
   *
   *  @return void
   */
  void TaskWaitInit();

}  // namespace Ripple

#endif  /* !RIPPLE_COMMON_UTILITIES_HPP */
