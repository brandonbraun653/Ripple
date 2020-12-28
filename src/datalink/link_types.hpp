/********************************************************************************
 *  File Name:
 *    link_types.hpp
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

namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  /**
   *  Supported callback identifiers that can be used to register a
   *  function for invocation on an event.
   */
  enum CallbackId : uint8_t
  {
    VECT_UNHANDLED,   /**< Default unhandled callback */
    VECT_TX_COMPLETE, /**< A frame completely transmitted (including ACK received) */
    VECT_RX_COMPLETE, /**< A frame was received */
    VECT_INIT_ERROR,  /**< Initialization error */

    VECT_NUM_OPTIONS
  };

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct DLHandle
  {
    Chimera::Threading::ThreadId threadId; /**< Id of the thread running the datalink services */
  };

  /**
   *  Packed data structure that represents a possible frame to
   *  be written into the NRF24 TX FIFO.
   */
  struct Frame
  {
    IPAddress dstNode;     /**< Which node this data is going to */
    IPAddress srcNode;     /**< Original sender of the data */
    uint8_t control;       /**< Internal control flags */
    uint8_t header;        /**< Identifies the payload message type */
    uint8_t payload[ 26 ]; /**< Buffer for packet payload */
  };
  static_assert( sizeof( Frame ) == PHY::MAX_TX_PAYLOAD_SIZE );

  /**
   *  Data that is passed into an datalink layer callback event
   */
  struct CBData
  {
    CallbackId id;
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

  using CBFunction = etl::delegate<void(CBData &)>;
  using CBVectors  = std::array<CBFunction, VECT_NUM_OPTIONS>;

}    // namespace Ripple::DATALINK

#endif /* !RIPPLE_DATALINK_TYPES_HPP */
