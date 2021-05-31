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

}  // namespace Ripple

#endif  /* !RIPPLE_NETWORK_INTERFACE_TYPES_HPP */
