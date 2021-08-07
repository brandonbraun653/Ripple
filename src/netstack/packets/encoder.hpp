/********************************************************************************
 *  File Name:
 *    encoder.hpp
 *
 *  Description:
 *    Utility to help with building packets to send out on the network. Packets
 *    are based on Google's protocol buffers but use an implementation that is
 *    embedded friendly.
 *
 *  Notes:
 *    See documentation for Nanopb at https://jpa.kapsi.fi/nanopb/docs/
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PACKET_ENCODER_HPP
#define RIPPLE_PACKET_ENCODER_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

/* Nanopb Includes */
#include "pb.h"
#include "pb_common.h"
#include "pb_encode.h"

/* Aurora Includes */
#include <Aurora/memory>

/* Ripple Includes */
#include <Ripple/src/netstack/packets/packet.hpp>
#include <Ripple/src/netstack/types.hpp>
#include <Ripple/src/user/user_contract.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  namespace Transport
  {
    /**
     * @brief Builds a transport layer packet, ready to be submitted to a Socket interface
     *
     * @param context         Memory context to allocate from
     * @param header          Transport layer header to attach
     * @param data            Raw data payload
     * @param bytes           Size of raw data payload
     * @return Packet_sPtr    Fully constructed packet
     */
    Packet_sPtr constructPacket( Aurora::Memory::IHeapAllocator *const context, const TransportHeader &header,
                                 const void *const data, const size_t bytes );
  }    // namespace Transport

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   * @brief Encodes NanoPB generated types into a Ripple packet
   *
   * @tparam PacketType         Generated NanoPB type
   * @tparam PacketType_size    Generated NanoPB sizing information about PacketType
   */
  template<typename PacketType, const size_t PacketType_size>
  class PacketEncoder
  {
  public:
    typedef PacketType value_type;
    typedef std::array<uint8_t, PacketType_size> buffer_type;

    /**
     * @brief Encodes ProtoBuf based data into a packet for transmission
     *
     * @param data      Data to be encoded
     * @return Packet_sPtr
     */
    buffer_type encode( const value_type &data, const pb_msgdesc_t *fields )
    {
      /*-----------------------------------------------------------------
      Create the output stream
      -----------------------------------------------------------------*/
      pb_ostream_t stream = pb_ostream_from_buffer( mStreamBuffer.data(), sizeof( mStreamBuffer ) );

      /*-----------------------------------------------------------------
      Encode the data
      -----------------------------------------------------------------*/
      if ( !pb_encode( &stream, fields, &data ) )
      {
        mStreamBuffer.fill( 0 );
      }

      return mStreamBuffer;
    }

  private:
    buffer_type mStreamBuffer;
  };

}    // namespace Ripple

#endif /* !RIPPLE_PACKET_ENCODER_HPP */
