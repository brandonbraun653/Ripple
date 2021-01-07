/********************************************************************************
 *  File Name:
 *    session_driver.cpp
 *
 *  Description:
 *    Implementation of the session layer interface
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/assert>
#include <Chimera/common>
#include <Chimera/system>
#include <Chimera/thread>

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

  /*-------------------------------------------------------------------------------
  Service Class Implementation
  -------------------------------------------------------------------------------*/
  Service::Service() :
      mContext( nullptr ), mUpdateRate( Chimera::Threading::TIMEOUT_50MS ),
      mServiceStarvedThreshold( 2 * Chimera::Threading::TIMEOUT_1MS )
  {
  }


  Service::~Service()
  {
  }


  void Service::run( Session::Context context )
  {
    using namespace Chimera::Threading;

    /*-------------------------------------------------
    Wait for this thread to be told to initialize
    -------------------------------------------------*/
    this_thread::pendTaskMsg( TSK_MSG_WAKEUP );
    mThreadId = this_thread::id();

    /*-------------------------------------------------
    Verify the handles used in the entire DataLink
    service have been registered correctly.
    -------------------------------------------------*/
    Session::Handle *session = Session::getHandle( context );

    if ( !session )
    {
      Chimera::System::softwareReset();
    }
    else
    {
      mContext = context;
    }

    /*-------------------------------------------------
    Register each process and initialize them
    -------------------------------------------------*/
    registerProcess();
    initializeProcess();

    /*-------------------------------------------------
    Run the service
    -------------------------------------------------*/
    size_t lastWakeup  = 0;
    size_t pendTime    = mUpdateRate;
    size_t nextWakeup  = 0;
    size_t currentTick = 0;

    while( 1 )
    {
      /*-------------------------------------------------
      Block this thread until periodic process timeout
      expires or another thread wakes this one up.
      -------------------------------------------------*/
      this_thread::pendTaskMsg( ITCMsg::TSK_MSG_WAKEUP, pendTime );
      lastWakeup = Chimera::millis();
      nextWakeup = lastWakeup + mUpdateRate;

      /*-------------------------------------------------
      Process the session services state machine
      -------------------------------------------------*/
      this->lock();
      for( auto &pcb : mProcessCB )
      {
        pcb.status = pcb.controller->getStatus();

        switch( pcb.status )
        {
          case Process::Status::IDLE:
            continue;
            break;

          case Process::Status::INITIALIZE:
            pcb.controller->initialize();
            break;

          case Process::Status::RUNNING:
            pcb.controller->update();
            break;

          case Process::Status::COMPLETE:
            pcb.controller->close();
            break;

          case Process::Status::ERROR:
            pcb.controller->onError();
            break;

          default:
            RT_HARD_ASSERT( false );
            break;
        };
      }
      this->unlock();

      /*-------------------------------------------------
      Calculate the next time this thread should wake.
      -------------------------------------------------*/
      currentTick = Chimera::millis();
      pendTime    = nextWakeup - currentTick;

      /*-------------------------------------------------
      Notify a listener if the thread is doing too much
      in its time slice or was interrupted by a higher
      priority thread that consumed too much time.

      This callback firing is more of a warning bell then
      a sure-fire sign that there is a problem.
      -------------------------------------------------*/
      if ( ( currentTick >= nextWakeup ) || ( pendTime <= mServiceStarvedThreshold ) )
      {
        mDelegateRegistry.call<CallbackId::CB_SERVICE_OVERRUN>();
      }
    }
  }


  void Service::setUpdateRate( const size_t period )
  {
    this->lock();
    mUpdateRate = period;
    this->unlock();
  }


  Chimera::Status_t Service::registerCallback( const CallbackId id, etl::delegate<void( size_t )> func )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !( id < CallbackId::CB_NUM_OPTIONS ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Register the callback
    -------------------------------------------------*/
    this->lock();
    if ( id == CallbackId::CB_UNHANDLED )
    {
      mDelegateRegistry.register_unhandled_delegate( func );
    }
    else
    {
      mDelegateRegistry.register_delegate( id, func );
    }
    this->unlock();
    return Chimera::Status::OK;
  }


  void Service::registerProcess()
  {
    /*-------------------------------------------------
    Process: Join Network
    -------------------------------------------------*/
    Process::Type procType         = Process::Type::JOIN_NETWORK;
    size_t index                   = static_cast<size_t>( procType );
    mProcessCB[ index ].type       = procType;
    mProcessCB[ index ].controller = &mProc_JoinNetwork;

    /*-------------------------------------------------
    Process: Establish Connection
    -------------------------------------------------*/
    procType                       = Process::Type::ESTABLISH_CONNECTION;
    index                          = static_cast<size_t>( procType );
    mProcessCB[ index ].type       = procType;
    mProcessCB[ index ].controller = &mProc_EstablishConn;

    /*-------------------------------------------------
    Process: Terminate Connection
    -------------------------------------------------*/
    procType                       = Process::Type::TERMINATE_CONNECTION;
    index                          = static_cast<size_t>( procType );
    mProcessCB[ index ].type       = procType;
    mProcessCB[ index ].controller = &mProc_TerminateConn;
  }


  void Service::initializeProcess()
  {
    for ( auto &pcb : mProcessCB )
    {
      /*-------------------------------------------------
      Make sure all controllers were registered
      -------------------------------------------------*/
      RT_HARD_ASSERT( pcb.controller );

      /*-------------------------------------------------
      All processes are idle at init
      -------------------------------------------------*/
      pcb.status = Process::Status::IDLE;
    }
  }

}  // namespace Ripple::Session
