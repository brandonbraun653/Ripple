/********************************************************************************
 *  File Name:
 *    lbk_adapter.cpp
 *
 *  Description:
 *    Lookback adapter implementation
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Aurora Includes */
#include <Aurora/logging>

/* Chimera Includes */
#include <Chimera/assert>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/netstack>
#include <Ripple/netif/loopback>


namespace Ripple::NetIf::Loopback
{
  // Randomly select when a packet transmit will cause a queue reordering
  // Dump queue into vector, reorder randomly, then put back

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Adapter *createNetIf( Context_rPtr context )
  {
    /*-------------------------------------------------
    Use placement new to allocate a handle on the heap
    -------------------------------------------------*/
    RT_HARD_ASSERT( context );
    void * ptr = context->malloc( sizeof( Adapter ) );
    return new ( ptr ) Adapter();
  }


  /*-------------------------------------------------------------------------------
  Service Class Implementation
  -------------------------------------------------------------------------------*/
  Adapter::Adapter()
  {
  }


  Adapter::~Adapter()
  {
  }


  /*-------------------------------------------------------------------------------
  Service: Net Interface
  -------------------------------------------------------------------------------*/
  bool Adapter::powerUp( void * context )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if( !context )
    {
      return false;
    }

    /*-------------------------------------------------
    Initialize module memory
    -------------------------------------------------*/
    mContext = reinterpret_cast<Context_rPtr>( context );
    mAddressCache.clear();
    mPacketQueue.clear();

    auto lock_mem = mContext->malloc( sizeof( Chimera::Thread::RecursiveMutex ) );
    if( lock_mem )
    {
      mLock = new ( lock_mem ) Chimera::Thread::RecursiveMutex();
      return true;
    }
    else
    {
      return false;
    }
  }


  void Adapter::powerDn()
  {
  }


  Chimera::Status_t Adapter::recv( MsgFrag ** fragmentList )
  {
    Chimera::Thread::LockGuard lck( *mLock );

    /*-------------------------------------------------
    Pop off the first element in the queue
    -------------------------------------------------*/
    if( mPacketQueue.empty() )
    {
      return Chimera::Status::EMPTY;
    }

    *fragmentList = mPacketQueue.front();
    mPacketQueue.pop();

    // /*-------------------------------------------------
    // Dump all remaining data to a local array
    // -------------------------------------------------*/
    // std::array<MsgFrag, LB_QUEUE_DEPTH> tmpArray;
    // int cnt = 0;

    // while( !mPacketQueue.empty() )
    // {
    //   tmpArray[ cnt ] = mPacketQueue.front();
    //   mPacketQueue.pop();
    //   cnt++;
    // }

    // /*-------------------------------------------------
    // Re-shuffle the order of the packets for a naive
    // simulation of complex network data arrival timing.
    // -------------------------------------------------*/
    // while( cnt > 0 )
    // {
    //   int nextChoice = rand() % cnt;
    //   RT_HARD_ASSERT( ( nextChoice >= 0 ) && ( nextChoice < LB_QUEUE_DEPTH ) );

    //   mPacketQueue.push( tmpArray[ nextChoice ] );
    //   tmpArray[ nextChoice ] = {};

    //   cnt--;
    // }

    return Chimera::Status::READY;
  }


  Chimera::Status_t Adapter::send( const MsgFrag *const msg, const IPAddress &ip )
  {
    Chimera::Thread::LockGuard lck( *mLock );

    /*-------------------------------------------------
    Simulate moving memory into the ether and TX-ing
    -------------------------------------------------*/
    size_t allocationSize = Fragment::memoryConsumption( msg );
    void *buffer = mContext->malloc( allocationSize );

    MsgFrag * copiedMessage = Fragment::copyToBuffer( msg, buffer, allocationSize );

    /*-------------------------------------------------
    Push all messages into the queue
    -------------------------------------------------*/
    if( copiedMessage )
    {
      mPacketQueue.push( copiedMessage );
      mContext->free( ( void * )( msg ) );

      return Chimera::Status::OK;
    }
    else
    {
      return Chimera::Status::MEMORY;
    }
  }


  IARP *Adapter::addressResolver()
  {
    return this;
  }


  size_t Adapter::maxTransferSize() const
  {
    return 29; // Simulate packet size of NRF24L01
  }

  size_t Adapter::maxNumFragments() const
  {
    return 512; // Arbitrary. No real limit outside of memory.
  }


  size_t Adapter::linkSpeed() const
  {
    return 1024; // 1kB per second
  }


  size_t Adapter::lastActive() const
  {
    return 0;
  }


  /*-------------------------------------------------------------------------------
  Service: ARP Interface
  -------------------------------------------------------------------------------*/
  Chimera::Status_t Adapter::addARPEntry( const IPAddress &ip, const void *const mac, const size_t size )
  {
    /*-------------------------------------------------
    Copy over the mac address if the size matches
    -------------------------------------------------*/
    if ( size != sizeof( uint64_t ) )
    {
      return Chimera::Status::FAIL;
    }

    uint64_t addr = 0;
    memcpy( &addr, mac, sizeof( uint64_t ) );

    /*-------------------------------------------------
    Attempt to insert the new entry
    -------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *this );
    mAddressCache.insert( { ip, addr } );

    return Chimera::Status::OK;
  }


  Chimera::Status_t Adapter::dropARPEntry( const IPAddress &ip )
  {
    /*-------------------------------------------------
    Does nothing if the entry doesn't exist
    -------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *this );
    auto iter = mAddressCache.find( ip );
    if( iter != mAddressCache.end() )
    {
      mAddressCache.erase( iter );
    }

    return Chimera::Status::OK;
  }


  bool Adapter::arpLookUp( const IPAddress &ip, void *const mac, const size_t size )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !mac || ( size != sizeof( uint64_t ) ) )
    {
      return false;
    }

    Chimera::Thread::LockGuard lck( *this );
    auto iter = mAddressCache.find( ip );
    if( iter != mAddressCache.end() )
    {
      memcpy( mac, &iter->second, size );
      return true;
    }

    return false;
  }


  IPAddress Adapter::arpLookUp( const void *const mac, const size_t size )
  {
    // Currently not supported but might be in the future?
    RT_HARD_ASSERT( false );
    return 0;
  }

}  // namespace Ripple::NetIf::Loopback
