/********************************************************************************
 *  File Name:
 *    transport_types.hpp
 *
 *  Description:
 *    Types and definitions associated with the Transport layer
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_TRANSPORT_TYPES_HPP
#define RIPPLE_TRANSPORT_TYPES_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

/* ETL Includes */
#include <etl/list.h>
#include <etl/pool.h>
#include <etl/delegate.h>

/* Ripple Includes */
#include <Ripple/src/network/network_types.hpp>

namespace Ripple::Transport
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  /**
   *  Setting size to zero here allows for a shared memory pool
   */
  using DatagramList = etl::list<Network::Datagram, 0>;

  template<const size_t SIZE>
  using DatagramPool = etl::pool<DatagramList::pool_type, SIZE>;

  using EPCallback = etl::delegate<void( DataLink::Endpoint )>;

  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr uint8_t FRAGMENT_ID_NONE      = 0;
  static constexpr uint8_t FRAGMENT_ID_FIRST     = 1;
  static constexpr uint8_t FRAGMENT_ID_LAST      = 254;
  static constexpr uint8_t FRAGMENT_ID_TERMINATE = 255;

  /**
   *  Set the upper limit on the packet length the network will
   *  attempt to fragment and transmit.
   */
  static constexpr size_t MAX_PACKET_SIZE = 4096;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum FragmentId : uint8_t
  {
    FRAG_ID_FIRST = 0,   /**< First fragrment number for a packet */
    FRAG_ID_MAX   = 253, /**< Maximum non-terminating fragment number for a packet */
    FRAG_ID_NONE,        /**< Datagram contains no fragments */
    FRAG_ID_TERMINATE    /**< Indicates this is the last packet fragment number */
  };

  enum CallbackId : uint8_t
  {
    CB_UNHANDLED,
    CB_SERVICE_OVERRUN,
    CB_EP_RX_AVAILABLE, /**< An endpoint has received data */
    CB_EP_TX_COMPLETE,  /**< An endpoint transmission has completed */
    CB_EP_NO_MEMORY,    /**< An endpoint has run out of memory to store datagrams */

    CB_NUM_OPTIONS
  };


  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct Handle
  {
    uint8_t dummy;
  };

  struct Packet
  {
    uint32_t destIP;
    uint8_t numFragments;

    void *buffer;       /**< IO buffer for user memory */
    size_t bufferSize;  /**< Buffer size in bytes */
  };

}    // namespace Ripple::Transport

#endif /* !RIPPLE_TRANSPORT_TYPES_HPP */
