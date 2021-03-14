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
  Enumerations
  -------------------------------------------------------------------------------*/
  enum FrameType : uint32_t
  {
    INVALID,
    ACK_FRAME,
    NACK_FRAME,
    USER_DATA,
  };

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Waits for an ACK message on the given pipe
   *
   *  @param[in]  handle        Control block for the virtual driver
   *  @param[in]  pipe          Which pipe to wait on
   *  @param[in]  timeout       How long to wait in ms
   *  @return Chimera::Status_t
   */
  Chimera::Status_t waitForACK( Handle &handle, const PipeNumber pipe, const size_t timeout );

  /**
   *  Sends an ACK message
   *
   *  @param[in]  handle        Control block for the virtual driver
   *  @param[in]  pipe          Which pipe to send the ACK on.
   *  @return Chimera::Status_t
   */
  Chimera::Status_t sendACK( Handle &handle, const PipeNumber pipe );

  /**
   *  Thread that acts as a message pump to collect RX data from the ZMQ pipes
   *  into a queue for processing by the data link layer.
   *
   *  @param[in]  cfg           Network configuration for the simulator
   *  @return void
   */
  void fifoMessagePump( ZMQConfig *const cfg );

  /**
   *  Factory to build a frame type
   *
   *  @param[out] frame         Frame to be modified
   *  @param[in]  type          What frame to build
   *  @return void
   */
  void frameFactory( DataLink::Frame &frame, FrameType type );

}    // namespace Ripple::NetIf::NRF24::Physical

#endif /* !RIPPLE_NETIF_NRF24L01_SHOCKBURST_HPP */
