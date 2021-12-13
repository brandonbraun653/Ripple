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
  Public Functions
  -------------------------------------------------------------------------------*/
  bool packetInFilter( const PacketId pkt, const PacketFilter &filter )
  {
    for ( auto &filter_pkt : filter )
    {
      if ( pkt == filter_pkt )
      {
        return true;
      }
    }

    return false;
  }


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


  Chimera::Status_t Socket::open( const SocketConfig &cfg )
  {
    mThisPort = cfg.devicePort;
    mConfig   = cfg;

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
    Push the packet to the queue
    -------------------------------------------------*/
    Packet_sPtr newPacket = Transport::constructPacket( &mContext->mHeap, header, data, bytes );
    auto result           = Chimera::Status::FAIL;

    if ( newPacket )
    {
      mTXQueue.push( newPacket );
      mTXReady = true;
      result   = Chimera::Status::OK;
    }


    return result;
  }


  Chimera::Status_t Socket::read( void *const data, const size_t bytes )
  {
    Chimera::Thread::LockGuard sockLock( *this );
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
    const size_t packetSize    = packet->size();
    const size_t crcDataOffset = offsetof( TransportHeader, dstPort );
    const size_t dataSize      = packetSize - sizeof( TransportHeader );
    uint8_t *scratch           = reinterpret_cast<uint8_t *>( mContext->malloc( packetSize ) );
    memset( scratch, 0, packetSize );
    packet->unpack( scratch, packetSize );

    /*-------------------------------------------------
    Calculate the CRC over the packet data
    -------------------------------------------------*/
    etl::crc32 crc_gen;
    crc_gen.reset();
    crc_gen.add( scratch + crcDataOffset, scratch + packetSize );

    auto hdr_ptr = reinterpret_cast<TransportHeader *>( scratch );

    /*-------------------------------------------------
    Read the packet into the user's buffer. Toss the
    first fragment (network header).
    -------------------------------------------------*/
    uint32_t crc = crc_gen.value();
    if ( ( hdr_ptr->crc == crc ) && ( bytes <= dataSize ) )
    {
      memcpy( data, scratch + sizeof( TransportHeader ), bytes );
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
    Chimera::Thread::LockGuard sockLock( *this );
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


  void Socket::processData()
  {
    /*-----------------------------------------------------------------
    Ensure thread safety on memory accesses
    -----------------------------------------------------------------*/
    Chimera::Thread::LockGuard sockLock( *this );
    Chimera::Thread::LockGuard<Context> lock( *mContext );

    /*-----------------------------------------------------------------
    Process all pending packets
    -----------------------------------------------------------------*/
    auto bytesAvailable = this->available();
    while ( bytesAvailable )
    {
      /*-----------------------------------------------------------------
      Allocate temporary storage for the packet
      -----------------------------------------------------------------*/
      uint8_t *rxData = reinterpret_cast<uint8_t *>( mContext->malloc( bytesAvailable ) );
      if ( !rxData )
      {
        LOG_ERROR( "Out of storage for socket\r\n" );
        return;
      }

      /*-----------------------------------------------------------------
      Read out the data packet
      -----------------------------------------------------------------*/
      auto result = this->read( rxData, bytesAvailable );

      /*-----------------------------------------------------------------
      Read the header and invoke the appropriate handler
      -----------------------------------------------------------------*/
      if ( result == Chimera::Status::OK )
      {
        /*-----------------------------------------------------------------
        Is the packet allowed to be received by the socket?
        -----------------------------------------------------------------*/
        PacketHdr *hdr = reinterpret_cast<PacketHdr *>( rxData );
        if ( packetInFilter( hdr->id, mConfig.rxFilter ) )
        {
          /*-----------------------------------------------------------------
          Calculate the offset in to the buffer where the raw data lives
          -----------------------------------------------------------------*/
          uint8_t *pktData = rxData + sizeof( PacketHdr );

          /*-----------------------------------------------------------------
          Call the specific handler if available, else call default handler
          -----------------------------------------------------------------*/
          auto callbackIterator = mPktCallbacks.find( hdr->id );
          if ( callbackIterator == mPktCallbacks.end() )
          {
            RT_HARD_ASSERT( mCommonPktCallback );
            mCommonPktCallback( hdr->id, pktData, hdr->size );
          }
          else
          {
            callbackIterator->second( hdr->id, pktData, hdr->size );
          }
        }
        else
        {
          LOG_DEBUG( "Packet id [%d] rejected by socket\r\n", hdr->id );
        }
      }
      else
      {
        LOG_ERROR( "Packet read failure: %d\r\n", result );
      }

      /*-----------------------------------------------------------------
      Free the temporary storage and grab the next packet size
      -----------------------------------------------------------------*/
      mContext->free( rxData );
      bytesAvailable = this->available();
    }
  }

}    // namespace Ripple
