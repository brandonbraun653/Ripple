/********************************************************************************
 *  File Name:
 *    network_types.hpp
 *
 *  Description:
 *    Types and definitions associated with the Network layer
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NETWORK_TYPES_HPP
#define RIPPLE_NETWORK_TYPES_HPP

/* STL Includes */
#include <cstdint>

/* Chimera Includes */
#include <Chimera/common>

/* Ripple Includes */
#include <Ripple/src/physical/phy_device_constants.hpp>


namespace Ripple::Network
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using IPAddress = uint32_t;
  using Port = uint8_t;

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   *  Base unit of data that will be transmitted across the air. Must fit within
   *  the maximum limits of the user's TX protocol, up to the hardware radio limit.
   *  There is no checksum due to the NRF24L01 implementing a checksum in hardware.
   *  TX-ing/RX-ing a packet implicitly means a checksum was successful.
   */
  struct Datagram
  {
    IPAddress source;       /**< IP address of the node who originally sent the message */
    IPAddress destination;  /**< IP address of the final destination node */
    uint8_t fragmentId;     /**< Unique identifier for which fragment this is of a larger packet */
    uint8_t control;        /**< Control field indicating packet properties */
    uint8_t data[ 22 ];     /**< Storage for the packet's user data */
  };

  static_assert( sizeof( Datagram ) <= Physical::MAX_TX_PAYLOAD_SIZE );
  static_assert( offsetof( Datagram, source ) == 0 );
  static_assert( offsetof( Datagram, destination ) == 4 );
  static_assert( offsetof( Datagram, fragmentId ) == 8 );
  static_assert( offsetof( Datagram, control ) == 9 );
  static_assert( offsetof( Datagram, data ) == 10 );


  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t DATAGRAM_MAX_PAYLOAD = ARRAY_BYTES( Datagram::data );

  /*-------------------------------------------------
  Datagram::control: Packet requires network ACK
    Some packets need acknowledgement from the higher
    level layers to know that they made it. Setting
    this flag signals the Network layer to send an
    ACK back upon reception.
  -------------------------------------------------*/
  static constexpr uint8_t CTL_ACK_POS = 0;
  static constexpr uint8_t CTL_ACK_MSK = ( 1u << CTL_ACK_POS );

  /*-------------------------------------------------
  Datagram::control: Bits 1 & 2 are reserved
  -------------------------------------------------*/
  // TBD

  /*-------------------------------------------------
  Datagram::control: Packet length bit-field
    5-bits, representing up to 32 bytes of data that
    could be stored in the Datagram::data field. In
    real life it will be less than this.
  -------------------------------------------------*/
  static constexpr uint8_t CTL_LENGTH_POS = 3;
  static constexpr uint8_t CTL_LENGTH_MSK = ( 0x1F << CTL_LENGTH_POS );


}  // namespace Ripple::Network

#endif  /* !RIPPLE_NETWORK_TYPES_HPP */
