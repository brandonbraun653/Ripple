/********************************************************************************
 *  File Name:
 *    cmn_utils.cpp
 *
 *  Description:
 *    Implementation of the Ripple shared utility functions
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/shared/cmn_utils.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  IPAddress constructIP( const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d )
  {
    return ( ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | ( d << 0 ) );
  }


  void TaskWaitInit()
  {
    using namespace Chimera::Thread;

    /*-------------------------------------------------
    Wait for task registration to complete. On the sim
    this happens so fast that trying to receive a task
    message will cause a fault. This is due to tasks
    starting at creation on PCs, but not on embedded.
    -------------------------------------------------*/
#if defined( SIMULATOR )
    Chimera::delayMilliseconds( 10 );
#endif

    /*-------------------------------------------------
    Wait for the expected task message to arrive
    -------------------------------------------------*/
    TaskMsg msg = ITCMsg::TSK_MSG_NOP;
    while ( true )
    {
      if ( this_thread::receiveTaskMsg( msg, TIMEOUT_BLOCK ) && ( msg == ITCMsg::TSK_MSG_WAKEUP ) )
      {
        break;
      }
      else
      {
        this_thread::yield();
#if defined( SIMULATOR )
        Chimera::delayMilliseconds( 5 );
#endif
      }
    }
  }

}    // namespace Ripple
