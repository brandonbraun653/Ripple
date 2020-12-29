/********************************************************************************
 *  File Name:
 *    phy_device_driver.cpp
 *
 *  Description:
 *    Implements the physical device driver for an NRF24L01 module
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/common>

/* Ripple Includes */
#include <Ripple/physical>
#include <Ripple/src/physical/phy_device_internal.hpp>


namespace Ripple::PHY
{
  /*-------------------------------------------------------------------------------
  Private Structures
  -------------------------------------------------------------------------------*/
  struct RegisterDefaults
  {
    uint8_t reg;
    uint8_t val;
  };

  /*-------------------------------------------------------------------------------
  Static Data
  -------------------------------------------------------------------------------*/
  static const RegisterDefaults sRegDefaults[] = {
    /* clang-format off */
    { REG_ADDR_CONFIG, CONFIG_Reset },
    { REG_ADDR_EN_AA, EN_AA_Reset },
    { REG_ADDR_EN_RXADDR, EN_RXADDR_Reset },
    { REG_ADDR_SETUP_AW, SETUP_AW_Reset },
    { REG_ADDR_SETUP_RETR, SETUP_RETR_Reset },
    { REG_ADDR_RF_CH, RF_CH_Reset },
    { REG_ADDR_RF_SETUP, RF_SETUP_Reset },
    { REG_ADDR_STATUS, STATUS_Reset },
    { REG_ADDR_OBSERVE_TX, OBSERVE_TX_Reset },
    { REG_ADDR_RX_ADDR_P2, RX_ADDR_P2_Reset },
    { REG_ADDR_RX_ADDR_P3, RX_ADDR_P3_Reset },
    { REG_ADDR_RX_ADDR_P4, RX_ADDR_P4_Reset },
    { REG_ADDR_RX_ADDR_P5, RX_ADDR_P5_Reset },
    { REG_ADDR_RX_PW_P0, RX_PW_P0_Reset },
    { REG_ADDR_RX_PW_P1, RX_PW_P1_Reset },
    { REG_ADDR_RX_PW_P2, RX_PW_P2_Reset },
    { REG_ADDR_RX_PW_P3, RX_PW_P3_Reset },
    { REG_ADDR_RX_PW_P4, RX_PW_P4_Reset },
    { REG_ADDR_RX_PW_P5, RX_PW_P5_Reset },
    { REG_ADDR_FIFO_STATUS, FIFO_STATUS_Reset },
    { REG_ADDR_DYNPD, DYNPD_Reset },
    { REG_ADDR_FEATURE, FEATURE_Reset }
    /* clang-format on */
  };

  /*-------------------------------------------------------------------------------
  Static Functions
  -------------------------------------------------------------------------------*/
  static bool driverReady( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    For now, this is just a single flag
    -------------------------------------------------*/
    return handle.opened;
  }

  /*-------------------------------------------------------------------------------
  Utility Functions
  -------------------------------------------------------------------------------*/
  DeviceHandle *getHandle( SessionContext session )
  {
    if ( !session )
    {
      return nullptr;
    }
    else
    {
      auto context  = reinterpret_cast<NetStackHandle *>( session );
      auto physical = reinterpret_cast<PHY::DeviceHandle *>( context->physical );

      return physical;
    }
  }


  /*-------------------------------------------------------------------------------
  Open/Close Functions
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openDevice( const DeviceConfig &cfg, DeviceHandle &handle )
  {
    // Do not perform the configuration here. Simply validate that the device
    // is connected on the configured GPIO/SPI drivers.
    handle.opened = true;
    handle.cfg = cfg;

    return Chimera::Status::OK;
  }


  Chimera::Status_t closeDevice( DeviceHandle &handle )
  {
    // Clear the handle memory, including the drivers.

    return Chimera::Status::NOT_SUPPORTED;
  }


  /*-------------------------------------------------------------------------------
  Device Commands
  -------------------------------------------------------------------------------*/
  Chimera::Status_t resetRegisterDefaults( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Reset each register back to power-on defaults
    -------------------------------------------------*/
    auto result = Chimera::Status::OK;
    for ( size_t x = 0; x < ARRAY_COUNT( sRegDefaults ); x++ )
    {
      result |= writeRegister( handle, sRegDefaults[ x ].reg, sRegDefaults[ x ].val );
      auto val = readRegister( handle, sRegDefaults[ x ].reg );

      if ( ( val != sRegDefaults[ x ].val ) || ( result != Chimera::Status::OK ) )
      {
        Chimera::insert_debug_breakpoint();
        return Chimera::Status::FAIL;
      }
    }

    /*-------------------------------------------------
    Handle the multi-byte registers
    -------------------------------------------------*/
    writeRegister( handle, REG_ADDR_TX_ADDR, &TX_ADDR_Reset, TX_ADDR_byteWidth );
    writeRegister( handle, REG_ADDR_RX_ADDR_P0, &RX_ADDR_P0_Reset, RX_ADDR_P0_byteWidth );
    writeRegister( handle, REG_ADDR_RX_ADDR_P1, &RX_ADDR_P1_Reset, RX_ADDR_P1_byteWidth );

    return result;
  }


