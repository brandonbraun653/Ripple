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
#ifndef RIPPLE_DATALINK_TYPES_HPP
#define RIPPLE_DATALINK_TYPES_HPP

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
#include <Ripple/src/physical/phy_device_constants.hpp>
#include <Ripple/src/physical/phy_device_types.hpp>

namespace Ripple::DATALINK
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
  /**
   *  Supported callback identifiers that can be used to register a
   *  function for invocation on an event. Intended to be used by the
   *  Network layer.
   */
  enum CallbackId : uint8_t
  {
    CB_UNHANDLED,           /**< Default unhandled callback */
    CB_TX_SUCCESS,          /**< A frame completely transmitted (including ACK received) */
    CB_RX_PAYLOAD,          /**< A frame was received */
    CB_ERROR_TX_FAILURE,    /**< A frame's max transmit retry limit was reached */
    CB_ERROR_RX_QUEUE_FULL, /**< Notification that the RX queue should be processed */
    CB_ERROR_RX_QUEUE_LOST, /**< A frame failed to be placed into the RX queue */
    CB_ERROR_TX_QUEUE_FULL, /**< A frame failed to be placed into the TX queue */
    CB_ERROR_ARP_RESOLVE,   /**< ARP could not resolve the destination address */
    CB_ERROR_ARP_LIMIT,     /**< ARP cache has reached the max storage entries */

    CB_NUM_OPTIONS
  };

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
  static_assert( EP_NUM_OPTIONS == ( PHY::MAX_NUM_RX_PIPES - 1 ) );

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   *  Handle to the DataLink layer configuration and runtime state information.
   *  This gets passed around the stack for interaction with this layer.
   */
  struct Handle
  {
    /**
     * Time to wait for a hardware IRQ event (ms) to instruct the datalink
     * layer it has new events to process. This also has the effect of setting
     * the minimum polling rate for processing the transmit and receive queues.
     */
    size_t hwIRQEventTimeout;

    /**
     *  Tracks the number of RX queue overflow events that have occurred since
     *  powerup. Useful to see if the datalink layer has enough bandwidth to
     *  process the queue or if the network layer isn't emptying it fast enough.
     */
    size_t rxQueueOverflows;

    /**
     *  Tracks the number of TX queue overflow events that have occurred since
     *  powerup. Useful to see if the datalink layer is being overloaded or too
     *  much data is being pumped in by the network layer.
     */
    size_t txQueueOverflows;

    void clear()
    {
      hwIRQEventTimeout = 25;
      rxQueueOverflows  = 0;
      txQueueOverflows  = 0;
    }
  };

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
    PHY::AutoRetransmitCount rtxCount; /**< Max retransmit attempts */
    PHY::AutoRetransmitDelay rtxDelay; /**< Delay between each retransmission attempt */

    /*-------------------------------------------------
    RX Specific Data
    -------------------------------------------------*/
    PHY::PipeNumber rxPipe; /**< Which pipe the data came from */

    /*-------------------------------------------------
    Common Data
    -------------------------------------------------*/
    uint16_t frameNumber;                        /**< ID of the frame in the network layer */
    uint16_t length;                             /**< Number of bytes being sent */
    uint16_t control;                            /**< Control flags for the transfer */
    uint8_t payload[ PHY::MAX_TX_PAYLOAD_SIZE ]; /**< Buffer for packet payload */

    void clear()
    {
      nextHop     = 0;
      frameNumber = 0;
      length      = 0;
      control     = 0;
      rtxCount    = PHY::AutoRetransmitCount::ART_COUNT_INVALID;
      rtxDelay    = PHY::AutoRetransmitDelay::ART_DELAY_UNKNOWN;
      rxPipe      = PHY::PipeNumber::PIPE_INVALID;

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
  using RawBuffer = std::array<uint8_t, PHY::MAX_TX_PAYLOAD_SIZE>;

  /**
   *  Queue that is optimized to hold up to 255 elements at a time. Note
   *  that this is not thread safe and will require protection.
   */
  template<const size_t SIZE>
  using FrameQueue = etl::queue<Frame, SIZE, etl::memory_model::MEMORY_MODEL_SMALL>;
}    // namespace Ripple::DATALINK

#endif /* !RIPPLE_DATALINK_TYPES_HPP */
