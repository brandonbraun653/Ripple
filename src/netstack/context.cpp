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
    Initialize the fragment assembly workspace
    -------------------------------------------------*/
    for ( auto &assemblyItem : mPacketAssembly )
    {
      assemblyItem.second.inProgress = false;
    }

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
    /*-------------------------------------------------
    Input protections
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Context>( *this );

    if ( !mNetIf )
    {
      return;
    }

    /*-------------------------------------------------
    Perform RX procedures
    -------------------------------------------------*/
    pruneStaleRXFragments();
    processRXPacketAssembly();
    acquireFragments();
  }


  void Context::processTX()
  {
    using namespace Aurora::Logging;

    /*-------------------------------------------------
    Input protections
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Context>( *this );

    if ( !mNetIf )
    {
      return;
    }

    /*-------------------------------------------------
    Check each registered socket for available TX data
    -------------------------------------------------*/
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
      Grab the next fragment list to transmit, validating
      if the data pointers are ok.
      -------------------------------------------------*/
      Chimera::Thread::LockGuard<Socket> sockLock( *( *sock ) );
      if ( ( *sock )->mTXQueue.empty() )
      {
        continue;
      }

      Packet_sPtr msg;
      ( *sock )->mTXQueue.pop_into( msg );

      /*-------------------------------------------------
      Move the data into the network interface
      -------------------------------------------------*/
      auto sts = mNetIf->send( msg->head, ( *sock )->mDestAddress );

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
    //LOG_INFO( "NetIf received fragment\r\n" );
  }


  void Context::cb_OnFragmentTXFail( size_t callbackID )
  {
    //LOG_ERROR( "NetIf fragment TX fail\r\n" );
  }


  void Context::cb_OnRXQueueFull( size_t callbackID )
  {
    //LOG_ERROR( "NetIf RX queue full\r\n" );
  }


  void Context::cb_OnTXQueueFull( size_t callbackID )
  {
    //LOG_ERROR( "NetIf TX queue full\r\n" );
  }


  void Context::cb_OnARPResolveError( size_t callbackID )
  {
    //LOG_ERROR( "NetIf ARP resolve error\r\n" );
  }


  void Context::cb_OnARPStorageLimit( size_t callbackID )
  {
    //LOG_ERROR( "NetIf storage limit reached\r\n" );
  }


  void Context::pruneStaleRXFragments()
  {
    std::array<uint32_t, 16> uuidToRemove;
    uuidToRemove.fill( UUID_NO_REMOVE );
    uint8_t removeIdx = 0;

    /*-------------------------------------------------
    Iterate over all the current assembly operations
    -------------------------------------------------*/
    for ( auto &assemblyItem : mPacketAssembly )
    {
      PacketAssemblyCB *const assembly = &assemblyItem.second;
      const uint32_t uuid              = assemblyItem.first;

      /*-------------------------------------------------
      Mark for removal if explicitly told to or if the
      packet has timed out.
      -------------------------------------------------*/
      if ( assembly->remove ||
           ( assembly->inProgress && ( ( assembly->lastTimeoutCheck - assembly->startRxTime ) >= assembly->timeout ) ) )
      {
        uuidToRemove[ removeIdx ] = uuid;
        removeIdx++;
      }

      /*-------------------------------------------------
      Update the timeout check
      -------------------------------------------------*/
      assembly->lastTimeoutCheck = Chimera::millis();
    }

    /*-------------------------------------------------
    Free the memory flagged for removal. Assumes
    operation within a protected (locked) context.
    -------------------------------------------------*/
    for ( size_t idx = 0; idx < removeIdx; idx++ )
    {
      auto iter = mPacketAssembly.find( uuidToRemove[ idx ] );
      if ( iter != mPacketAssembly.end() )
      {
        mPacketAssembly.erase( iter );
      }
    }

    LOG_IF_DEBUG( ( removeIdx == 0 ), "Pruned %d packets from assembly\r\n", removeIdx );
  }


  void Context::processRXPacketAssembly()
  {
    /*-------------------------------------------------
    Iterate over each possible assembly task
    -------------------------------------------------*/
    for ( auto &assemblyItem : mPacketAssembly )
    {
      PacketAssemblyCB *const assembly = &assemblyItem.second;
      const uint32_t uuid              = assemblyItem.first;

      /*-------------------------------------------------
      Is this item not assembling anything or not all
      bytes have been received?
      -------------------------------------------------*/
      if ( !assembly->inProgress /* TODO: || packet is missing fragments */)
      {
        continue;
      }

      /*-------------------------------------------------
      All bytes received. Sort the packets, erasing them
      if the packet doesn't have the proper structure.
      -------------------------------------------------*/
      assembly->inProgress = false;
      assembly->packet->sort();

      assembly->packet->printPayload();

      /*-------------------------------------------------
      Double check the first fragment is large enough to
      contain the header.
      -------------------------------------------------*/
      if ( ( assembly->packet->head->number != 0 ) || ( assembly->packet->head->length < sizeof( TransportHeader ) ) )
      {
        assembly->remove = true;
        LOG_ERROR( "Invalid fragment header\r\n" );
        continue;
      }

      /*-------------------------------------------------
      Find the socket associated with the UUID and push
      the message into its receive queue.
      -------------------------------------------------*/
      void **raw_data = assembly->packet->head->data.get();
      auto header = reinterpret_cast<TransportHeader *>( *raw_data );

      for ( auto sock = mSocketList.begin(); sock != mSocketList.end(); sock++ )
      {
        /*-------------------------------------------------
        No socket matched the received UUID
        -------------------------------------------------*/
        if ( sock == mSocketList.end() )
        {
          LOG_ERROR( "Destination port %d for packet %d doesn't exist!\r\n", ( *sock )->port(), uuid );
          assembly->remove = true;
          break;
        }

        /*-------------------------------------------------
        Found the destination socket
        -------------------------------------------------*/
        if ( ( ( *sock )->type() == SocketType::PULL ) && ( ( *sock )->port() == header->dstPort ) )
        {
          /*-------------------------------------------------
          Check for enough room in the RX queue
          -------------------------------------------------*/
          if ( ( *sock )->mRXQueue.full() )
          {
            LOG_ERROR( "Destination port %d receive queue was full\r\n", ( *sock )->port() );
            assembly->remove = true;
            break;
          }

          /*-------------------------------------------------
          Push the root fragment into the queue, effectively
          informing the socket of the entire ordered packet.
          -------------------------------------------------*/
          ( *sock )->mRXQueue.push( assembly->packet );
          assembly->remove = true;
          LOG_DEBUG( "Received packet on port %d\r\n", ( *sock )->port() );
          break;
        }
      }
    }
  }


  void Context::acquireFragments()
  {
    Fragment_sPtr list;
    auto state = Chimera::Status::READY;

    while ( state == Chimera::Status::READY )
    {
      /*-------------------------------------------------
      Try to receive a message
      -------------------------------------------------*/
      list = Fragment_sPtr();
      state = mNetIf->recv( list );

      if ( ( state != Chimera::Status::READY ) || !list )
      {
        continue;
      }

      /*-------------------------------------------------
      Extract each fragment from the list and push it to
      its respective packet assembly area.
      -------------------------------------------------*/
      while ( list )
      {
        /*-------------------------------------------------
        Cache the next item in the list for later.
        -------------------------------------------------*/
        Fragment_sPtr nextFragment = list->next;
        list->next                 = Fragment_sPtr();

        /*-------------------------------------------------
        Does the fragment UUID exist in the assembly area?
        -------------------------------------------------*/
        auto iter = mPacketAssembly.find( list->uuid );
        if ( iter != mPacketAssembly.end() )
        {
          /*-------------------------------------------------
          Does this packet already exist in the assembly?
          -------------------------------------------------*/
          Fragment_sPtr frag = iter->second.packet->head;
          bool isDuplicate   = false;
          while( frag )
          {
            if( list->number == frag->number )
            {
              isDuplicate = true;
              break;
            }

            frag = frag->next;
          }

          /*-------------------------------------------------
          Insert the fragment at the front, out of order. The
          sorting process will come later once all packets
          have been received.
          -------------------------------------------------*/
          if( !isDuplicate )
          {
            Fragment_sPtr prevAssemblyFragPtr = iter->second.packet->head;

            iter->second.packet->head         = list;
            iter->second.packet->head->next   = prevAssemblyFragPtr;
            iter->second.bytesRcvd += list->length;
          }
        }
        else if ( !mPacketAssembly.full() )
        {
          PacketAssemblyCB newAssembly( this );

          newAssembly.inProgress       = true;
          newAssembly.remove           = false;
          newAssembly.bytesRcvd        = list->length;
          newAssembly.packet->head     = list;
          newAssembly.startRxTime      = Chimera::millis();
          newAssembly.lastTimeoutCheck = newAssembly.startRxTime;
          newAssembly.timeout          = 500 * Chimera::Thread::TIMEOUT_1MS;

          /*-------------------------------------------------
          First item in the list. Ensure it's terminated.
          -------------------------------------------------*/
          newAssembly.packet->head->next = Fragment_sPtr();
          mPacketAssembly.insert( { list->uuid, newAssembly } );
        }
        else
        {
          LOG_ERROR( "Packet assembly limit reached. Dropped fragment with UUID: %d\r\n", list->uuid );
        }

        /*-------------------------------------------------
        Start parsing the next fragment
        -------------------------------------------------*/
        list = nextFragment;
      }
    }
  }


}    // namespace Ripple
