/********************************************************************************
 *  File Name:
 *    device_types.hpp
 *
 *  Description:
 *    Types for the network interface drivers
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NETWORK_INTERFACE_TYPES_HPP
#define RIPPLE_NETWORK_INTERFACE_TYPES_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

namespace Ripple::NetIf
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
    CB_UNHANDLED,           /**< Default unhandled callback */
    CB_TX_SUCCESS,          /**< A frame completely transmitted (including ACK received) */
    CB_RX_SUCCESS,          /**< A frame was received */
    CB_ERROR_TX_FAILURE,    /**< A frame's max transmit retry limit was reached */
    CB_ERROR_RX_QUEUE_FULL, /**< Notification that the RX queue should be processed */
    CB_ERROR_TX_QUEUE_FULL, /**< A frame failed to be placed into the TX queue */
    CB_ERROR_ARP_RESOLVE,   /**< ARP could not resolve the destination address */
    CB_ERROR_ARP_LIMIT,     /**< ARP cache has reached the max storage entries */

    CB_NUM_OPTIONS
  };

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct CallbackReport
  {
    CallbackId id;
    // Could use union for data storage?
  };


  struct PerfStats
  {
    uint32_t tx_bytes;      /**< Raw number of bytes transmitted */
    uint32_t tx_bytes_lost; /**< Total bytes lost */
    uint32_t rx_bytes;      /**< Raw number of bytes received */
    uint32_t rx_bytes_lost; /**< Raw number of bytes lost */
    uint32_t frame_tx;      /**< Frames transmitted */
    uint32_t frame_rx;      /**< Frames received */
    uint32_t frame_tx_fail; /**< Frames failed to be transmitted */
    uint32_t frame_tx_drop; /**< Total frames dropped due to failure */
    uint32_t frame_rx_drop; /**< Total frames lost */
    uint32_t link_speed_rx; /**< Received bytes per second */
    uint32_t link_speed_tx; /**< Transmitted bytes per second */
    uint32_t link_up_time;  /**< Time the link has been up (ms) */
  };

}  // namespace Ripple

#endif  /* !RIPPLE_NETWORK_INTERFACE_TYPES_HPP */
