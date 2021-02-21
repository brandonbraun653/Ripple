/********************************************************************************
 *  File Name:
 *    data_link_types.hpp
 *
 *  Description:
 *    Data types and definitions associated with the data link layer
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_DataLink_TYPES_HPP
#define RIPPLE_DataLink_TYPES_HPP

/* STL Includes */
#include <array>
#include <cstddef>

/* Chimera Includes */
#include <Chimera/thread>

/* ETL Includes */
#include <etl/callback_service.h>
#include <etl/delegate.h>
#include <etl/queue.h>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_constants.hpp>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_types.hpp>

namespace Ripple::NetIf::NRF24::DataLink
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr uint16_t SubNetMask = 0x7;

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using IPSubNetLevel = uint16_t;
  using IPHostId      = uint16_t;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/

  enum bfControlFlags : uint16_t
  {
    CTRL_PAYLOAD_ACK = ( 1u << 0 ), /**< Payload requires ack */
    CTRL_STATIC      = ( 1u << 1 ), /**< Payload should be configured for static length */
  };

  /**
   *  Describes logical endpoints for data flowing through the network. These
   *  directly correspond to RX pipes on the radio device.
   */
  enum Endpoint : uint8_t
  {
    EP_DEVICE_ROOT,        /**< Root pipe that handles command and control data */
    EP_NETWORK_SERVICES,   /**< Network housekeeping and internal messages */
    EP_DATA_FORWARDING,    /**< Data that needs to be forwarded to another device */
    EP_APPLICATION_DATA_0, /**< Data destined for the user application to consume */
    EP_APPLICATION_DATA_1, /**< Second pipe for user data to increase throughput */

    EP_NUM_OPTIONS,
    EP_UNKNOWN
  };
  // One RX pipe is dedicated for the TX auto-ack process
  static_assert( EP_NUM_OPTIONS == ( Physical::MAX_NUM_RX_PIPES - 1 ) );

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   *  Core data type of the DataLink layer. Defines a packet's transmission
   *  properties, payload, and other information necessary to implement the
   *  functionality of this layer.
   */
  struct Frame
  {
    /*-------------------------------------------------
    TX Specific Data
    -------------------------------------------------*/
    uint32_t nextHop;                  /**< Which node this data is going to (IPAddress) */
    Physical::AutoRetransmitCount rtxCount; /**< Max retransmit attempts */
    Physical::AutoRetransmitDelay rtxDelay; /**< Delay between each retransmission attempt */

    /*-------------------------------------------------
    RX Specific Data
    -------------------------------------------------*/
    Physical::PipeNumber rxPipe; /**< Which pipe the data came from */

    /*-------------------------------------------------
    Common Data
    -------------------------------------------------*/
    uint16_t frameNumber;                        /**< ID of the frame in the network layer */
    uint16_t length;                             /**< Number of bytes being sent */
    uint16_t control;                            /**< Control flags for the transfer */
    uint8_t payload[ Physical::MAX_TX_PAYLOAD_SIZE ]; /**< Buffer for packet payload */

    void clear()
    {
      nextHop     = 0;
      frameNumber = 0;
      length      = 0;
      control     = 0;
      rtxCount    = Physical::AutoRetransmitCount::ART_COUNT_INVALID;
      rtxDelay    = Physical::AutoRetransmitDelay::ART_DELAY_UNKNOWN;
      rxPipe      = Physical::PipeNumber::PIPE_INVALID;

      memset( payload, 0, ARRAY_BYTES( payload ) );
    }
  };


  struct TransferControlBlock
  {
    bool inProgress; /**< TX is ongoing and hasn't been ACK'd yet */
    size_t timeout;  /**< Timeout for the transfer */
    size_t start;    /**< Start time for the transfer */
  };

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  /**
   *  Buffer type that can hold the raw data coming in and out of PHY layer
   */
  using RawBuffer = std::array<uint8_t, Physical::MAX_TX_PAYLOAD_SIZE>;

  /**
   *  Queue that is optimized to hold up to 255 elements at a time. Note
   *  that this is not thread safe and will require protection.
   */
  template<const size_t SIZE>
  using FrameQueue = etl::queue<Frame, SIZE, etl::memory_model::MEMORY_MODEL_SMALL>;
}    // namespace Ripple::DataLink

#endif /* !RIPPLE_DataLink_TYPES_HPP */
