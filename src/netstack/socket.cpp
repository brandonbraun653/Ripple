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
#include <cstring>

/* ETL Includes */
#include <etl/crc32.h>

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

    Chimera::Thread::LockGuard lck( *mContext );

    /*-------------------------------------------------
    Create a header for the packet
    -------------------------------------------------*/
    TransportHeader header;
    header.crc        = 0;
    header.dataLength = sizeof( TransportHeader ) + bytes;
    header.dstPort    = mDestPort;
    header.srcPort    = mThisPort;
    header.srcAddress = mContext->getIPAddress();
    header._pad       = 0;

    /*-------------------------------------------------
    Build the full packet in some scratch memory
    -------------------------------------------------*/
    const size_t allocationSize = bytes + sizeof( TransportHeader );
    uint8_t *scratch            = reinterpret_cast<uint8_t *>( mContext->malloc( allocationSize ) );

    memset( scratch, 0, allocationSize );
    memcpy( scratch, &header, sizeof( TransportHeader ) );
    memcpy( scratch + sizeof(TransportHeader ), data, bytes );

    /*-------------------------------------------------
    Add the CRC to the packet header
    -------------------------------------------------*/
    etl::crc32 crc_gen;
    crc_gen.reset();
    crc_gen.add( scratch + offsetof( TransportHeader, dstPort ), scratch + allocationSize );

    auto hdr_ptr = reinterpret_cast<TransportHeader *>( scratch );
    hdr_ptr->crc = crc_gen.value();

    /*-------------------------------------------------
    Push the packet to the queue
    -------------------------------------------------*/
    Packet_sPtr newPacket = allocPacket( mContext );
    auto result = Chimera::Status::FAIL;

    if ( newPacket && newPacket->pack( scratch, allocationSize ) )
    {
      mTXQueue.push( newPacket );
      mTXReady = true;
      result =  Chimera::Status::OK;
    }

    /*-------------------------------------------------
    Free the scratch memory
    -------------------------------------------------*/
    mContext->free( scratch );

    return result;
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

    Packet_sPtr packet       = mRXQueue.front();
    Chimera::Status_t status = Chimera::Status::OK;

    /*-------------------------------------------------
    Build the full packet in some scratch memory
    -------------------------------------------------*/
    const size_t packetSize = packet->size();
    const size_t crcDataOffset = offsetof( TransportHeader, dstPort );
    const size_t dataSize   = packetSize - sizeof( TransportHeader );
    void *scratch = mContext->malloc( packetSize );
    std::memset( scratch, 0, packetSize );

    packet->printPayload();
    packet->unpack( scratch, packetSize );

    /*-------------------------------------------------
    Calculate the CRC over the packet data
    -------------------------------------------------*/
    uint8_t *user_data = ( uint8_t *)scratch;

    etl::crc32 crc_gen;
    crc_gen.reset();
    crc_gen.add( user_data + crcDataOffset, user_data + packetSize );

    auto hdr_ptr = reinterpret_cast<TransportHeader *>( scratch );

    /*-------------------------------------------------
    Read the packet into the user's buffer. Toss the
    first fragment (network header).
    -------------------------------------------------*/
    uint32_t crc = crc_gen.value();
    if ( ( hdr_ptr->crc == crc ) && ( bytes <= dataSize ) )
    {
      memcpy( data, user_data + crcDataOffset, bytes );
    }
    else
    {
      status = Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Free the packet and return the status
    -------------------------------------------------*/
    mRXQueue.pop();
    mContext->free( scratch );

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
