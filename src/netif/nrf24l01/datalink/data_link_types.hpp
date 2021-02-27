/********************************************************************************
 *  File Name:
 *    data_link_types.hpp
 *
 *  Description:
 *    Data types and definitions associated with the data link layer
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_DataLink_TYPES_HPP
#define RIPPLE_DataLink_TYPES_HPP

/* STL Includes */
#include <array>
#include <cstddef>

/* Chimera Includes */
#include <Chimera/thread>

/* ETL Includes */
#include <etl/queue.h>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_constants.hpp>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_types.hpp>


namespace Ripple::NetIf::NRF24::DataLink
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using IPSubNetLevel = uint16_t;
  using IPHostId      = uint16_t;

  /*-------------------------------------------------------------------------------
  Forward Declarations
  -------------------------------------------------------------------------------*/
  class Frame;

  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr uint8_t EndpointAddrModifiers[ 5 ] = {
    0xCA, /**< DEVICE CONTROL */
    0xC5, /**< NETWORK_SERVICES */
    0x54, /**< DATA FORWARDING */
    0xB3, /**< APPLICATION DATA 0 */
    0xD3  /**< APPLICATION DATA 1 */
  };

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  /**
   *  Describes logical endpoints for data flowing through the network. These
   *  directly correspond to RX pipes on the radio device.
   */
  enum Endpoint : uint8_t
  {
    EP_DEVICE_CTRL,        /**< Root pipe that handles command and control data */
    EP_NETWORK_SERVICES,   /**< Network housekeeping and internal messages */
    EP_DATA_FORWARDING,    /**< Data that needs to be forwarded to another device */
    EP_APPLICATION_DATA_0, /**< Data destined for the user application to consume */
    EP_APPLICATION_DATA_1, /**< Second pipe for user data to increase throughput */

    EP_NUM_OPTIONS
  };

  // One RX pipe is dedicated for the TX auto-ack process
  static_assert( EP_NUM_OPTIONS == ( Physical::MAX_NUM_RX_PIPES - 1 ) );
  static_assert( EP_NUM_OPTIONS ==  ARRAY_COUNT( EndpointAddrModifiers ) );

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct TransferControlBlock
  {
    bool inProgress; /**< TX is ongoing and hasn't been acknowledged yet */
    size_t timeout;  /**< Timeout for the transfer */
    size_t start;    /**< Start time for the transfer */
  };

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  /**
   *  Queue that is optimized to hold up to 255 elements at a time. Note
   *  that this is not thread safe and will require protection.
   */
  template<const size_t SIZE>
  using FrameQueue = etl::queue<Frame, SIZE, etl::memory_model::MEMORY_MODEL_SMALL>;

  /**
   *  Buffer that can hold the maximum single data transaction on the radio
   */
  using FrameBuffer = std::array<uint8_t, Physical::MAX_SPI_DATA_LEN>;
}    // namespace Ripple::NetIf::NRF24::DataLink

#endif /* !RIPPLE_DataLink_TYPES_HPP */
