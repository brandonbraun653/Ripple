/********************************************************************************
 *  File Name:
 *    phy_device_constants.hpp
 *
 *  Description:
 *    Constants describing attributes about the NRF24L01 device
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PHYSICAL_DEVICE_CONSTANTS_HPP
#define RIPPLE_PHYSICAL_DEVICE_CONSTANTS_HPP

/* STL Includes */
#include <cstddef>

/* Chimera Includes */
#include <Chimera/spi>

namespace Ripple::Physical
{
  /*-------------------------------------------------
  Driver Configuration
  -------------------------------------------------*/
  static constexpr bool ValidateRegisters = false;

  /*------------------------------------------------
  Hardware Configuration
  ------------------------------------------------*/
  static constexpr Chimera::SPI::BitOrder SPI_BIT_ORDER   = Chimera::SPI::BitOrder::MSB_FIRST;
  static constexpr Chimera::SPI::ClockFreq SPI_MAX_CLOCK  = 8000000;
  static constexpr Chimera::SPI::ClockMode SPI_CLOCK_MODE = Chimera::SPI::ClockMode::MODE0;

  /*------------------------------------------------
  Data Pipe Details
  ------------------------------------------------*/
  static constexpr size_t MAX_NUM_PIPES       = 6;
  static constexpr size_t MAX_NUM_TX_PIPES    = 1;
  static constexpr size_t MAX_NUM_RX_PIPES    = MAX_NUM_PIPES;
  static constexpr size_t MIN_TX_PAYLOAD_SIZE = 0;
  static constexpr size_t MAX_TX_PAYLOAD_SIZE = 32;
  static constexpr size_t DFLT_PAYLOAD_SIZE   = MAX_TX_PAYLOAD_SIZE;
  static constexpr size_t MIN_ADDR_BYTES      = 3;
  static constexpr size_t MAX_ADDR_BYTES      = 5;

}    // namespace Ripple::Physical

#endif /* !RIPPLE_PHYSICAL_DEVICE_CONSTANTS_HPP */
