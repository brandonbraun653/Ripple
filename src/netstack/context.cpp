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
    mSocketList.clear();
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
    if ( ( availableMemory() < cacheSize ) || mSocketList.full() )
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
    mSocketList.push_back( sock );
    mSocketList.sort( Socket::compare );

    return sock;
  }


  void Context::attachNetif( NetIf::INetIf *const netif )
  {
    /*-------------------------------------------------
    Ensure the interface actually exists
    -------------------------------------------------*/
    RT_HARD_ASSERT( netif );
    mNetIf = netif;

    /*-------------------------------------------------
    Register callbacks with the interface
    -------------------------------------------------*/
    mNetIf->registerCallback( NetIf::CallbackId::CB_ERROR_ARP_LIMIT,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_OnARPStorageLimit>( *this ) );

    mNetIf->registerCallback( NetIf::CallbackId::CB_ERROR_ARP_RESOLVE,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_OnARPResolveError>( *this ) );

    mNetIf->registerCallback( NetIf::CallbackId::CB_ERROR_RX_QUEUE_FULL,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_OnRXQueueFull>( *this ) );

    mNetIf->registerCallback( NetIf::CallbackId::CB_ERROR_TX_QUEUE_FULL,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_OnTXQueueFull>( *this ) );

    mNetIf->registerCallback( NetIf::CallbackId::CB_ERROR_TX_FAILURE,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_OnFragmentTXFail>( *this ) );

    mNetIf->registerCallback( NetIf::CallbackId::CB_RX_SUCCESS,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_OnFragmentRX>( *this ) );

    mNetIf->registerCallback( NetIf::CallbackId::CB_TX_SUCCESS,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_OnFragmentTX>( *this ) );

    mNetIf->registerCallback( NetIf::CallbackId::CB_UNHANDLED,
                              etl::delegate<void( size_t )>::create<Context, &Context::cb_Unhandled>( *this ) );
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
    LOG_DEBUG( "Starting Ripple Net Manager\r\n" );

    /*-------------------------------------------------
    Perform the core processing loop
    -------------------------------------------------*/
    while ( true )
    {
      /*-------------------------------------------------
      Process all registered socket queues
      -------------------------------------------------*/
      processRX();
      processTX();
      this_thread::sleep_for( 10 );
    }
  }


  void Context::processRX()
  {
    Chimera::Thread::LockGuard lck( *this );
  }


  void Context::processTX()
  {
    using namespace Aurora::Logging;

    /*-------------------------------------------------
    Check each registered socket for available TX data
    -------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *this );
    for ( auto sock = mSocketList.begin(); sock != mSocketList.end(); sock++ )
    {
      /*-------------------------------------------------
      Nothing ready? Move to the next one.
      -------------------------------------------------*/
      if ( !( *sock )->mTXReady )
      {
        continue;
      }

      /*-------------------------------------------------
      Grab the next fragment list to transmit from the
      socket and validate pointers are ok.
      -------------------------------------------------*/
      Chimera::Thread::LockGuard<Socket> sockLock( *( *sock ) );
      if ( ( *sock )->mTXQueue.empty() )
      {
        continue;
      }

      MsgFrag *msg = nullptr;
      ( *sock )->mTXQueue.pop_into( msg );

      if ( !msg || !msg->fragmentData )
      {
        continue;
      }

      /*-------------------------------------------------
      Save off the original message address to free later
      -------------------------------------------------*/
      MsgFrag *baseMsg = msg;

      /*-------------------------------------------------
      Iterate over each fragment in the list
      -------------------------------------------------*/
      auto sts = Chimera::Status::READY;
      while ( sts == Chimera::Status::READY )
      {
        /*-------------------------------------------------
        Send the current fragment to the network interface
        for transmission. Assumes Netif will copy the data.
        -------------------------------------------------*/
        sts = mNetIf->send( *msg, ( *sock )->mDestAddr );
        if ( !msg->nextFragment )
        {
          break;
        }

        /*-------------------------------------------------
        Move to the next message in the list
        -------------------------------------------------*/
        msg = msg->nextFragment;
      }

      /*-------------------------------------------------
      Release the memory for this packet
      -------------------------------------------------*/
      this->free( baseMsg );

      /*-------------------------------------------------
      Update transmit status
      -------------------------------------------------*/
      ( *sock )->mTXReady = false;
      if ( ( sts != Chimera::Status::OK ) && ( sts != Chimera::Status::READY ) )
      {
        LOG_DEBUG( "Failed TX to netif\r\n" );
      }
    }
  }


  void Context::cb_Unhandled( size_t callbackID )
  {
    LOG_ERROR( "NetIf unhandled callback id: %d\r\n", callbackID );
  }


  void Context::cb_OnFragmentTX( size_t callbackID )
  {

  }


  void Context::cb_OnFragmentRX( size_t callbackID )
  {
    LOG_INFO( "NetIf received fragment\r\n" );
  }


  void Context::cb_OnFragmentTXFail( size_t callbackID )
  {

  }


  void Context::cb_OnRXQueueFull( size_t callbackID )
  {

  }


  void Context::cb_OnTXQueueFull( size_t callbackID )
  {

  }


  void Context::cb_OnARPResolveError( size_t callbackID )
  {

  }


  void Context::cb_OnARPStorageLimit( size_t callbackID )
  {

  }

}    // namespace Ripple
