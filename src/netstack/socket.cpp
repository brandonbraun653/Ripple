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
    if ( mLock )
    {
      mLock->~RecursiveMutex();
      mContext->free( mLock );
    }
  }


  Chimera::Status_t Socket::open( const Port port )
  {
    /*-------------------------------------------------
    Initialize the socket
    -------------------------------------------------*/
    mThisPort = port;

    return Chimera::Status::OK;
  }


  void Socket::close()
  {
    using namespace Aurora::Logging;
  }


  Chimera::Status_t Socket::connect( const IPAddress address, const Port port )
  {
    mDestAddress = address;
    mDestPort    = port;
    return Chimera::Status::OK;
  }


  Chimera::Status_t Socket::disconnect()
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

    /*-------------------------------------------------
    Push a new packet to the queue
    -------------------------------------------------*/
    Packet_sPtr newPacket = allocPacket( mContext );

    if ( newPacket && newPacket->pack( data, bytes ) )
    {
      mTXQueue.push( newPacket );
      mTXReady = true;
      return Chimera::Status::OK;
    }
    else
    {
      return Chimera::Status::FAIL;
    }
  }


  Chimera::Status_t Socket::read( void *const data, const size_t bytes )
  {
    Chimera::Thread::LockGuard<Context> lock( *mContext );

    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !data || !bytes )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }
    else if ( mRXQueue.empty() )
    {
      return Chimera::Status::EMPTY;
    }

    /*-------------------------------------------------
    Read the packet into the user's buffer. Toss the
    first fragment (network header).
    -------------------------------------------------*/
    Packet_sPtr packet       = mRXQueue.front();
    Chimera::Status_t status = Chimera::Status::OK;

    if ( !packet->unpack( data, bytes ) )
    {
      status = Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Free the packet and return the status
    -------------------------------------------------*/
    mRXQueue.pop();
    return status;
  }


  size_t Socket::available()
  {
    Chimera::Thread::LockGuard<Context> lock( *mContext );

    if ( mRXQueue.empty() )
    {
      return 0;
    }
    else
    {
      auto packet = mRXQueue.front();
      return packet->size() - sizeof( TransportHeader );
    }
  }

}    // namespace Ripple
