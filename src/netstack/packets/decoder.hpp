/********************************************************************************
 *  File Name:
 *    decoder.hpp
 *
 *  Description:
 *    Utility to help decode packets received from the network. Packets are based
 *    on Google's protocol buffers but use an embedded friendly implementation.
 *
 *  Notes:
 *    See documentation for Nanopb at https://jpa.kapsi.fi/nanopb/docs/
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PACKET_DECODER_HPP
#define RIPPLE_PACKET_DECODER_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>
#include <cstring>

/* Nanopb Includes */
#include "pb.h"
#include "pb_common.h"
#include "pb_decode.h"

/* Aurora Includes */
#include <Aurora/logging>
#include <Aurora/memory>

/* Ripple Includes */
#include <Ripple/src/user/user_contract.hpp>
#include <Ripple/src/netstack/packets/packet.hpp>


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t PB_DECODER_BYTE_OVERHEAD = 20;

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  template<typename PacketType, const size_t EncodedSize>
  class PacketDecoder
  {
  public:

    bool decode( const Packet_sPtr pkt, PacketType &data, const pb_msgdesc_t* fields )
    {
      /*-----------------------------------------------------------------
      Unpack the packet into the stream buffer
      -----------------------------------------------------------------*/
      memset( mStreamBuffer, 0, sizeof( mStreamBuffer ) );
      if( !pkt->unpack( mStreamBuffer, sizeof( mStreamBuffer ) ) )
      {
        return false;
      }

      /*-----------------------------------------------------------------
      Decode the data into the packet type
      -----------------------------------------------------------------*/
      pb_istream_t stream = pb_istream_from_buffer( mStreamBuffer, sizeof( mStreamBuffer ) );
      return pb_decode( &stream, fields, &data );
    }

  private:
    Aurora::Memory::IHeapAllocator *const mContext;
    uint8_t mStreamBuffer[ EncodedSize ];
  };

}  // namespace Ripple

#endif  /* !RIPPLE_PACKET_DECODER_HPP */
