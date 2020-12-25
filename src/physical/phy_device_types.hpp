/********************************************************************************
 *  File Name:
 *    phy_device_types.hpp
 *
 *  Description:
 *    Types and declarations for the RF24 hardware controller
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PHYSICAL_DEVICE_TYPES_HPP
#define RIPPLE_PHYSICAL_DEVICE_TYPES_HPP

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/gpio>
#include <Chimera/spi>


namespace Ripple::PHY
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using RFChannel = uint8_t;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum PipeBitField_t : size_t
  {
    PIPE_BF_0 = ( 1u << 0 ),
    PIPE_BF_1 = ( 1u << 1 ),
    PIPE_BF_2 = ( 1u << 2 ),
    PIPE_BF_3 = ( 1u << 3 ),
    PIPE_BF_4 = ( 1u << 4 ),
    PIPE_BF_5 = ( 1u << 5 ),

    PIPE_BF_MASK = 0x3Fu
  };

  enum PipeNumber : size_t
  {
    PIPE_NUM_0   = 0u,
    PIPE_NUM_1   = 1u,
    PIPE_NUM_2   = 2u,
    PIPE_NUM_3   = 3u,
    PIPE_NUM_4   = 4u,
    PIPE_NUM_5   = 5u,
    PIPE_NUM_MAX = 6u,
    PIPE_NUM_ALL = PIPE_NUM_MAX,
    PIPE_INVALID = std::numeric_limits<size_t>::max()
  };


  /**
   *   Definitions for tracking the hardware state machine mode
   */
  enum Mode : size_t
  {
    MODE_POWER_DOWN = 0,
    MODE_STANDBY_I,
    MODE_STANDBY_II,
    MODE_RX,
    MODE_TX,

    MAX_MODES,
    UNKNOWN_MODE
  };

  /**
   *   Definitions for allowed TX power levels
   */
  enum PowerAmplitude : size_t
  {
    PA_MIN  = 0u, /**< -18 dBm */
    PA_LOW  = 2u, /**< -12 dBm */
    PA_HIGH = 4u, /**<  -6 dBm */
    PA_MAX  = 6u  /**<   0 dBm */
  };

  /**
   *   Definitions for allowed data rates
   */
  enum DataRate : size_t
  {
    DR_1MBPS,  /**< 1 MBPS */
    DR_2MBPS,  /**< 2 MBPS */
    DR_250KBPS /**< 250 KBPS */
  };

  /**
   *   Definitions for CRC settings
   */
  enum CRCLength : size_t
  {
    CRC_DISABLED, /**< No CRC */
    CRC_8,        /**< 8 Bit CRC */
    CRC_16        /**< 16 Bit CRC */
  };

  /**
   *   Definitions for how many address bytes to use. The
   *   numerical value here is NOT the number of bytes. This is the
   *   register level definition.
   */
  enum AddressWidth : size_t
  {
    AW_3Byte = 0x01,
    AW_4Byte = 0x02,
    AW_5Byte = 0x03
  };

  /**
   *   Definitions for the auto retransmit delay register field
   */
  enum AutoRetransmitDelay : size_t
  {
    ART_DELAY_250uS  = 0,
    ART_DELAY_500uS  = 1,
    ART_DELAY_750uS  = 2,
    ART_DELAY_1000uS = 3,
    ART_DELAY_1250uS = 4,
    ART_DELAY_1500uS = 5,
    ART_DELAY_1750uS = 6,
    ART_DELAY_2000uS = 7,
    ART_DELAY_2250uS = 8,
    ART_DELAY_2500uS = 9,
    ART_DELAY_2750uS = 10,
    ART_DELAY_3000uS = 11,
    ART_DELAY_3250uS = 12,
    ART_DELAY_3500uS = 13,
    ART_DELAY_3750uS = 14,
    ART_DELAY_4000uS = 15,

    ART_DELAY_MIN = ART_DELAY_250uS,
    ART_DELAY_MED = ART_DELAY_2250uS,
    ART_DELAY_MAX = ART_DELAY_4000uS
  };


  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   *  Cached version of the internal registers in the NRF24L01. It's
   *  up to the program to properly track the state of these registers.
   *
   *  See datasheet section 9.1 for the referenced mappings.
   */
  struct RegisterMap
  {
    Reg8_t CONFIG;
    Reg8_t EN_AA;
    Reg8_t EN_RX_ADDR;
    Reg8_t SETUP_AW;
    Reg8_t SETUP_RETR;
    Reg8_t RF_CH;
    Reg8_t RF_SETUP;
    Reg8_t STATUS;
    Reg8_t OBSERVE_TX;
    Reg8_t RPD;
    Reg64_t RX_ADDR_P0;
    Reg64_t RX_ADDR_P1;
    Reg8_t RX_ADDR_P2;
    Reg8_t RX_ADDR_P3;
    Reg8_t RX_ADDR_P4;
    Reg8_t RX_ADDR_P5;
    Reg64_t TX_ADDR;
    Reg8_t RX_PW_P0;
    Reg8_t RX_PW_P1;
    Reg8_t RX_PW_P2;
    Reg8_t RX_PW_P3;
    Reg8_t RX_PW_P4;
    Reg8_t RX_PW_P5;
    Reg8_t FIFO_STATUS;
    Reg8_t DYNPD;
    Reg8_t FEATURE;
  };

  /**
   *  NRF24L01 hardware configuration specs
   */
  struct DriverConfig
  {
    /*-------------------------------------------------
    Physical Interface Defintions
    -------------------------------------------------*/
    Chimera::SPI::DriverConfig cfgSPI;    /**< SPI interface configuration */
    Chimera::GPIO::PinInit cfgIRQ;        /**< IRQ pin GPIO configuration */
    Chimera::GPIO::PinInit cfgCE;         /**< Chip enable GPIO configuration */

    /*-------------------------------------------------
    Driver Configuration
    -------------------------------------------------*/
    PowerAmplitude hwPowerAmplitude;
    DataRate hwDataRate;
    CRCLength hwCRCLength;
    AddressWidth hwAddressWidth;
    AutoRetransmitDelay hwRTXDelay;
    RFChannel hwRFChannel;
  };


  struct DriverHandle
  {
    /*-------------------------------------------------
    Physical Interface
    -------------------------------------------------*/
    Chimera::SPI::Driver_sPtr spi;      /**< Reference to the SPI driver instance */
    Chimera::GPIO::Driver_sPtr cePin;   /**< Reference to the Chip Enable pin instance */
    Chimera::GPIO::Driver_sPtr irqPin;  /**< Reference to the IRQ pin instance */

    /*-------------------------------------------------
    Driver State
    -------------------------------------------------*/
    RegisterMap reg;  /**< Tracks the system state as reads/writes occur */


  };
}  // namespace Ripple::PHY

#endif  /* !RIPPLE_PHYSICAL_DEVICE_TYPES_HPP */
