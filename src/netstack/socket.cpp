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
#include <cstdint>
#include <cstddef>

/* ETL Includes */
#include <etl/crc32.h>
#include <etl/random.h>

/* Aurora Includes */
#include <Aurora/logging>

/* Chimera Assert */
#include <Chimera/assert>

/* Ripple Includes */
#include <Ripple/shared>
#include <Ripple/src/netstack/types.hpp>
#include <Ripple/src/netstack/socket.hpp>
#include <Ripple/src/netstack/context.hpp>

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


  Chimera::Status_t Socket::open( const std::string_view &port )
  {
    /*-------------------------------------------------
    Initialize the socket
    -------------------------------------------------*/
    s_rng.initialise( Chimera::millis() );
    mThisAddr = port;

    return Chimera::Status::OK;
  }


  void Socket::close()
  {
    using namespace Aurora::Logging;

    if( disconnect( mDestAddr ) == Chimera::Status::OK )
    {
      mDestAddr = "";
    }
    else
    {
      LOG_DEBUG( "Failed to close %s\r\n", mDestAddr.data() );
    }
  }


  Chimera::Status_t Socket::connect( const std::string_view &port )
  {
    mDestAddr = port;
    return Chimera::Status::OK;
  }


  Chimera::Status_t Socket::disconnect( const std::string_view &port )
  {
    return Chimera::Status::FAIL;
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

    auto userData = reinterpret_cast<const uint8_t *const>( data );

    /*-------------------------------------------------
    Figure out the fragment transfer size that can be
    handled by the network layer.
    -------------------------------------------------*/
    size_t maxPayload = mContext->mNetIf->maxTransferSize();
    if ( !maxPayload )
    {
      // Protect against a div/0 down below
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Figure out the fragment boundaries
    -------------------------------------------------*/
    size_t numFragments     = 0;
    size_t lastFragmentSize = 0;

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

    // Increment fragment count to allow for header
    numFragments++;

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

    const size_t available      = mContext->availableMemory();
    const size_t userDataSize   = bytes + ( sizeof( MsgFrag ) * numFragments );
    const size_t headerSize     = sizeof( MsgFrag ) + sizeof( TransportHeader );
    const size_t allocationSize = headerSize + userDataSize;

    if ( available <= allocationSize )
    {
      LOG_DEBUG( "Socket out of memory\n" );
      return Chimera::Status::MEMORY;
    }

    /*-------------------------------------------------
    Allocate the memory and assign the fragments
    -------------------------------------------------*/
    uint8_t *pool = reinterpret_cast<uint8_t *>( mContext->malloc( allocationSize ) );
    if ( !pool )
    {
      // Shouldn't fail, but it doesn't hurt to check
      return Chimera::Status::FAIL;
    }

    // Runtime boundaries
    const auto endAddr   = reinterpret_cast<std::uintptr_t>( pool + allocationSize );

    /*-------------------------------------------------
    Construct the header
    -------------------------------------------------*/
    TransportHeader header;
    header.crc           = 0;
    header.dataLength    = sizeof( TransportHeader ) + bytes;
    header.dstSocketUUID = 0;    // TODO: Will need to look this up
    header.srcSocketUUID = mUUID;
    header._pad          = 0;

    /* Allocate the message memory */
    MsgFrag *msg = new ( pool ) MsgFrag();
    pool += sizeof( MsgFrag );

    /* Allocate the payload (header) memory */
    msg->fragmentData   = pool;
    msg->fragmentLength = sizeof( TransportHeader );

    /* Copy in the payload data */
    memcpy( msg->fragmentData, &header, sizeof( TransportHeader ) );
    pool += sizeof( TransportHeader );

    MsgFrag *rootMsg = msg;

    /*-------------------------------------------------
    Construct the fragment list
    -------------------------------------------------*/
    size_t bytesLeft  = bytes;
    MsgFrag *lastFrag = msg;
    size_t dataOffset = 0;

    const uint32_t random_uuid = s_rng();

    for ( size_t fIdx = 1; fIdx < numFragments; fIdx++ )
    {
      /*-------------------------------------------------
      Construct the structure holding the message
      -------------------------------------------------*/
      msg = new ( pool ) MsgFrag();
      pool += sizeof( MsgFrag );
      RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( pool ) <= endAddr );

      /*-------------------------------------------------
      Link the fragment list, unless this is the last one
      -------------------------------------------------*/
      msg->nextFragment = nullptr;
      if ( lastFrag && ( fIdx != numFragments ) )
      {
        lastFrag->nextFragment = msg;
      }
      lastFrag = msg;

      /*-------------------------------------------------
      Fill out a few control fields
      -------------------------------------------------*/
      msg->flags          = 0;
      msg->type           = 0;
      msg->fragmentNumber = fIdx;
      msg->totalLength    = bytes;
      msg->uuid           = random_uuid;

      /*-------------------------------------------------
      Decide the number of bytes going into this transfer
      -------------------------------------------------*/
      size_t payloadSize = maxPayload;
      if ( bytesLeft < maxPayload )
      {
        payloadSize = bytesLeft;
      }

      /*-------------------------------------------------
      Copy in the user data and update the byte tracking
      -------------------------------------------------*/
      msg->fragmentData   = pool;
      msg->fragmentLength = payloadSize;
      memcpy( msg->fragmentData, ( userData + dataOffset ), payloadSize );

      pool += payloadSize;
      bytesLeft -= payloadSize;
      dataOffset += payloadSize;
      RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( pool ) <= endAddr );
    }

    /*-------------------------------------------------
    Check exit conditions
    -------------------------------------------------*/
    RT_HARD_ASSERT( bytesLeft == 0 );
    RT_HARD_ASSERT( endAddr == reinterpret_cast<std::uintptr_t>( pool ) );

    /*-------------------------------------------------
    Calculate the CRC over the entire message
    -------------------------------------------------*/
    etl::crc32 crc_gen;
    crc_gen.reset();

    /* CRC of the header itself, minus the CRC field */
    crc_gen.add( reinterpret_cast<uint8_t *>( &header + sizeof( TransportHeader::crc ) ),
                 reinterpret_cast<uint8_t *>( &header + sizeof( header ) ) );

    /* CRC of the data contained in each fragment */
    MsgFrag *fragIter = rootMsg;
    while( fragIter->nextFragment )
    {
      crc_gen.add( reinterpret_cast<uint8_t *>( fragIter->fragmentData ),
                   reinterpret_cast<uint8_t *>( fragIter->fragmentData ) + fragIter->fragmentLength );
      fragIter = fragIter->nextFragment;
    }

    auto hdr = reinterpret_cast<TransportHeader*>( rootMsg->fragmentData );
    hdr->crc = crc_gen.value();

    /*-------------------------------------------------
    Finally, push the fragment list into the queue
    -------------------------------------------------*/
    mTXQueue.push( rootMsg );
    mTXReady = true;

    return Chimera::Status::OK;
  }


  Chimera::Status_t Socket::read( void *const data, const size_t bytes )
  {
    return Chimera::Status::FAIL;
  }


  size_t Socket::available()
  {
    return 0;
  }

}    // namespace Ripple
