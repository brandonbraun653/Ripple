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

/*-------------------------------------------------------------------------------
Literals
-------------------------------------------------------------------------------*/
#define DEBUG_MODULE ( false )


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


  void Context::setIPAddress( const IPAddress address )
  {
    mIP = address;
  }


  IPAddress Context::getIPAddress()
  {
    return mIP;
  }


  Socket_rPtr Context::socket( const SocketType type, const size_t cacheSize )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( ( cacheSize < sizeof( Socket ) ) || ( ( cacheSize % sizeof( size_t ) ) != 0 ) )
    {
      LOG_DEBUG_IF( DEBUG_MODULE, "Cache size of %d is too small for socket of size %d!\r\n", cacheSize, sizeof( Socket ) );
      return nullptr;
    }

    /*-------------------------------------------------
    Check and see if enough memory exists
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Context> _ctxLock( *this );
    if ( ( availableMemory() < cacheSize ) || mSocketList.full() )
    {
      LOG_DEBUG_IF( DEBUG_MODULE, "Out of memory to create socket!\r\n" );
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
    Chimera::Thread::LockGuard<Context> _ctxLock( *this );
    void *mem = mHeap.malloc( size );

    if ( !mem )
    {
      mCBService_registry.call<CallbackId::CB_OUT_OF_MEMORY>();
    }

    return mem;
  }


  void Context::free( void *pv )
  {
    Chimera::Thread::LockGuard<Context> _ctxLock( *this );
    mHeap.free( pv );
  }


  size_t Context::availableMemory() const
  {
    return mHeap.available();
  }


  void Context::printStats()
  {
    /*-------------------------------------------------------------------------
    Grab the latest stats
    -------------------------------------------------------------------------*/
    NetIf::PerfStats stats;
    mNetIf->getStats( stats );

    /*-------------------------------------------------------------------------
    Format a string for printing to the console
    -------------------------------------------------------------------------*/
    char buf[ 512 ];
    memset( buf, 0, ARRAY_BYTES( buf ) );

    snprintf( buf, ARRAY_BYTES( buf ),
      "\r\n\tRX:\tbytes\tframes\tspeed\tdropped\tlost"
      "\r\n\t\t%ld\t%ld\t%ld\t%ld\t%ld"
      "\r\n\tTX:\tbytes\tframes\tspeed\tdropped\tlost"
      "\r\n\t\t%ld\t%ld\t%ld\t%ld\t%ld"
      "\r\n"
      ,
      stats.rx_bytes, stats.frame_rx, stats.link_speed_rx, stats.frame_rx_drop, stats.rx_bytes_lost,
      stats.tx_bytes, stats.frame_tx, stats.link_speed_tx, stats.frame_tx_drop, stats.tx_bytes_lost );

    LOG_INFO( buf );
  }


  void Context::ManagerThread( void *arg )
  {
    using namespace Aurora::Logging;
    using namespace Chimera::Thread;

    /*-------------------------------------------------------------------------
    Wait for this thread to be told to initialize
    -------------------------------------------------------------------------*/
    Ripple::TaskWaitInit();
    this_thread::set_name( "NetMgr" );
    LOG_DEBUG_IF( DEBUG_MODULE, "Starting Ripple Net Manager\r\n" );

    /*-------------------------------------------------------------------------
    Initialize the fragment assembly workspace
    -------------------------------------------------------------------------*/
    for ( auto &assemblyItem : mPacketAssembly )
    {
      assemblyItem.second.inProgress = false;
    }

    /*-------------------------------------------------------------------------
    Perform the core processing loop
    -------------------------------------------------------------------------*/
    while ( true )
    {
      processRX();
      processTX();
      this_thread::sleep_for( 10 );
    }
  }


  void Context::processRX()
  {
    /*-----------------------------------------------------------------
    Input Protections
    -----------------------------------------------------------------*/
    if ( !mNetIf )
    {
      return;
    }

    /*-----------------------------------------------------------------
    Process in-progress RX'd fragments first, then pull any waiting
    fragments from the network interface driver.
    -----------------------------------------------------------------*/
    Chimera::Thread::LockGuard<Context> _ctxLock( *this );
    unsafe_processRXFrags();
    unsafe_pumpRXFrags();

    /*-----------------------------------------------------------------
    Process all socket periodic functionality
    -----------------------------------------------------------------*/
    for ( auto sock = mSocketList.begin(); sock != mSocketList.end(); sock++ )
    {
      ( *sock )->processData();
    }
  }


  void Context::processTX()
  {
    /*-------------------------------------------------------------------------
    Input Protections
    -------------------------------------------------------------------------*/
    if ( !mNetIf )
    {
      return;
    }

    /*-------------------------------------------------------------------------
    Check each registered socket for available TX data
    -------------------------------------------------------------------------*/
    for ( auto sock = mSocketList.begin(); sock != mSocketList.end(); sock++ )
    {
      Chimera::Thread::LockGuard<Socket> _sckLock( *( *sock ) );

      /*-----------------------------------------------------------------------
      Grab the next fragment list to transmit until no more exist
      -----------------------------------------------------------------------*/
      while( !( *sock )->mTXQueue.empty() )
      {
        Packet_sPtr msg;
        ( *sock )->mTXQueue.pop_into( msg );

        /*---------------------------------------------------------------------
        Move the data into the network interface driver
        ---------------------------------------------------------------------*/
        auto sts = mNetIf->send( msg->head, ( *sock )->mDestAddress );
        if ( ( sts != Chimera::Status::OK ) && ( sts != Chimera::Status::READY ) )
        {
          LOG_DEBUG_IF( DEBUG_MODULE, "Failed TX to netif\r\n" );
        }
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


  /**
   * @brief Removes dead packets from the assembly area
   */
  void Context::unsafe_pruneRXFrags()
  {
    /*-------------------------------------------------------------------------
    Local Variables
    -------------------------------------------------------------------------*/
    std::array<uint32_t, RIPPLE_CTX_MAX_PKT> uuidToRemove;
    uint8_t removeIdx = 0;

    /*-------------------------------------------------------------------------
    Find all packets that are expired, whether explicitly or via timeout.
    -------------------------------------------------------------------------*/
    uuidToRemove.fill( UUID_NO_REMOVE );
    for ( auto &assemblyItem : mPacketAssembly )
    {
      PacketAssembly *const assembly = &assemblyItem.second;
      const uint32_t uuid            = assemblyItem.first;
      const uint32_t lifetime        = assembly->lastTimeoutCheck - assembly->startRxTime;

      if ( lifetime >= assembly->timeout )
      {
        assembly->remove    = true;
        assembly->whyRemove = PacketAssembly::RemoveErr::TIMEOUT;
      }

      if ( assembly->remove )
      {
        uuidToRemove[ removeIdx ] = uuid;
        removeIdx++;
      }
      else
      {
        assembly->lastTimeoutCheck = Chimera::millis();
      }
    }

    /*-------------------------------------------------------------------------
    Free the memory flagged for removal
    -------------------------------------------------------------------------*/
    for ( size_t idx = 0; idx < removeIdx; idx++ )
    {
      auto iter = mPacketAssembly.find( uuidToRemove[ idx ] );
      if ( iter != mPacketAssembly.end() )
      {
        /*---------------------------------------------------------------------
        Log the reason for removal. Very useful for post mortem debugging.
        ---------------------------------------------------------------------*/
        PacketAssembly *const assembly = &iter->second;
        if( assembly->whyRemove != PacketAssembly::RemoveErr::COMPLETED )
        {
          LOG_DEBUG_IF( DEBUG_MODULE, "Abnormal assembly removal of UUID [%d]: %s\r\n", uuidToRemove[ idx ], assembly->whyRemoveString() );
        }

        mPacketAssembly.erase( iter );
      }
    }

    LOG_TRACE_IF( ( DEBUG_MODULE && removeIdx ), "Pruned %d packets from assembly\r\n", removeIdx );
  }


  /**
   * @brief Process packet assembly for completed or malformed packets.
   *
   * Acts as a message pump of sorts. Completed fragments are inspected to
   * determine which destination socket on the network stack the packet will be
   * assigned to, then pushed in to the appropriate RX queue.
   */
  void Context::unsafe_processRXFrags()
  {
    /*-------------------------------------------------------------------------
    Clean the assembly area to prepare for new frags
    -------------------------------------------------------------------------*/
    unsafe_pruneRXFrags();

    /*-------------------------------------------------------------------------
    Iterate over each possible assembly task and update their status. If ready,
    push to the RX queue of the destination socket.
    -------------------------------------------------------------------------*/
    for ( auto &assemblyItem : mPacketAssembly )
    {
      PacketAssembly *const assembly = &assemblyItem.second;

      /*-----------------------------------------------------------------------
      Check for a completed packet
      -----------------------------------------------------------------------*/
      if ( !assembly->inProgress || assembly->packet->isMissingFragments() )
      {
        continue;
      }

      /*-----------------------------------------------------------------------
      All fragments received. Perform basic validity checks on whole packet.
      -----------------------------------------------------------------------*/
      assembly->inProgress = false;
      assembly->packet->sort();

      if ( !assembly->packet->isFullyComposed() )
      {
        assembly->remove    = true;
        assembly->whyRemove = PacketAssembly::RemoveErr::CORRUPTION;
        continue;
      }

      /*-----------------------------------------------------------------------
      Find the socket associated with the packet, then push it to the RX queue
      -----------------------------------------------------------------------*/
      void **raw_data = assembly->packet->head->data.get();
      auto header     = reinterpret_cast<TransportHeader *>( *raw_data );

      for ( auto sock = mSocketList.begin(); /* Check exit below*/; sock++ )
      {
        /*---------------------------------------------------------------------
        No socket matched the destination port. Exit the loop here. Note that
        only sockets which are PULL type can receive data.
        ---------------------------------------------------------------------*/
        if ( sock == mSocketList.end() )
        {
          assembly->remove    = true;
          assembly->whyRemove = PacketAssembly::RemoveErr::SOCK_NOT_FOUND;
          break;
        }

        /*---------------------------------------------------------------------
        Found the destination socket. Right type and right port.
        ---------------------------------------------------------------------*/
        if ( ( ( *sock )->type() == SocketType::PULL ) && ( ( *sock )->port() == header->dstPort ) )
        {
          /*-------------------------------------------------------------------
          Check for enough room in the RX queue
          -------------------------------------------------------------------*/
          if ( ( *sock )->mRXQueue.full() )
          {
            assembly->remove    = true;
            assembly->whyRemove = PacketAssembly::RemoveErr::SOCK_Q_FULL;
            break;
          }

          /*-------------------------------------------------------------------
          Push the root fragment into the queue, effectively informing the
          socket of the entire ordered packet.
          -------------------------------------------------------------------*/
          ( *sock )->mRXQueue.push( assembly->packet );
          assembly->remove    = true;
          assembly->whyRemove = PacketAssembly::RemoveErr::COMPLETED;
          break;
        }
      }
    }
  }


  /**
   * @brief Moves received data from hardware driver to net stack
   *
   * Acts as a message pump to communicate with the device driver, pulling out
   * any waiting message fragments and pushing them into the appropriate area
   * for assembly into a full message.
   */
  void Context::unsafe_pumpRXFrags()
  {
    /*-------------------------------------------------------------------------
    Local Variables
    -------------------------------------------------------------------------*/
    Fragment_sPtr fragList;              /**< Temp var for HW assembled data */
    auto state = Chimera::Status::READY; /**< Reported HW state */

    /*-------------------------------------------------------------------------
    Clean the assembly area to prepare for new frags
    -------------------------------------------------------------------------*/
    unsafe_pruneRXFrags();

    /*-------------------------------------------------------------------------
    Keep pulling in data while NetIf says it's ready
    -------------------------------------------------------------------------*/
    while ( state == Chimera::Status::READY )
    {
      /*-----------------------------------------------------------------------
      Try to receive a message from the hardware driver
      -----------------------------------------------------------------------*/
      fragList = Fragment_sPtr();
      state    = mNetIf->recv( fragList );

      if ( ( state != Chimera::Status::READY ) || !fragList )
      {
        continue;
      }

      /*-----------------------------------------------------------------------
      Pull each fragment from the list and push it to the packet assembly area.
      -----------------------------------------------------------------------*/
      while ( fragList )
      {
        /*---------------------------------------------------------------------
        Cache the next item in the list for later
        ---------------------------------------------------------------------*/
        Fragment_sPtr nextFragment = fragList->next;
        fragList->next             = Fragment_sPtr();

        /*---------------------------------------------------------------------
        Does the fragment UUID exist in the assembly area?
        ---------------------------------------------------------------------*/
        auto iter = mPacketAssembly.find( fragList->uuid );
        if ( iter != mPacketAssembly.end() )
        {
          LOG_TRACE_IF( DEBUG_MODULE, "Received fragment UUID: %d\r\n", fragList->uuid );

          /*-------------------------------------------------------------------
          Does this particular packet already exist in the assembly? Iterate
          with pointers to avoid constructor costs.
          -------------------------------------------------------------------*/
          Fragment_sPtr* frag = &iter->second.packet->head;
          bool isDuplicate    = false;
          while ( *frag )
          {
            if ( fragList->number == ( *frag )->number )
            {
              LOG_ERROR_IF( DEBUG_MODULE, "Got duplicate fragment %d for UUID %d \r\n", fragList->number, fragList->uuid );
              isDuplicate = true;
              break; // First while
            }

            frag = &( *frag )->next;
          }

          /*-------------------------------------------------------------------
          Insert the fragment at the front, out of order. The sorting process
          will come later once all packets have been received.
          -------------------------------------------------------------------*/
          if ( !isDuplicate )
          {
            Fragment_sPtr prevAssemblyFragPtr = iter->second.packet->head;

            iter->second.packet->head       = fragList;
            iter->second.packet->head->next = prevAssemblyFragPtr;
            iter->second.bytesRcvd += fragList->length;
          }
        }
        else if ( !mPacketAssembly.full() )
        {
          /*-------------------------------------------------------------------
          Allocate memory for a new assembly
          -------------------------------------------------------------------*/
          PacketAssembly newAssembly( &this->mHeap );

          newAssembly.inProgress         = true;
          newAssembly.remove             = false;
          newAssembly.bytesRcvd          = fragList->length;
          newAssembly.packet->head       = fragList;
          newAssembly.startRxTime        = Chimera::millis();
          newAssembly.lastTimeoutCheck   = newAssembly.startRxTime;
          newAssembly.timeout            = RIPPLE_PKT_LIFETIME;
          newAssembly.packet->head->next = Fragment_sPtr();

          /*-------------------------------------------------------------------
          Need to explicitly move the newly created assembly else some memory
          issues occur with transferring the internal shared_ptr objects.
          -------------------------------------------------------------------*/
          std::pair<const uint32_t, Ripple::PacketAssembly> x{ fragList->uuid, std::move( newAssembly ) };
          mPacketAssembly.insert( std::move( x ) );

          LOG_TRACE_IF( DEBUG_MODULE, "Starting assembly for UUID: %d\r\n", fragList->uuid );
        }
        else
        {
          LOG_ERROR( "Packet assembly limit [%d] reached. Dropped fragment with UUID: %d\r\n", mPacketAssembly.MAX_SIZE,
                     fragList->uuid );
        }

        /*---------------------------------------------------------------------
        Start parsing the next fragment
        ---------------------------------------------------------------------*/
        fragList = nextFragment;
      }
    }
  }

}    // namespace Ripple
