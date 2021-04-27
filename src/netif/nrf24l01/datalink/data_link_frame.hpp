/********************************************************************************
 *  File Name:
 *    data_link_frame.hpp
 *
 *  Description:
 *    Framing utilities for NRF24L01
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NET_INTERFACE_NRF24L01_FRAME_HPP
#define RIPPLE_NET_INTERFACE_NRF24L01_FRAME_HPP

/* Ripple Includes */
#include <Ripple/src/netif/nrf24l01/datalink/data_link_types.hpp>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_constants.hpp>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_types.hpp>

namespace Ripple::NetIf::NRF24::DataLink
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  /*-------------------------------------------------
  Control Field Configuration:
  Be aware adjusting the sizing will likely break the
  packing in the data structures below.
  -------------------------------------------------*/
  /**
   *  Sets the number of bits used to represent the length of the
   *  user data payload inside a single frame. Five bits will store
   *  the max possible size (32 bytes), but realistically that size
   *  will never be achieved due to protocol overhead.
   */
  static constexpr size_t DATA_LENGTH_BITS = 5;

  /**
   *  Sets the number of bits used to represent a fragmented packet's
   *  frame identifier so a full packet can be reconstructed on the
   *  other side.
   *
   *  The max supported packet size is equal to:
   *    sMax = 2^FRAME_NUMBER_BITS * sizeof( PackedFrame::userData )
   */
  static constexpr size_t FRAME_NUMBER_BITS = 5;

  /**
   *  Sets the number of bits used to represent the desired endpoint
   *  for a packet once it reaches the destination.
   */
  static constexpr size_t ENDPOINT_BITS = 3;
  static_assert( Endpoint::EP_NUM_OPTIONS < 7 );

  /**
   *  Current control structure version
   */
  static constexpr size_t VERSION_LENGTH_BITS = 3;
  static constexpr size_t CTRL_STRUCTURE_VERSION = 0;

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   *  Bit packed control field for a datalink frame
   */
  struct _pfCtrl
  {   /* clang-format off */
    uint8_t version     : VERSION_LENGTH_BITS;  /**< Structure versioning information */
    uint8_t dataLength  : DATA_LENGTH_BITS;     /**< User data length */
    uint8_t frameNumber : FRAME_NUMBER_BITS;    /**< Frame number in a fragmented packet */
    uint8_t endpoint    : ENDPOINT_BITS;        /**< Endpoint this frame is destined for on the target */
    bool    multicast   : 1;                    /**< Should this be blasted across the network to everyone? */
    bool    requireACK  : 1;                    /**< Payload should require an ACK */
    bool    pad         : 6;                    /**< Pad for alignment */
  };  /* clang-format on */
  static_assert( sizeof( _pfCtrl ) == sizeof( uint8_t[ 3 ] ) );

  /**
   *  Raw frame type that is transmitted out on the wire. This is sized such
   *  that it can accommodate the maximum payload for the radio.
   */
  struct PackedFrame
  {
    _pfCtrl control;        /**< Frame control field */
    uint8_t userData[ 29 ]; /**< User configurable payload */
  };
  static_assert( sizeof( PackedFrame ) == Physical::MAX_SPI_DATA_LEN );


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Core data structure used to pass data around in the NRF24 network interface
   *  driver. This encapsulates a single transmission unit that the radio supports
   *  and provides several utility functions to facilitate interactions with the
   *  underlying packed data structure.
   */
  class Frame
  {
  public:
    /*-------------------------------------------------
    Public Attributes
    -------------------------------------------------*/
    std::string_view nextHop;               /**< Which node this data is going to (IPAddress) */
    PackedFrame wireData;                   /**< Data frame transmitted on the wire */
    Physical::PipeNumber receivedPipe;      /**< Which pipe the data came from */
    Physical::AutoRetransmitCount rtxCount; /**< Max retransmit attempts */
    Physical::AutoRetransmitDelay rtxDelay; /**< Delay between each retransmission attempt */

    /*-------------------------------------------------
    Constructors/Destructors
    -------------------------------------------------*/
    Frame();
    Frame( const Frame &other );
    Frame( const Frame &&other );
    ~Frame() = default;

    /*-------------------------------------------------
    Operator Overloading
    -------------------------------------------------*/
    void operator=( const Frame &other );

    /*-------------------------------------------------
    Interface Functions
    -------------------------------------------------*/
    /**
     *  Writes data into the user data field and sets the length attribute
     *  if all bytes fit.
     *
     *  @param[in]  data      Data to write into the payload
     *  @param[in]  size      How many bytes to write
     *  @return size_t        Actual number of bytes written
     */
    size_t writeUserData( const void *const data, const size_t size );

    /**
     *  Read out data from the user data field
     *
     *  @param[out] data      Buffer to write into
     *  @param[in]  size      Number of bytes to read
     *  @return size_t        Actual number of bytes read
     */
    size_t readUserData( void *const data, const size_t size );

    /**
     *  Packs a frame into the buffer using network byte ordering
     *
     *  @param[out] buffer    Output buffer to be transmitted over the network
     *  @return void
     */
    void pack( FrameBuffer &buffer );

    /**
     *  Unpacks data received from the network into host byte ordering
     *
     *  @param[in]  buffer    Network data
     *  @return void
     */
    void unpack( const FrameBuffer &buffer );
  };
}    // namespace Ripple::NetIf::NRF24::DataLink

#endif /* !RIPPLE_NET_INTERFACE_NRF24L01_FRAME_HPP */
