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

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>

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
  bool validateContext( SessionContext session );
}  // namespace Ripple

#endif  /* !RIPPLE_COMMON_UTILITIES_HPP */
