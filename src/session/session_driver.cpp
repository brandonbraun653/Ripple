/********************************************************************************
 *  File Name:
 *    session_driver.cpp
 *
 *  Description:
 *    Implementation of the session layer interface
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/session>

namespace Ripple::Session
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Handle *getHandle( Context context )
  {
    return reinterpret_cast<Handle *>( context );
  }

}  // namespace Ripple::Session
