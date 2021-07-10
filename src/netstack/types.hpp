/********************************************************************************
 *  File Name:
 *    user_intf.hpp
 *
 *  Description:
 *    Declarations for the network stack user interface
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NET_STACK_CONTEXT_HPP
#define RIPPLE_NET_STACK_CONTEXT_HPP

/* STL Includes */
#include <cstddef>
#include <cstdint>

/* ETL Includes */
#include <etl/list.h>
#include <etl/string.h>
#include <etl/queue.h>

/* Aurora Includes */
#include <Aurora/memory>

/* Chimera Includes */
#include <Chimera/callback>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Forward Declarations
  -------------------------------------------------------------------------------*/
  class Context;
  class Socket;

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Context_rPtr = Context *;
  using Socket_rPtr  = Socket *;
  using SocketId     = uint16_t;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum CallbackId : uint8_t
  {
    CB_OUT_OF_MEMORY,

    CB_NUM_OPTIONS,
    CB_INVALID
  };


  enum class SocketType : uint8_t
  {
    PUSH,
    PULL,

    INVALID
  };

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   * @brief Header field for a transport layer packet
   * @note Byte aligned to reduce size on the network
   *
   * This header is concerned with data integrity and identification of which
   * socket to send payloads to after they've reached their destination device.
   */
  struct TransportHeader
  {
    uint32_t crc;           /**< CRC of the entire packet, including this header */
    SocketId dstPort;       /**< Unique ID for the destination socket on this node */
    SocketId srcPort;       /**< Unique ID for the source socket on the transmitting node */
    IPAddress srcAddress;   /**< Source address this packet came from */
    uint16_t dataLength;    /**< Length of the data payload for this packet */
    uint16_t _pad;          /**< Padding for alignment */
  };
}    // namespace Ripple

#endif /* !RIPPLE_NET_STACK_CONTEXT_HPP */
