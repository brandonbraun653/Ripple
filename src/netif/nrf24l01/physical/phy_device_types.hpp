/********************************************************************************
 *  File Name:
 *    phy_device_types.hpp
 *
 *  Description:
 *    Types and declarations for the RF24 hardware controller
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PHYSICAL_DEVICE_TYPES_HPP
#define RIPPLE_PHYSICAL_DEVICE_TYPES_HPP

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/exti>
#include <Chimera/gpio>
#include <Chimera/spi>

/* Ripple Includes */
#include <Ripple/src/netif/nrf24l01/physical/phy_device_constants.hpp>

#if defined( SIMULATOR )
#include <mutex>
#include <zmq.hpp>
#endif /* SIMULATOR */

namespace Ripple::NetIf::NRF24::Physical
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using RFChannel  = uint8_t;
  using MACAddress = uint64_t; /**< Hardware address uniquely identifying a pipe in the network */

  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t SPI_CMD_BYTE_LEN = 1;
  static constexpr size_t MAX_SPI_DATA_LEN = 32;

  /**
   *  Maximum number of bytes that will go out on the wire during a single SPI
   *  transaction. This accounts for max frame length (32 bytes) + command (1 byte).
   */
  static constexpr size_t MAX_SPI_TRANSACTION_LEN = 33;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  /**
   *  Payload classification type. Used to inform the hardware what
   *  kinds of bits to set during data packing.
   */
  enum class PayloadType : uint8_t
  {
    PAYLOAD_REQUIRES_ACK,
    PAYLOAD_NO_ACK
  };

  /**
   *  Compact way to represent multiple pipes in a single field
   */
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

  /**
   *  Identification of individual pipes as opposed to PipeBitField_t
   */
  enum PipeNumber : uint8_t
  {
    PIPE_NUM_0,
    PIPE_NUM_1,
    PIPE_NUM_2,
    PIPE_NUM_3,
    PIPE_NUM_4,
    PIPE_NUM_5,
    PIPE_NUM_ALL,
    PIPE_INVALID = std::numeric_limits<uint8_t>::max()
  };

  /**
   *  Modes the transceiver may be in
   */
  enum TranscieverMode : uint8_t
  {
    TRANSMIT,
    RECEIVE
  };

  /**
   *  Definitions for allowed TX power levels
   */
  enum RFPower : uint8_t
  {
    PA_LVL_0, /**< -18 dBm */
    PA_LVL_1, /**< -12 dBm */
    PA_LVL_2, /**<  -6 dBm */
    PA_LVL_3, /**<   0 dBm */

    PA_INVALID
  };

  /**
   *  Definitions for allowed data rates
   */
  enum DataRate : uint8_t
  {
    DR_1MBPS,   /**< 1 MBPS */
    DR_2MBPS,   /**< 2 MBPS */
    DR_250KBPS, /**< 250 KBPS */

    DR_NUM_OPTIONS,
    DR_INVALID
  };

  /**
   *  Definitions for CRC settings
   */
  enum CRCLength : uint8_t
  {
    CRC_DISABLED, /**< No CRC */
    CRC_8,        /**< 8 Bit CRC */
    CRC_16,       /**< 16 Bit CRC */

    CRC_NUM_OPTIONS,
    CRC_UNKNOWN
  };

  /**
   *  Definitions for how many address bytes to use. The numerical value here
   *  is NOT the number of bytes. This is the register level definition.
   */
  enum AddressWidth : uint8_t
  {
    AW_3Byte = 3,
    AW_4Byte,
    AW_5Byte,

    AW_NUM_OPTIONS,
    AW_INVALID
  };

  /**
   *  Definitions for the auto retransmit delay register field
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

    ART_DELAY_MIN     = ART_DELAY_250uS,
    ART_DELAY_MED     = ART_DELAY_2250uS,
    ART_DELAY_MAX     = ART_DELAY_4000uS,
    ART_DELAY_UNKNOWN = std::numeric_limits<uint8_t>::max()
  };

  /**
   *  Number of transmit retry attempts that will be made before giving up
   */
  enum AutoRetransmitCount : uint8_t
  {
    ART_COUNT_DISABLED,
    ART_COUNT_1,
    ART_COUNT_2,
    ART_COUNT_3,
    ART_COUNT_4,
    ART_COUNT_5,
    ART_COUNT_6,
    ART_COUNT_7,
    ART_COUNT_8,
    ART_COUNT_9,
    ART_COUNT_10,
    ART_COUNT_11,
    ART_COUNT_12,
    ART_COUNT_13,
    ART_COUNT_14,
    ART_COUNT_15,
    ART_COUNT_INVALID
  };

  /**
   *  Compact way to represent multiple ISR events in a single field
   */
  enum bfISRMask : uint8_t
  {
    ISR_NONE       = 0,
    ISR_MSK_TX_DS  = ( 1u << 0 ),
    ISR_MSK_RX_DR  = ( 1u << 1 ),
    ISR_MSK_MAX_RT = ( 1u << 2 ),

    ISR_MSK_ALL = ISR_MSK_MAX_RT | ISR_MSK_RX_DR | ISR_MSK_TX_DS
  };

  /**
   *  Compact way to represent several command and control options in a single field
   */
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

    Chimera::EXTI::EdgeTrigger irqEdge;

    /*-------------------------------------------------
    Driver Configuration
    -------------------------------------------------*/
    uint8_t hwChannel;   /**< Radio channel number in range [1, 255] */
    size_t networkBaud;  /**< Desired effective link speed in kbps */
    IPAddress ipAddress; /**< Static address of this device */
    uint8_t hwStaticPayloadWidth;
    RFPower hwPowerAmplitude;
    DataRate hwDataRate;
    CRCLength hwCRCLength;
    MACAddress hwAddress;
    AddressWidth hwAddressWidth;
    AutoRetransmitDelay hwRTXDelay;
    RFChannel hwRFChannel;
    bfISRMask hwISRMask;
    bool verifyRegisters; /**< Runtime verification of register setting updates */

    void clear()
    {
      hwAddress            = 0;
      hwAddressWidth       = Physical::AddressWidth::AW_INVALID;
      hwChannel            = 0;
      hwDataRate           = Physical::DataRate::DR_INVALID;
      hwPowerAmplitude     = Physical::RFPower::PA_INVALID;
      hwStaticPayloadWidth = static_cast<uint8_t>( Physical::MAX_TX_PAYLOAD_SIZE );
      ipAddress            = 0;
      networkBaud          = 0;
      verifyRegisters      = true;
    }
  };