  Chimera::Status_t flushTX( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Flush the hardware/software TX buffers
    -------------------------------------------------*/
    writeCommand( handle, CMD_FLUSH_TX );
    memset( handle.txBuffer, 0, ARRAY_BYTES( DeviceHandle::txBuffer ) );
    return Chimera::Status::OK;
  }


  Chimera::Status_t flushRX( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Flush the hardware/software RX buffers
    -------------------------------------------------*/
    writeCommand( handle, CMD_FLUSH_RX );
    memset( handle.rxBuffer, 0, ARRAY_BYTES( DeviceHandle::rxBuffer ) );
    return Chimera::Status::OK;
  }


  Chimera::Status_t startListening( DeviceHandle &handle )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if( handle.flags & DEV_IS_LISTENING )
    {
      return Chimera::Status::OK;
    }

    /*-------------------------------------------------
    Transition back to Standby-1 mode
    -------------------------------------------------*/
    handle.cePin->setState( State::LOW );

    /*-------------------------------------------------
    If we are auto-acknowledging RX packets with a payload,
    make sure the TX FIFO is clean so we don't accidentally
    transmit data on the next transition back to TX mode.
    -------------------------------------------------*/
    if ( registerIsBitmaskSet( handle, REG_ADDR_FEATURE, FEATURE_EN_ACK_PAY ) )
    {
      flushTX( handle );
    }

    /*-------------------------------------------------
    Clear interrupt flags and transition to RX mode by
    setting PRIM_RX=1 and CE=1. Wait the required ~130uS
    RX settling time needed to get into RX mode.
    -------------------------------------------------*/
    setRegisterBits( handle, REG_ADDR_STATUS, ( STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT ) );
    setRegisterBits( handle, REG_ADDR_CONFIG, CONFIG_PRIM_RX );
    handle.cePin->setState( State::HIGH );
    Chimera::delayMilliseconds( 1 );

    /*-------------------------------------------------
    If the Pipe 0 RX address was previously clobbered
    so that a TX could occur, restore the address.
    -------------------------------------------------*/
    if( handle.cachedPipe0RXAddr )
    {
      openReadPipe( handle, PIPE_NUM_0, handle.cachedPipe0RXAddr );
    }

    /*-------------------------------------------------
    Update listener status flags
    -------------------------------------------------*/
    handle.flags &= ~( DEV_IS_LISTENING | DEV_LISTEN_PAUSE );
    handle.flags |= DEV_IS_LISTENING;

