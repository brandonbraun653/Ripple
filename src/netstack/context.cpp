/********************************************************************************
 *  File Name:
 *    context.cpp
 *
 *  Description:
 *    Net stack context implementation
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Aurora Includes */
#include <Aurora/logging>

/* Chimera Includes */
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/shared>
#include <Ripple/src/netstack/socket.hpp>
#include <Ripple/src/netstack/context.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Context Class
  -------------------------------------------------------------------------------*/
  Context::Context()
  {
  }


  Context::Context( Aurora::Memory::Heap &&heap ) : mHeap( std::move( heap ) )
  {
    mSocketCB.clear();
  }


  Context::~Context()
  {
  }


  Socket_rPtr Context::socket( const SocketType type, const size_t cacheSize )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( ( cacheSize < sizeof( Socket ) ) || ( ( cacheSize % sizeof( size_t ) ) != 0 ) )
    {
      return nullptr;
    }

    /*-------------------------------------------------
    Check and see if enough memory exists
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Context>( *this );
    if ( ( availableMemory() < cacheSize ) || mSocketCB.full() )
    {
      return nullptr;
    }

    /*-------------------------------------------------
    Construct the socket
    -------------------------------------------------*/
    Socket_rPtr sock = new ( this->malloc( sizeof( Socket ) ) ) Socket( this, type, cacheSize );
    RT_HARD_ASSERT( sock );

    /*-------------------------------------------------
    Register socket with context manager
    -------------------------------------------------*/
    mSocketCB.push_back( sock );
    mSocketCB.sort( Socket::compare );

    return sock;
  }


  void Context::attachNetif( NetIf::INetIf *const netif )
  {
    RT_HARD_ASSERT( netif );
    mNetIf = netif;
  }


  void *Context::malloc( const size_t size )
  {
    Chimera::Thread::LockGuard<Context>( *this );
    void *mem = mHeap.malloc( size );

    if ( !mem )
    {
      mCBService_registry.call<CallbackId::CB_OUT_OF_MEMORY>();

      // TODO: Remove this once debugging is done
      RT_HARD_ASSERT( false );
    }

    return mem;
  }


  void Context::free( void *pv )
  {
    Chimera::Thread::LockGuard<Context>( *this );
    mHeap.free( pv );
  }


  size_t Context::availableMemory() const
  {
    return mHeap.available();
  }


  void Context::ManagerThread( void *arg )
  {
    using namespace Aurora::Logging;
    using namespace Chimera::Thread;

    /*-------------------------------------------------
    Wait for this thread to be told to initialize
    -------------------------------------------------*/
    Ripple::TaskWaitInit();
    this_thread::set_name( "NetMgr" );
    getRootSink()->flog( Level::LVL_DEBUG, "Starting Ripple Net Manager\r\n" );

    /*-------------------------------------------------
    Perform the core processing loop
    -------------------------------------------------*/
    size_t lastWoken = Chimera::millis();

    while ( true )
    {
      /*-------------------------------------------------
      Process all registered socket queues
      -------------------------------------------------*/
      processRX();
      processTX();

      Chimera::Thread::this_thread::yield();
    }
  }


  void Context::processRX()
  {
    Chimera::Thread::LockGuard<Context>( *this );
  }


  void Context::processTX()
  {
    Chimera::Thread::LockGuard<Context>( *this );

    //if( mTXQue)
  }
}    // namespace Ripple
