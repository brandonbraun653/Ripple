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
#include <Ripple/netstack>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t UUID_NO_REMOVE = 0xFFFFFFFF;

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
    Chimera::Thread::LockGuard<Context>( *this );

    /*-------------------------------------------------
    Input protections
    -------------------------------------------------*/
    if ( !mNetIf )
    {
      return;
    }

    /*-------------------------------------------------
    Prune the fragment assembly tree
    -------------------------------------------------*/
    // TODO: Look for stale packets and remove them
    // TODO: Look for complete packets and push them to a socket (use network layer header)

    std::array<uint32_t, 16> uuidToRemove;
    uuidToRemove.fill( UUID_NO_REMOVE );
    uint8_t removeIdx = 0;

    for( auto &assemblyItem : mFragAssembly )
    {
      /*-------------------------------------------------
      Is this item currently assembling a frame?
      -------------------------------------------------*/
      if( !assemblyItem.second.inProgress )
      {
        continue;
      }

      /*-------------------------------------------------
      Is the assembly stale? Flag it for removal.
      -------------------------------------------------*/
      if( ( assemblyItem.second.lastFragmentTime - assemblyItem.second.startRxTime ) >= assemblyItem.second.timeout )
      {
        uuidToRemove[ removeIdx ] = assemblyItem.first;
        removeIdx++;
      }

      /*-------------------------------------------------
      Have all packets been acquired?
      -------------------------------------------------*/
      if( assemblyItem.second.bytesRcvd == assemblyItem.second.fragment->totalLength)
      {
        assemblyItem.second.inProgress = false;

        /*-------------------------------------------------
        Sort the fragments
        -------------------------------------------------*/
        sortFragments( &assemblyItem.second.fragment );

        /*-------------------------------------------------
        Fragments are now sorted, so the first fragment
        should be the packet header.
        -------------------------------------------------*/
        if( ( assemblyItem.second.fragment->fragmentNumber == 0 )
         && ( assemblyItem.second.fragment->fragmentLength == sizeof( TransportHeader ) ) )
        {
          auto *header = reinterpret_cast<TransportHeader*>( assemblyItem.second.fragment->fragmentData );

          /*-------------------------------------------------
          Find the socket associated with the UUID and push
          the message into its receive queue.
          -------------------------------------------------*/
          for ( auto sock = mSocketList.begin(); sock != mSocketList.end(); sock++ )
          {
            /*-------------------------------------------------
            No socket matched the received UUID
            -------------------------------------------------*/
            if( sock == mSocketList.end() )
            {
              LOG_ERROR( "Destination socket %d for packet %d doesn't exist!\r\n", ( *sock )->uuid(), assemblyItem.first );
              uuidToRemove[ removeIdx ] = assemblyItem.first;
              removeIdx++;
            }

            /*-------------------------------------------------
            Found the destination socket
            -------------------------------------------------*/
            if ( ( *sock )->uuid() == header->dstSocketUUID )
            {
              /*-------------------------------------------------
              Packet is about to be dropped?
              -------------------------------------------------*/
              if( ( *sock )->mRXQueue.full() )
              {
                LOG_ERROR( "Destination socket %d receive queue was full\r\n", ( *sock )->uuid() );
                uuidToRemove[ removeIdx ] = assemblyItem.first;
                removeIdx++;
              }

              /*-------------------------------------------------
              Push the root fragment into the queue, effectively
              informing the socket of the entire ordered packet.
              -------------------------------------------------*/
              ( *sock )->mRXQueue.push( assemblyItem.second.fragment );
              break;
            }
          }
        }
        else
        {
          /*-------------------------------------------------
          Something went wrong. Erase the packet.
          -------------------------------------------------*/
          LOG_ERROR( "Fragment sorting error or packet corruption!\r\n" );
          uuidToRemove[ removeIdx ] = assemblyItem.first;
          removeIdx++;
        }
      }
    }

    /*-------------------------------------------------
    Free the memory flagged for removal
    -------------------------------------------------*/
    for( size_t idx = 0; idx < uuidToRemove.size(); idx++ )
    {
      freeFragmentsWithUUID( uuidToRemove[ idx ] );
    }

    /*-------------------------------------------------
    Get all available packets from the netif and push
    them into their respective assembly areas.
    -------------------------------------------------*/
    MsgFrag msg;
    auto state = Chimera::Status::READY;

    while ( state == Chimera::Status::READY )
    {
      state = mNetIf->recv( msg );

      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //               !!!!IMPORTANT!!!!
      // The following only works if the NetIf driver
      // returns a single fragment on each call. Without
      // this, packet fragments will become lost as no
      // connections are made to join the ends of the
      // lists together.
      //
      // Eventually list joining will be supported, but
      // it's not pertinent to the first cut of this SW.
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

      /*-------------------------------------------------
      Does the fragment UUID exist in the assembly area?
      -------------------------------------------------*/
      auto iter = mFragAssembly.find( msg.uuid );
      if ( iter != mFragAssembly.end() )
      {
        MsgFrag *heapMsg = copyFragmentToHeap( msg );

        /*-------------------------------------------------
        Insert the fragment at the front, out of order. The
        sorting process will come later once all packets
        have been received.
        -------------------------------------------------*/
        heapMsg->nextFragment         = iter->second.fragment;
        iter->second.fragment         = heapMsg;
        iter->second.lastFragmentTime = Chimera::millis();
        iter->second.bytesRcvd += heapMsg->fragmentLength;
      }
      else if ( !mFragAssembly.full() )
      {
        MsgFrag *heapMsg = copyFragmentToHeap( msg );

        FragAssem newAssembly;
        newAssembly.inProgress       = true;
        newAssembly.bytesRcvd        = 0;
        newAssembly.fragment         = heapMsg;
        newAssembly.startRxTime      = Chimera::millis();
        newAssembly.lastFragmentTime = newAssembly.startRxTime;

        mFragAssembly.insert( { msg.uuid, newAssembly } );
      }
      else
      {
        LOG_ERROR( "Packet assembly limit reached. Dropped fragment with UUID: %d", msg.uuid );
      }
    }
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


  MsgFrag * Context::copyFragmentToHeap( MsgFrag &msg )
  {
    /*-------------------------------------------------
    Check if enough memory is available to handle the
    raw data + the fragment control structure overhead.
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Context> lock( *this );

    size_t available      = this->availableMemory();
    size_t allocationSize = msg.fragmentLength + ( sizeof( MsgFrag ) );

    if ( available <= allocationSize )
    {
      LOG_DEBUG( "Net context out of memory for fragment assembly\r\n" );
      return nullptr;
    }

    /*-------------------------------------------------
    Allocate memory for the fragment
    -------------------------------------------------*/
    uint8_t *pool = reinterpret_cast<uint8_t *>( this->malloc( allocationSize ) );
    if ( !pool )
    {
      return nullptr;
    }

    const auto endAddr = reinterpret_cast<std::uintptr_t>( pool + allocationSize );

    MsgFrag *heapMsg = new ( pool ) MsgFrag();
    pool += sizeof( MsgFrag );

    /*-------------------------------------------------
    Copy the fragment over
    -------------------------------------------------*/
    *heapMsg              = msg;
    heapMsg->fragmentData = pool;
    memcpy( heapMsg->fragmentData, msg.fragmentData, msg.fragmentLength );
    pool += msg.fragmentLength;

    RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( pool ) <= endAddr );
    return heapMsg;
  }


  void Context::freeFragmentsWithUUID( uint32_t uuid )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if( uuid == UUID_NO_REMOVE )
    {
      return;
    }

    /*-------------------------------------------------
    Find the entry and erase it. Assumes operation
    within a protected (locked) context.
    -------------------------------------------------*/
    auto iter = mFragAssembly.find( uuid );
    if( iter != mFragAssembly.end() )
    {
      mFragAssembly.erase( iter );
    }
  }

}    // namespace Ripple