    return Chimera::Status::OK;
  }


  Chimera::Status_t stopListening( DeviceHandle &handle )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if( !( handle.flags & ( DEV_IS_LISTENING | DEV_LISTEN_PAUSE ) ) )
    {
      return Chimera::Status::OK;
    }

    /*-------------------------------------------------
    Transition device into standby mode 1
    -------------------------------------------------*/
    handle.cePin->setState( State::LOW );
    clrRegisterBits( handle, REG_ADDR_CONFIG, CONFIG_PRIM_RX );

    /*-------------------------------------------------
    If we are auto-acknowledging RX packets with a payload,
    make sure the TX FIFO is clean so we don't accidentally
    transmit data on the next transition back to TX mode.
    -------------------------------------------------*/
    if ( registerIsBitmaskSet( handle, REG_ADDR_FEATURE, FEATURE_EN_ACK_PAY ) )
    {
      flushTX( handle );
    }

    /*-------------------------------------------------
    Update listener status flags
    -------------------------------------------------*/
    handle.flags &= ~( DEV_IS_LISTENING | DEV_LISTEN_PAUSE );

    return Chimera::Status::OK;
  }


  Chimera::Status_t pauseListening( DeviceHandle &handle )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) || !( handle.flags & DEV_IS_LISTENING ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if( handle.flags & DEV_LISTEN_PAUSE )
    {
      return Chimera::Status::OK;
    }

    /*-------------------------------------------------
    According to the state transition diagram, the
    module is in RX mode when PRIM_RX=1 and CE=1. By
    clearing CE=0, we can transition the module to
    Standby-1 mode.
    -------------------------------------------------*/
    handle.cePin->setState( State::LOW );
    handle.flags |= ( DEV_IS_LISTENING | DEV_LISTEN_PAUSE );

    return Chimera::Status::OK;
  }


  Chimera::Status_t resumeListening( DeviceHandle &handle )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) ||
         ( ( handle.flags & ( DEV_IS_LISTENING | DEV_LISTEN_PAUSE ) ) != ( DEV_IS_LISTENING | DEV_LISTEN_PAUSE ) ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    According to the state transition diagram, the
    module is in Standby-1 mode when PRIM_RX=1, CE=0.
    By setting CE=1, we can transition the module back
    to RX mode.
    -------------------------------------------------*/
    handle.cePin->setState( State::HIGH );
    handle.flags &= ~DEV_LISTEN_PAUSE;

    /* The transition requires an RX settling period of ~130us */
    Chimera::delayMilliseconds( 1 );
    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleAckPayloads( DeviceHandle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Enable/disable the ack payload functionality
    -------------------------------------------------*/
    if( state && !( handle.flags & DEV_ACK_PAYLOADS ) )
    {
      if( !( handle.flags & DEV_FEATURES_ACTIVE ) )
      {
        toggleFeatures( handle, true );
      }

      /*-------------------------------------------------
      Enable the proper bits
      // TODO: Why only P0/P1? Check datasheet then make comment
      -------------------------------------------------*/
      setRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_ACK_PAY );
      setRegisterBits( handle, REG_ADDR_DYNPD, DYNPD_DPL_P0 | DYNPD_DPL_P1 );

      handle.flags |= DEV_ACK_PAYLOADS;
    }
    else if( handle.flags & DEV_ACK_PAYLOADS ) // && !state
    {
      clrRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_DPL );
      clrRegisterBits( handle, REG_ADDR_DYNPD, DYNPD_DPL_P0 | DYNPD_DPL_P1 );

      handle.flags &= ~DEV_ACK_PAYLOADS;
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleDynamicPayloads( DeviceHandle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Enable/disable the dynamic payload functionality
    -------------------------------------------------*/
    if ( state )
    {
      /*-------------------------------------------------
      Send the activate command to enable features
      -------------------------------------------------*/
      if( !( handle.flags & DEV_FEATURES_ACTIVE ) )
      {
        toggleFeatures( handle, true );
      }

      /*-------------------------------------------------
      Enable the dynamic payload feature bit
      -------------------------------------------------*/
      setRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_DPL );

      /*-------------------------------------------------
      Enable dynamic payload on all pipes. This requires that
      auto-acknowledge be enabled.
      -------------------------------------------------*/
      setRegisterBits( handle, REG_ADDR_EN_AA, EN_AA_Mask );
      setRegisterBits( handle, REG_ADDR_DYNPD, DYNPD_Mask );

      handle.flags |= DEV_FEATURES_ACTIVE;
    }
    else if ( !state && ( handle.flags & DEV_FEATURES_ACTIVE ) )
    {
      /*-------------------------------------------------
      Disable for all pipes
      -------------------------------------------------*/
      clrRegisterBits( handle, REG_ADDR_DYNPD, DYNPD_Mask );
      clrRegisterBits( handle, REG_ADDR_EN_AA, EN_AA_Mask );
      clrRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_DPL );

      handle.flags &= ~DEV_FEATURES_ACTIVE;
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleDynamicAck( DeviceHandle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Enable/disable dynamic ack
    -------------------------------------------------*/
    if ( state )
    {
      if( !( handle.flags & DEV_FEATURES_ACTIVE ) )
      {
        toggleFeatures( handle, true );
      }

      setRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_DYN_ACK );
    }
    else if ( handle.flags & DEV_FEATURES_ACTIVE )
    {
      clrRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_DYN_ACK );
    }
    // else features are disabled, so dynamic ack is too
  }


  Chimera::Status_t toggleAutoAck( DeviceHandle &handle, const bool state, const PipeNumber pipe )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Figure out how many pipes to operate on
    -------------------------------------------------*/
    if ( pipe == PIPE_NUM_ALL )
    {
      if ( state )
      {
        setRegisterBits( handle, REG_ADDR_EN_AA, EN_AA_Mask );
      }
      else
      {
        clrRegisterBits( handle, REG_ADDR_EN_AA, EN_AA_Mask );
      }
    }
    else if ( pipe < MAX_NUM_PIPES )
    {
      uint8_t en_aa = readRegister( handle, REG_ADDR_EN_AA );

      if ( state )
      {
        en_aa |= 1u << pipe;
      }
      else
      {
        en_aa &= ~( 1u << pipe );
      }

      writeRegister( handle, REG_ADDR_EN_AA, en_aa );
    }
    else
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleFeatures( DeviceHandle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Enable/disable the features functionality
    -------------------------------------------------*/
    if( state && !( handle.flags & DEV_FEATURES_ACTIVE ) )
    {
      writeCommand( handle, CMD_ACTIVATE, &FEATURE_ACTIVATE_KEY, 1 );
      handle.flags |= DEV_FEATURES_ACTIVE;
    }
    else if( !state && ( handle.flags & DEV_FEATURES_ACTIVE ) )
    {
      writeCommand( handle, CMD_ACTIVATE, &FEATURE_ACTIVATE_KEY, 1 );
      handle.flags &= ~DEV_FEATURES_ACTIVE;
    }
    // else nothing to do
  }


  /*-------------------------------------------------------------------------------
  Data Pipe Operations
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openWritePipe( DeviceHandle &handle, const MACAddress address )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t closeWritePipe( DeviceHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t openReadPipe( DeviceHandle &handle, const PipeNumber pipe, const MACAddress address )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t closeReadPipe( DeviceHandle &handle, const PipeNumber pipe )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t readFIFO( DeviceHandle &handle, void *const buffer, const size_t length )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t writePipe( DeviceHandle &handle, const void *const buffer, const size_t length )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t stageAckPayload( DeviceHandle &handle, const PipeNumber pipe, const void *const buffer, size_t length )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  /*-------------------------------------------------------------------------------
  Data Setters/Getters
  -------------------------------------------------------------------------------*/
  Reg8_t getStatusRegister( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return 0;
    }

    return writeCommand( handle, CMD_NOP );
  }


  Chimera::Status_t setPALevel( DeviceHandle &handle, const PowerAmplitude level )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Merge bits from level into setup according to a mask
    https://graphics.stanford.edu/~seander/bithacks.html#MaskedMerge
    -------------------------------------------------*/
    uint8_t setup = readRegister( handle, REG_ADDR_RF_SETUP );
    setup ^= ( setup ^ static_cast<uint8_t>( level ) ) & RF_SETUP_RF_PWR_Msk;

    /*-------------------------------------------------
    Update the register setting
    -------------------------------------------------*/
    writeRegister( handle, REG_ADDR_RF_SETUP, setup );
    handle.registerCache.RF_SETUP = setup;

    return Chimera::Status::OK;
  }


  PowerAmplitude getPALevel( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      Chimera::insert_debug_breakpoint();
      return PowerAmplitude::PA_LOW;
    }

    /*-------------------------------------------------
    Read and convert the latest data
    -------------------------------------------------*/
    uint8_t setup = readRegister( handle, REG_ADDR_RF_SETUP );
    handle.registerCache.RF_SETUP = setup;

    return static_cast<PowerAmplitude>( ( setup & RF_SETUP_RF_PWR ) >> 1 );
  }


  Chimera::Status_t setDataRate( DeviceHandle &handle, const DataRate speed )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*------------------------------------------------
    Cache the current setup so we don't blow away bits
    ------------------------------------------------*/
    Reg8_t setup = readRegister( handle, REG_ADDR_RF_SETUP );

    /*------------------------------------------------
    Decide which bits need to be set/cleared
    ------------------------------------------------*/
    switch ( speed )
    {
      case DataRate::DR_250KBPS:
        setup |= RF_SETUP_RF_DR_LOW;
        setup &= ~RF_SETUP_RF_DR_HIGH;
        break;

      case DataRate::DR_1MBPS:
        setup &= ~( RF_SETUP_RF_DR_HIGH | RF_SETUP_RF_DR_LOW );
        break;

      case DataRate::DR_2MBPS:
        setup &= ~RF_SETUP_RF_DR_LOW;
        setup |= RF_SETUP_RF_DR_HIGH;
        break;

      default:
        return Chimera::Status::INVAL_FUNC_PARAM;
        break;
    }

    /*------------------------------------------------
    Write the configuration and verify it was set properly
    ------------------------------------------------*/
    writeRegister( handle, REG_ADDR_RF_SETUP, setup );
    handle.registerCache.RF_SETUP = setup;

    return Chimera::Status::OK;
  }


  DataRate getDataRate( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      Chimera::insert_debug_breakpoint();
      return DataRate::DR_250KBPS;
    }

    /*-------------------------------------------------
    Convert the current register setting
    -------------------------------------------------*/
    uint8_t setup = readRegister( handle, REG_ADDR_RF_SETUP );
    handle.registerCache.RF_SETUP = setup;
    return static_cast<DataRate>( setup & ( RF_SETUP_RF_DR_HIGH | RF_SETUP_RF_DR_LOW ) );
  }


  Chimera::Status_t setRetries( DeviceHandle &handle, const AutoRetransmitDelay delay, const size_t count )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Convert to the proper register settings
    -------------------------------------------------*/
    Reg8_t ard        = ( static_cast<Reg8_t>( delay ) & 0x0F ) << SETUP_RETR_ARD_Pos;
    Reg8_t arc        = ( count & 0x0F ) << SETUP_RETR_ARC_Pos;
    Reg8_t setup_retr = ard | arc;

    writeRegister( handle, REG_ADDR_SETUP_RETR, setup_retr );
    handle.registerCache.SETUP_RETR = setup_retr;
    return Chimera::Status::OK;
  }


  AutoRetransmitDelay getRTXDelay( DeviceHandle &handle )
  {
  }


  size_t getRTXCount( DeviceHandle &handle )
  {
  }


  Chimera::Status_t setRFChannel( DeviceHandle &handle, const size_t channel )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Write the RF channel
    -------------------------------------------------*/
    auto maskedChannel = static_cast<Reg8_t>( channel & RF_CH_Mask );
    writeRegister( handle, REG_ADDR_RF_CH, maskedChannel );
    handle.registerCache.RF_CH = maskedChannel;

    return Chimera::Status::OK;
  }


  size_t getRFChannel( DeviceHandle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if( !driverReady( handle ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Channel is directly convertible from register value
    -------------------------------------------------*/
    auto channel = readRegister( handle, REG_ADDR_RF_CH );
    handle.registerCache.RF_CH = channel;
    return static_cast<size_t>( channel );
  }


  Chimera::Status_t setISRMasks( DeviceHandle &handle, const bfISRMask msk )
  {

  }


  bfISRMask getISRMasks( DeviceHandle &handle )
  {

  }


  Chimera::Status_t setAddressWidth( DeviceHandle &handle, const AddressWidth width )
  {

  }


  AddressWidth getAddressWidth( DeviceHandle &handle )
  {

  }


  Chimera::Status_t setCRCLength( DeviceHandle &handle, const CRCLength length )
  {

  }


  CRCLength getCRCLength( DeviceHandle &handle )
  {

  }


  Chimera::Status_t setStaticPayloadSize( DeviceHandle &handle, const size_t size, const PipeNumber pipe )
  {
  }


  size_t getStaticPayloadSize( DeviceHandle &handle, const PipeNumber pipe )
  {
  }


  PipeNumber getFIFOPayloadAvailable( DeviceHandle &handle )
  {
  }


  size_t getFIFOPayloadSize( DeviceHandle &handle )
  {
  }

}    // namespace Ripple::PHY
