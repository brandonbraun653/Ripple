/********************************************************************************
 *  File Name:
 *    data_link_driver.hpp
 *
 *  Description:
 *    Driver interface for the datalink layer
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_DATALINK_DRIVER_INTERFACE_HPP
#define RIPPLE_DATALINK_DRIVER_INTERFACE_HPP

/* Ripple Includes */
#include <Ripple/src/datalink/data_link_types.hpp>
#include <Ripple/src/session/session_types.hpp>

namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Extracts the handle from the session context
   *
   *  @param[in]  session       The current session
   *  @return Handle *
   */
  Handle *getHandle( Session::Context session );

}  // namespace Ripple::DATALINK

#endif  /* !RIPPLE_DATALINK_DRIVER_INTERFACE_HPP */
