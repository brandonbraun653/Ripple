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
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t SPI_CMD_BYTE_LEN = 1;
  static constexpr size_t MAX_SPI_DATA_LEN = 32;

  /**
   *  Maximum number of bytes that will go out on the wire
   *  during a single SPI transaction. This accounts for the
   *  max frame length (32 bytes) + a command (1 byte )
   */
  static constexpr size_t MAX_SPI_TRANSACTION_LEN = 33;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum PipeBitField_t : uint8_t
  {
    PIPE_BF_0 = ( 1u << 0 ),
    PIPE_BF_1 = ( 1u << 1 ),
    PIPE_BF_2 = ( 1u << 2 ),
    PIPE_BF_3 = ( 1u << 3 ),
    PIPE_BF_4 = ( 1u << 4 ),
    PIPE_BF_5 = ( 1u << 5 ),

    PIPE_BF_MASK = 0x3Fu
  };

  enum PipeNumber : uint8_t
  {
    PIPE_NUM_0   = 0u,
    PIPE_NUM_1   = 1u,
    PIPE_NUM_2   = 2u,
    PIPE_NUM_3   = 3u,
    PIPE_NUM_4   = 4u,
    PIPE_NUM_5   = 5u,
    PIPE_NUM_MAX = 6u,
    PIPE_NUM_ALL = PIPE_NUM_MAX,
    PIPE_INVALID = std::numeric_limits<uint8_t>::max()
  };


  /**
   *   Definitions for tracking the hardware state machine mode
   */
  enum Mode : uint8_t
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
  enum PowerAmplitude : uint8_t
  {
    PA_MIN  = 0u, /**< -18 dBm */
    PA_LOW  = 2u, /**< -12 dBm */
    PA_HIGH = 4u, /**<  -6 dBm */
    PA_MAX  = 6u  /**<   0 dBm */
  };

  /**
   *   Definitions for allowed data rates
   */
  enum DataRate : uint8_t
  {
    DR_1MBPS,  /**< 1 MBPS */
    DR_2MBPS,  /**< 2 MBPS */
    DR_250KBPS /**< 250 KBPS */
  };

  /**
   *   Definitions for CRC settings
   */
  enum CRCLength : uint8_t
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
  enum AddressWidth : uint8_t
  {
    AW_3Byte = 0x01,
    AW_4Byte = 0x02,
    AW_5Byte = 0x03
  };

  /**
   *   Definitions for the auto retransmit delay register field
   */
  enum AutoRetransmitDelay : uint8_t
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


  enum bfISRMask : uint8_t
  {
    ISR_MSK_TX_DS  = ( 1u << 0 ),
    ISR_MSK_RX_DR  = ( 1u << 1 ),
    ISR_MSK_MAX_RT = ( 1u << 2 ),

    ISR_MSK_ALL = ISR_MSK_MAX_RT | ISR_MSK_RX_DR | ISR_MSK_TX_DS
  };

  enum bfControlFlags : uint8_t
  {
    DEV_PLUS_VARIANT     = ( 1u << 0 ), /**< Device is a + variation of NRF24L01 */
    DEV_IS_LISTENING     = ( 1u << 1 ), /**< Device is actively listening */
    DEV_LISTEN_PAUSE     = ( 1u << 2 ), /**< Device has paused listening */
    DEV_DYNAMIC_PAYLOADS = ( 1u << 3 ), /**< Dynamic payloads enabled */
    DEV_FEATURES_ACTIVE  = ( 1u << 4 ), /**< HW feature register enabled */
    DEV_ACK_PAYLOADS     = ( 1u << 5 ), /**< ACK payloads are enabled */
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
  struct DeviceConfig
  {
    /*-------------------------------------------------
    Physical Interface Defintions
    -------------------------------------------------*/
    Chimera::SPI::DriverConfig spi; /**< SPI interface configuration */
    Chimera::GPIO::PinInit irq;     /**< IRQ pin GPIO configuration */
    Chimera::GPIO::PinInit ce;      /**< Chip enable GPIO configuration */

    /*-------------------------------------------------
    Driver Configuration
    -------------------------------------------------*/
    PowerAmplitude hwPowerAmplitude;
    DataRate hwDataRate;
    CRCLength hwCRCLength;
    AddressWidth hwAddressWidth;
    AutoRetransmitDelay hwRTXDelay;
    RFChannel hwRFChannel;
    bfISRMask hwISRMask;
  };


  struct DeviceHandle
  {
    /*-------------------------------------------------
    Physical Interface
    -------------------------------------------------*/
    /* IO Drivers */
    Chimera::SPI::Driver_sPtr spi;     /**< Reference to the SPI driver instance */
    Chimera::GPIO::Driver_sPtr cePin;  /**< Reference to the Chip Enable pin instance */
    Chimera::GPIO::Driver_sPtr irqPin; /**< Reference to the IRQ pin instance */
    Chimera::GPIO::Driver_sPtr csPin;  /**< Chip select GPIO configuration */

    /* Config options */
    DeviceConfig cfg; /**< Configuration options */

    /*-------------------------------------------------
    Driver State
    -------------------------------------------------*/
    bool opened;                                 /**< Whether or not the driver has been enabled/openend */
    uint8_t flags;                               /**< Flags tracking runtime device settings */
    RegisterMap registerCache;                   /**< Tracks the system state as reads/writes occur */
    uint8_t txBuffer[ MAX_SPI_TRANSACTION_LEN ]; /**< Internal transmit buffer */
    uint8_t rxBuffer[ MAX_SPI_TRANSACTION_LEN ]; /**< Internal receive buffer */
    uint64_t cachedPipe0RXAddr;                  /**< RX address cache when Pipe 0 need to become TX */
    // TBD
  };
}    // namespace Ripple::PHY

#endif /* !RIPPLE_PHYSICAL_DEVICE_TYPES_HPP */
