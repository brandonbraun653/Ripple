/********************************************************************************
 *  File Name:
 *    message_types.hpp
 *
 *  Description:
 *    Message types for the virtual driver. These represent messages that can be
 *    sent back and forth on the physical layer using the Shockburst protocol
 *    described in the NRF24L01 datasheet.
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NETIF_NRF24L01_VIRTUAL_DRIVER_MESSAGES_HPP
#define RIPPLE_NETIF_NRF24L01_VIRTUAL_DRIVER_MESSAGES_HPP

/* STL Includes */
#include <cstdint>

namespace Ripple::NetIf::NRF24::Physical
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using ShockBurstMsg_t = uint32_t;

  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr ShockBurstMsg_t HW_ACK_MESSAGE  = 0xAABBCCDD;
  static constexpr ShockBurstMsg_t HW_NACK_MESSAGE = ~HW_ACK_MESSAGE;

}    // namespace Ripple::NetIf::NRF24::Physical

#endif /* !RIPPLE_NETIF_NRF24L01_VIRTUAL_DRIVER_MESSAGES_HPP */
