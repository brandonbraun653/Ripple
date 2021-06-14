/********************************************************************************
 *  File Name:
 *    socket.cpp
 *
 *  Description:
 *    Implementation of the network socket type
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* STL Includes */
#include <algorithm>
#include <cstdint>
#include <cstddef>

/* ETL Includes */
#include <etl/random.h>

/* Aurora Includes */
#include <Aurora/logging>

/* Chimera Assert */
#include <Chimera/assert>

/* Ripple Includes */
#include <Ripple/shared>
#include <Ripple/netstack>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Static Data
  -------------------------------------------------------------------------------*/
  static etl::random_xorshift s_rng;

  /*-------------------------------------------------------------------------------
  Socket Class
  -------------------------------------------------------------------------------*/
  Socket::Socket( Context_rPtr ctx, const SocketType type, const size_t memory ) : mContext( ctx ), mSocketType( type )
  {
    using namespace Chimera::Thread;
    RT_HARD_ASSERT( ctx );
    RT_HARD_ASSERT( memory );

    maxMem   = memory;
    allocMem = 0;
    mTXReady = false;
    mTXQueue.clear();
    mRXQueue.clear();

    mLock = new ( ctx->malloc( sizeof( RecursiveMutex ) ) ) RecursiveMutex();
  }

  Socket::~Socket()
  {
    mContext->free( mLock );
  }


  Chimera::Status_t Socket::open( const Port port )
  {
    /*-------------------------------------------------
    Initialize the socket
    -------------------------------------------------*/
    s_rng.initialise( Chimera::millis() );
    mThisPort = port;

    return Chimera::Status::OK;
  }


  void Socket::close()
  {
    using namespace Aurora::Logging;
  }


  Chimera::Status_t Socket::connect(  const IPAddress address, const Port port )
  {
    mDestAddress = address;
    mDestPort    = port;
    return Chimera::Status::OK;
  }


  Chimera::Status_t Socket::disconnect( )
  {
    mDestAddress = std::numeric_limits<IPAddress>::max();
    mDestPort    = std::numeric_limits<Port>::max();
    return Chimera::Status::OK;
  }


  Chimera::Status_t Socket::write( const void *const data, const size_t bytes )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !data || !bytes )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------------------------------------
    Determine transfer fragmentation sizing/boundaries
    -------------------------------------------------------------------------------*/
    /*-------------------------------------------------
    Figure out the fragment transfer size that can be
    handled by the network layer.
    -------------------------------------------------*/
    size_t maxPayload = mContext->mNetIf->maxTransferSize();
    if ( !maxPayload )
    {
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Figure out the fragment boundaries
    -------------------------------------------------*/
    size_t numFragments     = 0; /**< Total fragments to be sent */
    size_t lastFragmentSize = 0; /**< Track remainder payload in last non-full fragment */

    if ( bytes <= maxPayload )
    {
      /*-------------------------------------------------
      Single "fragment" lte to the reported fragment size
      -------------------------------------------------*/
      numFragments     = 1;
      lastFragmentSize = bytes;
    }
    else
    {
      /*-------------------------------------------------
      Multiple fragments, possibly not aligned
      -------------------------------------------------*/
      numFragments     = bytes / maxPayload;
      lastFragmentSize = bytes % maxPayload;

      if ( lastFragmentSize != 0 )
      {
        numFragments += 1u;
      }
    }

    /*-------------------------------------------------
    Increment the fragment number to account for the
    first fragment always being a packet header.
    -------------------------------------------------*/
    numFragments++;

    /*-------------------------------------------------
    Check that the number of fragments are supported by
    the underlying network interface.
    -------------------------------------------------*/
    if( numFragments > mContext->mNetIf->maxNumFragments() )
    {
      LOG_ERROR( "Packet too large. NetIf only supports %d fragments, but %d are needed.\r\n",
                 mContext->mNetIf->maxNumFragments(), numFragments );
      return Chimera::Status::NOT_SUPPORTED;
    }

    /*-------------------------------------------------------------------------------
    Validate memory requirements
    -------------------------------------------------------------------------------*/
    /*-------------------------------------------------
    Make sure the TX queue can handle another message
    -------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *this );
    if ( mTXQueue.full() )
    {
      return Chimera::Status::FULL;
    }

    /*-------------------------------------------------
    Check if enough memory is available to handle the
    raw data + the fragment control structure overhead.
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Context> lock( *mContext );
    const size_t freeMem = mContext->availableMemory();

    /*-------------------------------------------------
    Calculate the expected memory consumption:

        User payload data
      + Payload for first fragment
      + Total num fragment structures
      --------------------------------------
      = allocation Size
    -------------------------------------------------*/
    const size_t allocationSize = bytes + sizeof( TransportHeader ) + ( sizeof( MsgFrag ) * numFragments );
    if ( freeMem <= allocationSize )
    {
      LOG_DEBUG( "Socket out of memory. Tried to allocate %d bytes from remaining %d\r\n", allocationSize, freeMem );
      return Chimera::Status::MEMORY;
    }

    /*-------------------------------------------------
    Allocate the packet memory and initialize it
    -------------------------------------------------*/
    uint8_t *bytePool = reinterpret_cast<uint8_t *>( mContext->malloc( allocationSize ) );
    if ( !bytePool )
    {
      return Chimera::Status::FAIL;
    }

    /* If we see this "BaD" value in memory, something may have gone wrong... */
    memset( bytePool, 0xBD, allocationSize );

    /*-------------------------------------------------
    Helper value to double check later that memory ops
    didn't over/under run their expected behavior.
    -------------------------------------------------*/
    const std::uintptr_t finalMemAddr = reinterpret_cast<std::uintptr_t>( bytePool ) + allocationSize;

    /*-------------------------------------------------------------------------------
    Construct the header
    -------------------------------------------------------------------------------*/
    TransportHeader header;
    header.crc        = 0;
    header.dataLength = sizeof( TransportHeader ) + bytes;
    header.dstPort    = mDestPort;
    header.srcPort    = mThisPort;
    header.srcAddress = mContext->getIPAddress();
    header._pad       = 0;

    /*-------------------------------------------------------------------------------
    Initialize first message fragment (Header)
    -------------------------------------------------------------------------------*/
    const uint16_t randomUUID     = s_rng() % std::numeric_limits<uint16_t>::max();
    const uint32_t pktDataLength  = sizeof( TransportHeader ) + bytes;

    /*-------------------------------------------------
    Construct fragment from the recent allocation
    -------------------------------------------------*/
    MsgFrag *msg = new ( bytePool ) MsgFrag();
    bytePool += sizeof( MsgFrag );

    /*-------------------------------------------------
    Fill the fragment with packet header data
    -------------------------------------------------*/
    msg->fragmentData   = bytePool;
    msg->fragmentLength = sizeof( TransportHeader );
    msg->totalLength    = pktDataLength;
    msg->totalFragments = numFragments;
    msg->uuid           = randomUUID;

    memcpy( msg->fragmentData, &header, sizeof( TransportHeader ) );
    bytePool += sizeof( TransportHeader );

    /*-------------------------------------------------
    Initialize the linked list
    -------------------------------------------------*/
    MsgFrag *rootMsg = msg;

    /*-------------------------------------------------------------------------------
    Construct the fragment list from user data
    -------------------------------------------------------------------------------*/
    size_t bytesLeft          = bytes;   /**< Bytes to pack into fragments */
    MsgFrag *lastFragPtr      = msg;     /**< Ptr to last valid fragment */
    size_t dataOffset         = 0;       /**< Byte offset into the user data */

    /*-------------------------------------------------
    Fill in the remaining fragments. Start at 1 because
    the packet header has already been created.
    -------------------------------------------------*/
    for ( size_t fragCnt = 1; fragCnt < numFragments; fragCnt++ )
    {
      /*-------------------------------------------------
      Construct the structure holding the message
      -------------------------------------------------*/
      msg = new ( bytePool ) MsgFrag();
      bytePool += sizeof( MsgFrag );
      RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( bytePool ) <= finalMemAddr );

      /*-------------------------------------------------
      Link the fragment list, unless this is the last one
      -------------------------------------------------*/
      msg->next = nullptr;
      if ( lastFragPtr && ( fragCnt != numFragments ) )
      {
        lastFragPtr->next = msg;
      }
      lastFragPtr = msg;

      /*-------------------------------------------------
      Fill out a few control fields
      -------------------------------------------------*/
      msg->fragmentNumber = fragCnt;
      msg->totalLength    = pktDataLength;
      msg->totalFragments = numFragments;
      msg->uuid           = randomUUID;

      /*-------------------------------------------------
      Decide the number of bytes going into this transfer
      -------------------------------------------------*/
      const size_t payloadSize = std::min( bytesLeft, maxPayload );

      /*-------------------------------------------------
      Copy in the user data
      -------------------------------------------------*/
      msg->fragmentData   = bytePool;
      msg->fragmentLength = payloadSize;
      memcpy( msg->fragmentData, ( reinterpret_cast<const uint8_t *const>( data ) + dataOffset ), payloadSize );

      /*-------------------------------------------------
      Update tracking data
      -------------------------------------------------*/
      bytePool += payloadSize;
      bytesLeft -= payloadSize;
      dataOffset += payloadSize;

      RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( bytePool ) <= finalMemAddr );
    }

    /*-------------------------------------------------
    Check exit conditions. Either of these failing
    means a bug is present above.
    -------------------------------------------------*/
    RT_HARD_ASSERT( bytesLeft == 0 );
    RT_HARD_ASSERT( finalMemAddr == reinterpret_cast<std::uintptr_t>( bytePool ) );

    /*-------------------------------------------------------------------------------
    Calculate the CRC over the entire message
    -------------------------------------------------------------------------------*/
    auto crc = Fragment::calcCRC32( rootMsg );
    Fragment::setCRC32( rootMsg, crc );

    /*-------------------------------------------------------------------------------
    Finally, push the fragment list into the queue
    -------------------------------------------------------------------------------*/
    mTXQueue.push( rootMsg );
    mTXReady = true;

    return Chimera::Status::OK;
  }


  Chimera::Status_t Socket::read( void *const data, const size_t bytes )
  {
    Chimera::Thread::LockGuard<Context> lock( *mContext );

    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if( !data || !bytes )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }
    else if( mRXQueue.empty() )
    {
      return Chimera::Status::EMPTY;
    }

    /*-------------------------------------------------
    Read the packet into the user's buffer. Toss the
    first fragment (network header).
    -------------------------------------------------*/
    MsgFrag *packetRoot      = mRXQueue.front();
    MsgFrag *dataPacket      = packetRoot->next;
    Chimera::Status_t status = Chimera::Status::OK;

    memset( data, 0, bytes );
    if ( !Fragment::copyPayloadToBuffer( dataPacket, data, bytes ) )
    {
      status = Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Free the packet and return the status
    -------------------------------------------------*/
    mRXQueue.pop();
    mContext->freePacket( packetRoot );
    return status;
  }


  size_t Socket::available()
  {
    Chimera::Thread::LockGuard<Context> lock( *mContext );

    if( mRXQueue.empty() )
    {
      return 0;
    }
    else
    {
      auto packet = mRXQueue.front();
      return packet->totalLength - sizeof( TransportHeader );
    }
  }

}    // namespace Ripple
