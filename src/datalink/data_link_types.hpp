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
  using IPHostId = uint16_t;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  /**
   *  Supported callback identifiers that can be used to register a
   *  function for invocation on an event.
   */
  enum CallbackId : uint8_t
  {
    VECT_UNHANDLED,     /**< Default unhandled callback */
    VECT_TX_COMPLETE,   /**< A frame completely transmitted (including ACK received) */
    VECT_RX_COMPLETE,   /**< A frame was received */
    VECT_INIT_ERROR,    /**< Initialization error */
    VECT_RX_QUEUE_FULL, /**< Notification that the RX queue should be processed */
    VECT_RX_QUEUE_LOST, /**< A frame failed to be placed into the RX queue */
    VECT_TX_QUEUE_FULL, /**< A frame failed to be placed into the TX queue */
    VECT_UNKNOWN_DESTINATION,

    VECT_NUM_OPTIONS
  };

  enum bfControlFlags : uint16_t
  {
    CTRL_NO_ACK = ( 1u << 0 ), /**< Payload requires no ack */
    CTRL_STATIC = ( 1u << 1 ), /**< Payload should be configured for static length */
  };

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct Handle
  {
    /**
     * Time to wait for a hardware IRQ event (ms) to instruct the datalink
     * layer it has new events to process. This also has the effect of setting
     * the minimum polling rate for processing the transmit and receive queues.
     */
    size_t hwIRQEventTimeout;

    /**
     *  If true, configures the system to use dynamic length packets on all pipes.
     *  If false, uses a static packet length.
     */
    bool dynamicPackets;

    /**
     *  If dynamicPackets is false, this is the on-air length of each packet.
     */
    uint8_t staticPacketSize;

    void clear()
    {
      hwIRQEventTimeout = 25;
      dynamicPackets    = true;
      staticPacketSize  = PHY::MAX_TX_PAYLOAD_SIZE;
    }
  };


  struct Frame
  {
    IPAddress nextHop;                           /**< Which node this data is going to */
    uint16_t frameNumber;
    uint16_t length;                             /**< Number of bytes being sent */
    uint16_t control;                            /**< Control flags for the transfer */
    PHY::AutoRetransmitCount rtxCount;           /**< Max retransmit attempts */
    PHY::AutoRetransmitDelay rtxDelay;           /**< Delay between each retransmission attempt */
    uint8_t payload[ PHY::MAX_TX_PAYLOAD_SIZE ]; /**< Buffer for packet payload */
  };

  /**
   *  Data that is passed into an datalink layer callback event
   */
  struct CBData
  {
    CallbackId id;
    uint16_t frameNumber;
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

  using CBFunction = etl::delegate<void( CBData & )>;
  using CBVectors  = std::array<CBFunction, VECT_NUM_OPTIONS>;

}    // namespace Ripple::DATALINK

#endif /* !RIPPLE_DATALINK_TYPES_HPP */
