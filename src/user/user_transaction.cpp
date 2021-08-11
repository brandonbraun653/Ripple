/********************************************************************************
 *  File Name:
 *    user_transaction.cpp
 *
 *  Description:
 *    Implementation details for the transactional aspects of the Ripple network.
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Aurora Includes */
#include <Aurora/logging>

/* Nanopb Includes */
#include "pb.h"
#include "pb_common.h"
#include "pb_encode.h"

/* Project Includes */
#include <Ripple/packets>
#include <Ripple/user>


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  bool transmit( const PacketId pkt, Socket &socket, const void *const data, const size_t size )
  {
    /*-----------------------------------------------------------------
    Look up packet to see if supported by the socket
    -----------------------------------------------------------------*/
    if ( !packetInFilter( pkt, socket.mConfig.txFilter ) )
    {
      LOG_ERROR( "TX of packet %d not supported by socket\r\n", pkt );
      return false;
    }

    auto iter = PacketDefinitions.find( pkt );
    if ( iter == PacketDefinitions.end() )
    {
      LOG_ERROR( "Packet ID [%d] not found in project definitions table\r\n", pkt );
      return false;
    }

    PacketDef pktDef = iter->second;

    /*-----------------------------------------------------------------
    Calculate sizing paramters and allocate memory for the encoded data
    plus the packet header.
    -----------------------------------------------------------------*/
    size_t allocSize      = sizeof( PacketHdr ) + pktDef.size;
    uint8_t *packetBuffer = reinterpret_cast<uint8_t *>( socket.mContext->malloc( allocSize ) );

    if ( !packetBuffer )
    {
      LOG_ERROR( "Unable to allocate [%d] bytes for packet [%d]\r\n", allocSize, pkt );
      return false;
    }
    else
    {
      memset( packetBuffer, 0, allocSize );
    }

    /*-----------------------------------------------------------------
    Encode the data according to the packet definition
    -----------------------------------------------------------------*/
    uint8_t *encoderView = packetBuffer + sizeof( PacketHdr );
    pb_ostream_t stream  = pb_ostream_from_buffer( encoderView, pktDef.size );

    if ( !pb_encode( &stream, pktDef.fields, data ) )
    {
      socket.mContext->free( packetBuffer );
      return false;
    }

    PacketHdr *hdr = reinterpret_cast<PacketHdr *>( packetBuffer );
    hdr->id        = pkt;
    hdr->size      = pktDef.size;

    /*-----------------------------------------------------------------
    Write the raw bytes
    -----------------------------------------------------------------*/
    auto result = socket.write( packetBuffer, allocSize );
    socket.mContext->free( packetBuffer );

    return ( result == Chimera::Status::OK );
  }


  bool onReceive( const PacketId pkt, Socket &socket, PacketCallback callback )
  {
    /*-----------------------------------------------------------------
    Look up packet to see if supported by the socket
    -----------------------------------------------------------------*/
    if ( !packetInFilter( pkt, socket.mConfig.rxFilter ) )
    {
      LOG_DEBUG( "RX of packet %d not supported\r\n", pkt );
      return false;
    }

    /*-----------------------------------------------------------------
    Is the packet already in existence? Replace the callback.
    -----------------------------------------------------------------*/
    auto iter = socket.mPktCallbacks.find( pkt );
    if ( iter != socket.mPktCallbacks.end() )
    {
      iter->second = callback;
      return true;
    }

    /*-----------------------------------------------------------------
    New packet to add. Try and push the registry in.
    -----------------------------------------------------------------*/
    if ( !socket.mPktCallbacks.full() )
    {
      socket.mPktCallbacks.insert( { pkt, callback } );
      return true;
    }

    /*-----------------------------------------------------------------
    Last resort. If we make it here, there was no registration.
    -----------------------------------------------------------------*/
    return false;
  }


  bool onReceive( Socket &socket, PacketCallback callback )
  {
    socket.mCommonPktCallback = callback;
    return true;
  }

}    // namespace Ripple
