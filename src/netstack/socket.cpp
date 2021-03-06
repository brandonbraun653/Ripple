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
      getRootSink()->flog( Level::LVL_DEBUG, "Failed to close %s\r\n", mDestAddr.data() );
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

    /*-------------------------------------------------
    Make sure the TX queue can handle another message
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Socket>( *this );
    if ( mTXQueue.full() )
    {
      return Chimera::Status::FULL;
    }

    /*-------------------------------------------------
    Check if enough memory is available to handle the
    raw data + the fragment control structure overhead.
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Context> lock( *mContext );

    size_t available      = mContext->availableMemory();
    size_t allocationSize = bytes + ( sizeof( MsgFrag ) * numFragments );

    if ( available <= allocationSize )
    {
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
    const auto startAddr = reinterpret_cast<std::uintptr_t>( pool );
    const auto endAddr   = reinterpret_cast<std::uintptr_t>( pool + allocationSize );

    /*-------------------------------------------------
    Construct the fragment list
    -------------------------------------------------*/
    int bytesLeft     = bytes;
    MsgFrag *lastFrag = nullptr;
    size_t dataOffset = 0;

    for ( size_t fIdx = 0; fIdx < numFragments; fIdx++ )
    {
      /*-------------------------------------------------
      Construct the structure holding the message
      -------------------------------------------------*/
      MsgFrag *msg = new ( pool ) MsgFrag();
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

      /*-------------------------------------------------
      Assign the first fragment to the queue
      -------------------------------------------------*/
      if ( fIdx == 0 )
      {
        mTXQueue.push( msg );
      }
    }

    /*-------------------------------------------------
    Check exit conditions
    -------------------------------------------------*/
    RT_HARD_ASSERT( bytesLeft == 0 );
    RT_HARD_ASSERT( endAddr == reinterpret_cast<std::uintptr_t>( pool ) );

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