#if defined( SIMULATOR )
  /**
   *  Network configuration and control options with ZeroMQ. This is
   *  what forms the virtual hardware.
   */
  struct ZMQConfig
  {
    std::recursive_mutex lock;                 /**< Thread safety lock */
    zmq::context_t context;                    /**< Communication context */
    zmq::socket_t txPipe;                      /**< TX Pipe Endpoint */
    zmq::socket_t rxPipes[ MAX_NUM_RX_PIPES ]; /**< RX Pipe Endpoints */

    std::string txEndpoint;
    std::string rxEndpoints[ MAX_NUM_RX_PIPES ];
  };
#endif /* SIMULATOR */

  /**
   *  Core structure for the physical module. This contains all state information
   *  regarding a PHY driver for a single radio.
   */
  struct Handle
  {
    /*-------------------------------------------------
    Physical Interface
    -------------------------------------------------*/
    /* IO Drivers */
    Chimera::SPI::Driver_rPtr spi;     /**< Reference to the SPI driver instance */
    Chimera::GPIO::Driver_rPtr cePin;  /**< Reference to the Chip Enable pin instance */
    Chimera::GPIO::Driver_rPtr irqPin; /**< Reference to the IRQ pin instance */
    Chimera::GPIO::Driver_rPtr csPin;  /**< Chip select GPIO configuration */

    /* Config options */
    DeviceConfig cfg;

    /*-------------------------------------------------
    Driver State
    -------------------------------------------------*/
    bool opened;                                 /**< Whether or not the driver has been enabled/openend */
    uint8_t flags;                               /**< Flags tracking runtime device settings */
    uint8_t lastStatus;                          /**< Debug variable to track last status register returned in transaction */
    RegisterMap registerCache;                   /**< Tracks the system state as reads/writes occur */
    uint8_t txBuffer[ MAX_SPI_TRANSACTION_LEN ]; /**< Internal transmit buffer */
    uint8_t rxBuffer[ MAX_SPI_TRANSACTION_LEN ]; /**< Internal receive buffer */
    uint64_t cachedPipe0RXAddr;                  /**< RX address cache when Pipe 0 need to become TX */

    /**
     * Time to wait for a hardware IRQ event (ms) to instruct the DataLink
     * layer it has new events to process. This also has the effect of setting
     * the minimum polling rate for processing the transmit and receive queues.
     */
    size_t hwIRQEventTimeout;

    /**
     *  Tracks the number of RX queue overflow events that have occurred since
     *  power up. Useful to see if the DataLink layer has enough bandwidth to
     *  process the queue or if the network layer isn't emptying it fast enough.
     */
    size_t rxQueueOverflows;

    /**
     *  Tracks the number of TX queue overflow events that have occurred since
     *  power up. Useful to see if the DataLink layer is being overloaded or too
     *  much data is being pumped in by the network layer.
     */
    size_t txQueueOverflows;


    /*-------------------------------------------------
    Virtual Driver
    -------------------------------------------------*/
#if defined( SIMULATOR )
    ZMQConfig netCfg;
#endif /* SIMULATOR */

    /*-------------------------------------------------
    Helper Functions
    -------------------------------------------------*/
    void clear()
    {
      cfg.clear();

      opened            = false;
      flags             = 0;
      lastStatus        = 0;
      cachedPipe0RXAddr = 0;
      hwIRQEventTimeout = 25;
      rxQueueOverflows  = 0;
      txQueueOverflows  = 0;
      memset( &registerCache, 0, sizeof( RegisterMap ) );
      memset( txBuffer, 0, ARRAY_BYTES( txBuffer ) );
      memset( rxBuffer, 0, ARRAY_BYTES( rxBuffer ) );
    }
  };

}    // namespace Ripple::NetIf::NRF24::Physical

#endif /* !RIPPLE_PHYSICAL_DEVICE_TYPES_HPP */
