/********************************************************************************
 *  File Name:
 *    session_driver.hpp
 *
 *  Description:
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SESSION_HPP
#define RIPPLE_SESSION_HPP

/* Ripple Includes */
#include <Ripple/src/session/session_types.hpp>

namespace Ripple::Session
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Gets the session layer handle from the context information
   *
   *  @param[in]  context       User context
   *  @return Handle *
   */
  Handle *getHandle( Context context );

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  class Manager
  {
  public:
    Manager();
    ~Manager();
  private:
  };
}  // namespace Ripple::Session

#endif  /* !RIPPLE_SESSION_HPP */
