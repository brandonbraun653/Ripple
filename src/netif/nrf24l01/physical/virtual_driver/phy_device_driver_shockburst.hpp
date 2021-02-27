/********************************************************************************
 *  File Name:
 *    phy_device_driver_shockburst.hpp
 *
 *  Description:
 *    Shockburst layer for the virtual NRF24
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NETIF_NRF24L01_SHOCKBURST_HPP
#define RIPPLE_NETIF_NRF24L01_SHOCKBURST_HPP

/* STL Includes */
#include <cstddef>

/* Chimera Includes */
#include <Chimera/common>

/* Ripple Includes */
#include <Ripple/src/netif/nrf24l01/physical/phy_device_types.hpp>


namespace Ripple::NetIf::NRF24::Physical::ShockBurst
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Waits for an ACK message on the given pipe
   *
   *  @param[in]  handle        Control block for the virtual driver
   *  @param[in]  timeout       How long to wait in ms
   *  @return Chimera::Status_t
   */
  Chimera::Status_t waitForACK( Handle &handle, const size_t timeout );

  /**
   *  Thread that acts as a message pump to collect RX data from the ZMQ pipes
   *  into a queue for processing by the data link layer.
   *
   *  @param[in]  cfg           Network configuration for the simulator
   *  @return void
   */
  void fifoMessagePump( ZMQConfig *const cfg );

}    // namespace Ripple::NetIf::NRF24::Physical

#endif /* !RIPPLE_NETIF_NRF24L01_SHOCKBURST_HPP */
