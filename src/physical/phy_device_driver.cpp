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
    uint8_t reg;    /**< Register address */
    uint8_t val;    /**< Value to write */
    uint8_t rwMask; /**< Masks off only the R/W fields */
  };

  /*-------------------------------------------------------------------------------
  Static Data
  -------------------------------------------------------------------------------*/
  /* clang-format off */
  static const RegisterDefaults sRegDefaults[] = {
    /* clang-format off */
    { REG_ADDR_CONFIG,      CONFIG_Reset,       CONFIG_Mask     },
    { REG_ADDR_EN_AA,       EN_AA_Reset,        EN_AA_Mask      },
    { REG_ADDR_EN_RXADDR,   EN_RXADDR_Reset,    EN_RXADDR_Mask  },
    { REG_ADDR_SETUP_AW,    SETUP_AW_Reset,     SETUP_AW_Mask   },
    { REG_ADDR_SETUP_RETR,  SETUP_RETR_Reset,   SETUP_RETR_Mask },
    { REG_ADDR_RF_CH,       RF_CH_Reset,        RF_CH_Mask      },
    { REG_ADDR_RF_SETUP,    RF_SETUP_Reset,     RF_SETUP_Mask   },
    { REG_ADDR_RX_ADDR_P2,  RX_ADDR_P2_Reset,   RX_ADDR_P2_Mask },
    { REG_ADDR_RX_ADDR_P3,  RX_ADDR_P3_Reset,   RX_ADDR_P3_Mask },
    { REG_ADDR_RX_ADDR_P4,  RX_ADDR_P4_Reset,   RX_ADDR_P4_Mask },
    { REG_ADDR_RX_ADDR_P5,  RX_ADDR_P5_Reset,   RX_ADDR_P5_Mask },
    { REG_ADDR_RX_PW_P0,    RX_PW_P0_Reset,     RX_PW_P0_Mask   },
    { REG_ADDR_RX_PW_P1,    RX_PW_P1_Reset,     RX_PW_P1_Mask   },
    { REG_ADDR_RX_PW_P2,    RX_PW_P2_Reset,     RX_PW_P2_Mask   },
    { REG_ADDR_RX_PW_P3,    RX_PW_P3_Reset,     RX_PW_P3_Mask   },
    { REG_ADDR_RX_PW_P4,    RX_PW_P4_Reset,     RX_PW_P4_Mask   },
    { REG_ADDR_RX_PW_P5,    RX_PW_P5_Reset,     RX_PW_P5_Mask   },
    { REG_ADDR_DYNPD,       DYNPD_Reset,        DYNPD_Mask      },
    { REG_ADDR_FEATURE,     FEATURE_Reset,      FEATURE_Mask    }
    /* clang-format on */
  };

  static const uint8_t rxPipeAddressRegister[ MAX_NUM_PIPES ] = {
    REG_ADDR_RX_ADDR_P0,
    REG_ADDR_RX_ADDR_P1,
    REG_ADDR_RX_ADDR_P2,
    REG_ADDR_RX_ADDR_P3,
    REG_ADDR_RX_ADDR_P4,
    REG_ADDR_RX_ADDR_P5
  };

  static const uint8_t rxPipePayloadWidthRegister[ MAX_NUM_PIPES ] = {
    REG_ADDR_RX_PW_P0,
    REG_ADDR_RX_PW_P1,
    REG_ADDR_RX_PW_P2,
    REG_ADDR_RX_PW_P3,
    REG_ADDR_RX_PW_P4,
    REG_ADDR_RX_PW_P5
  };

  static const uint8_t rxPipeEnableBitField[ MAX_NUM_PIPES ] = {
    EN_RXADDR_P0,
    EN_RXADDR_P1,
    EN_RXADDR_P2,
    EN_RXADDR_P3,
    EN_RXADDR_P4,
    EN_RXADDR_P5
  };

  static const uint8_t rxPipeEnableDPLMask[ MAX_NUM_PIPES ] = {
    DYNPD_DPL_P0,
    DYNPD_DPL_P1,
    DYNPD_DPL_P2,
    DYNPD_DPL_P3,
    DYNPD_DPL_P4,
    DYNPD_DPL_P5
  };

  static const uint8_t rxPipeEnableAAMask[ MAX_NUM_PIPES ] = {
    EN_AA_P0,
    EN_AA_P1,
    EN_AA_P2,
    EN_AA_P3,
    EN_AA_P4,
    EN_AA_P5
  };
  /* clang-format on */

  /*-------------------------------------------------------------------------------
  Static Functions
  -------------------------------------------------------------------------------*/
  static bool driverReady( Handle &handle )
  {
    /*-------------------------------------------------
    For now, this is just a single flag
    -------------------------------------------------*/
    return handle.opened;
  }

  /*-------------------------------------------------------------------------------
  Utility Functions
  -------------------------------------------------------------------------------*/
  Handle *getHandle( Session::Context session )
  {
    if ( !session )
    {
      return nullptr;
    }
    else
    {
      auto context  = reinterpret_cast<Session::Handle *>( session );
      auto physical = reinterpret_cast<PHY::Handle *>( context->physical );

      return physical;
    }
  }


  /*-------------------------------------------------------------------------------
  Open/Close Functions
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openDevice( const DeviceConfig &cfg, Handle &handle )
  {
    constexpr uint8_t TestChannel = 103;

    /*-------------------------------------------------
    By this point, the project should have initialized
    all the hardware IO drivers appropriately. Try to
    communicate with the device.
    -------------------------------------------------*/
    handle.opened = true; // Temporarily set to true to the read/write can work
    handle.cfg    = cfg;

    writeRegister( handle, REG_ADDR_RF_CH, TestChannel );
    uint8_t val = readRegister( handle, REG_ADDR_RF_CH );

    if( val == TestChannel )
    {
      return Chimera::Status::OK;
    }
    else
    {
      handle.opened = false;
      return Chimera::Status::FAIL;
    }
  }


  Chimera::Status_t closeDevice( Handle &handle )
  {
    // Clear the handle memory, including the drivers.

    return Chimera::Status::NOT_SUPPORTED;
  }


  /*-------------------------------------------------------------------------------
  Device Commands
  -------------------------------------------------------------------------------*/
  Chimera::Status_t resetRegisterDefaults( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Reset each register back to power-on defaults
    -------------------------------------------------*/
    for ( size_t x = 0; x < ARRAY_COUNT( sRegDefaults ); x++ )
    {
      /*-------------------------------------------------
      Write the register, then read it back
      -------------------------------------------------*/
      uint8_t maskedValue = sRegDefaults[ x ].val & sRegDefaults[ x ].rwMask;
      writeRegister( handle, sRegDefaults[ x ].reg, maskedValue );
      uint8_t readValue = readRegister( handle, sRegDefaults[ x ].reg );

      /*-------------------------------------------------
      Verify register settings match
      -------------------------------------------------*/
      if ( ( readValue & sRegDefaults[ x ].rwMask ) != maskedValue )
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

    /*-------------------------------------------------
    Handle the STATUS register, which must be cleared
    by setting bits instead.
    -------------------------------------------------*/
    writeRegister( handle, REG_ADDR_STATUS, STATUS_Clear );

    return Chimera::Status::OK;
  }


  Chimera::Status_t flushTX( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Flush the hardware/software TX buffers
    -------------------------------------------------*/
    writeCommand( handle, CMD_FLUSH_TX );
    memset( handle.txBuffer, 0, ARRAY_BYTES( Handle::txBuffer ) );
    return Chimera::Status::OK;
  }


  Chimera::Status_t flushRX( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Flush the hardware/software RX buffers
    -------------------------------------------------*/
    writeCommand( handle, CMD_FLUSH_RX );
    memset( handle.rxBuffer, 0, ARRAY_BYTES( Handle::rxBuffer ) );
    return Chimera::Status::OK;
  }


  Chimera::Status_t startListening( Handle &handle )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if ( handle.flags & DEV_IS_LISTENING )
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
    if ( handle.cachedPipe0RXAddr )
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


  Chimera::Status_t stopListening( Handle &handle )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if ( !( handle.flags & ( DEV_IS_LISTENING | DEV_LISTEN_PAUSE ) ) )
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


  Chimera::Status_t pauseListening( Handle &handle )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) || !( handle.flags & DEV_IS_LISTENING ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if ( handle.flags & DEV_LISTEN_PAUSE )
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


  Chimera::Status_t resumeListening( Handle &handle )
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


  Chimera::Status_t toggleAckPayloads( Handle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Enable/disable the ack payload functionality
    -------------------------------------------------*/
    if ( state && !( handle.flags & DEV_ACK_PAYLOADS ) )
    {
      if ( !( handle.flags & DEV_FEATURES_ACTIVE ) )
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
    else if ( handle.flags & DEV_ACK_PAYLOADS )    // && !state
    {
      clrRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_DPL );
      clrRegisterBits( handle, REG_ADDR_DYNPD, DYNPD_DPL_P0 | DYNPD_DPL_P1 );

      handle.flags &= ~DEV_ACK_PAYLOADS;
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleDynamicPayloads( Handle &handle, const PipeNumber pipe, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if( !( pipe < PIPE_NUM_MAX ) && !( pipe == PIPE_NUM_ALL ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Determine mask to apply to EN_AA & DYNPD registers
    -------------------------------------------------*/
    uint8_t en_aa_mask = 0;
    uint8_t dynpd_mask = 0;

    if( pipe == PIPE_NUM_ALL )
    {
      en_aa_mask = EN_AA_Mask;
      dynpd_mask = DYNPD_Mask;
    }
    else
    {
      en_aa_mask = rxPipeEnableAAMask[ pipe ];
      dynpd_mask = rxPipeEnableDPLMask[ pipe ];
    }

    /*-------------------------------------------------
    Enable/disable the dynamic payload functionality
    -------------------------------------------------*/
    if ( state )
    {
      /*-------------------------------------------------
      Send the activate command to enable features
      -------------------------------------------------*/
      if ( !( handle.flags & DEV_FEATURES_ACTIVE ) )
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
      setRegisterBits( handle, REG_ADDR_EN_AA, en_aa_mask );
      setRegisterBits( handle, REG_ADDR_DYNPD, dynpd_mask );

      handle.flags |= DEV_FEATURES_ACTIVE;
    }
    else if ( !state && ( handle.flags & DEV_FEATURES_ACTIVE ) )
    {
      /*-------------------------------------------------
      Disable for all pipes
      -------------------------------------------------*/
      clrRegisterBits( handle, REG_ADDR_EN_AA, en_aa_mask );
      clrRegisterBits( handle, REG_ADDR_DYNPD, dynpd_mask );
      clrRegisterBits( handle, REG_ADDR_FEATURE, FEATURE_EN_DPL );

      handle.flags &= ~DEV_FEATURES_ACTIVE;
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleDynamicAck( Handle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Enable/disable dynamic ack
    -------------------------------------------------*/
    if ( state )
    {
      if ( !( handle.flags & DEV_FEATURES_ACTIVE ) )
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

    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleAutoAck( Handle &handle, const bool state, const PipeNumber pipe )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Figure out how many pipes to operate on
    -------------------------------------------------*/
    uint8_t mask = 0;
    if ( pipe == PIPE_NUM_ALL )
    {
      mask = EN_AA_Mask;
    }
    else if ( pipe < MAX_NUM_PIPES )
    {
      mask = rxPipeEnableAAMask[ pipe ];
    }
    else
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Write the data
    -------------------------------------------------*/
    if ( state )
    {
      setRegisterBits( handle, REG_ADDR_EN_AA, mask );
    }
    else
    {
      clrRegisterBits( handle, REG_ADDR_EN_AA, mask );
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleFeatures( Handle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Enable/disable the features functionality
    -------------------------------------------------*/
    if ( state && !( handle.flags & DEV_FEATURES_ACTIVE ) )
    {
      writeCommand( handle, CMD_ACTIVATE, &FEATURE_ACTIVATE_KEY, 1 );
      handle.flags |= DEV_FEATURES_ACTIVE;
    }
    else if ( !state && ( handle.flags & DEV_FEATURES_ACTIVE ) )
    {
      writeCommand( handle, CMD_ACTIVATE, &FEATURE_ACTIVATE_KEY, 1 );
      handle.flags &= ~DEV_FEATURES_ACTIVE;
    }
    // else nothing to do

    return Chimera::Status::OK;
  }


  Chimera::Status_t togglePower( Handle &handle, const bool state )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Set the power state
    -------------------------------------------------*/
    if( state )
    {
      setRegisterBits( handle, REG_ADDR_CONFIG, CONFIG_PWR_UP );
    }
    else
    {
      clrRegisterBits( handle, REG_ADDR_CONFIG, CONFIG_PWR_UP );
    }

    return Chimera::Status::OK;
  }


  /*-------------------------------------------------------------------------------
  Data Pipe Operations
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openWritePipe( Handle &handle, const MACAddress address )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Cache the currently configured RX address
    -------------------------------------------------*/
    readRegister( handle, REG_ADDR_RX_ADDR_P0, &handle.cachedPipe0RXAddr, MAX_ADDR_BYTES );

    /*-------------------------------------------------
    Set pipe 0 RX address == TX address. This allows the
    reception of an ACK packet from the node at the TX
    address.
    -------------------------------------------------*/
    writeRegister( handle, REG_ADDR_RX_ADDR_P0, &address, MAX_ADDR_BYTES );
    writeRegister( handle, REG_ADDR_TX_ADDR, &address, MAX_ADDR_BYTES );

    return Chimera::Status::OK;
  }


  Chimera::Status_t closeWritePipe( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Clobber the TX pipe address
    -------------------------------------------------*/
    MACAddress clobber = 0;
    writeRegister( handle, REG_ADDR_TX_ADDR, &clobber, MAX_ADDR_BYTES );

    /*-------------------------------------------------
    If possible, clobber the RX pipe as well. This is
    allowed if the device is in TX mode, aka not
    listening and not paused listening.
    -------------------------------------------------*/
    if ( !( handle.flags & ( DEV_IS_LISTENING | DEV_LISTEN_PAUSE ) ) )
    {
      writeRegister( handle, REG_ADDR_RX_ADDR_P0, &clobber, MAX_ADDR_BYTES );
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t openReadPipe( Handle &handle, const PipeNumber pipe, const MACAddress address )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if( !( pipe < PIPE_NUM_MAX ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Assign the address for the pipe to listen against
    -------------------------------------------------*/
    size_t addressBytes  = 0u;
    MACAddress addressMask = 0u;

    if ( ( pipe == PIPE_NUM_0 ) || ( pipe == PIPE_NUM_1 ) )
    {
      /*-------------------------------------------------
      Write only as many address bytes as set in SETUP_AW
      -------------------------------------------------*/
      addressBytes = getAddressWidth( handle );
      addressMask  = 0xFFFFFFFFFFu;
      writeRegister( handle, rxPipeAddressRegister[ pipe ], &address, addressBytes );

      /*------------------------------------------------
      Save the pipe 0 address because it is clobbered by
      openWritePipe() and will need to be restored later
      when we start listening again.
      ------------------------------------------------*/
      if ( pipe == PIPE_NUM_0 )
      {
        memcpy( &handle.cachedPipe0RXAddr, &address, addressBytes );
      }
    }
    else
    {
      addressBytes = 1u;
      addressMask  = 0xFFu;

      /*------------------------------------------------
      These pipes only need the first bytes assigned as
      they take the rest of their address from PIPE_NUM_0.
      ------------------------------------------------*/
      writeRegister( handle, rxPipeAddressRegister[ pipe ], &address, addressBytes );
    }

    /*------------------------------------------------
    Optionally validate the write
    ------------------------------------------------*/
    if constexpr ( ValidateRegisters )
    {
      uint64_t writtenAddress = 0u;
      readRegister( handle, rxPipeAddressRegister[ pipe ], &writtenAddress, addressBytes );

      if ( writtenAddress != ( address & addressMask ) )
      {
        return Chimera::Status::FAIL;
      }
    }

    /*-------------------------------------------------
    Write the payload width, then turn the pipe on
    -------------------------------------------------*/
    uint8_t payloadSize = DFLT_PAYLOAD_SIZE;
    if( handle.flags & DEV_DYNAMIC_PAYLOADS )
    {
      // TODO: Verify if this still allows for dynamic payloads. Datasheet says zero == disabled, but
      //       it might be what makes this mode work for some of the cheaper chinese clones.
      payloadSize = 0;
    }

    writeRegister( handle, rxPipePayloadWidthRegister[ pipe ], payloadSize );
    setRegisterBits( handle, REG_ADDR_EN_RXADDR, rxPipeEnableBitField[ pipe ] );

    return Chimera::Status::OK;
  }


  Chimera::Status_t closeReadPipe( Handle &handle, const PipeNumber pipe )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if( !( pipe < PIPE_NUM_MAX ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Disable the RX pipe
    -------------------------------------------------*/
    clrRegisterBits( handle, REG_ADDR_EN_RXADDR, rxPipeEnableBitField[ pipe ] );

    /*-------------------------------------------------
    Clear the payload width
    -------------------------------------------------*/
    writeRegister( handle, rxPipePayloadWidthRegister[ pipe ], 0 );

    /*-------------------------------------------------
    Clear the RX address
    -------------------------------------------------*/
    MACAddress clobberAddress = 0;
    if ( ( pipe == PIPE_NUM_0 ) || ( pipe == PIPE_NUM_1 ) )
    {
      writeRegister( handle, rxPipeAddressRegister[ pipe ], &clobberAddress, MAX_ADDR_BYTES );
    }
    else
    {
      writeRegister( handle, rxPipeAddressRegister[ pipe ], &clobberAddress, 1u );
    }

    /*-------------------------------------------------
    If using dynamic payloads, disable it
    -------------------------------------------------*/
    if( handle.flags & DEV_DYNAMIC_PAYLOADS )
    {
      clrRegisterBits( handle, REG_ADDR_DYNPD, rxPipeEnableDPLMask[ pipe ] );
      clrRegisterBits( handle, REG_ADDR_EN_AA, rxPipeEnableAAMask[ pipe ] );
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t readPayload( Handle &handle, void *const buffer, const size_t length )
  {
    using namespace Chimera::GPIO;

    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if ( !buffer || !length )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    The chip enable pin must be low to read out data.
    This places the device in Standby-1 mode.
    -------------------------------------------------*/
    State prevState = State::LOW;
    handle.cePin->getState( prevState );

    if( prevState == State::HIGH )
    {
      handle.cePin->setState( State::LOW );
    }

    /*-------------------------------------------------
    Read out the payload
    -------------------------------------------------*/
    size_t readLength = std::min( length, MAX_TX_PAYLOAD_SIZE );
    readCommand( handle, CMD_R_RX_PAYLOAD, buffer, readLength );

    /*-------------------------------------------------
    Reset the CE pin back to original state if needed.
    -------------------------------------------------*/
    if( prevState == State::HIGH )
    {
      handle.cePin->setState( prevState );
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t writePayload( Handle &handle, const void *const buffer, const size_t length, const PayloadType type )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if ( !buffer || !length )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Select the appropriate write command
    -------------------------------------------------*/
    uint8_t cmd = CMD_W_TX_PAYLOAD;
    if( type == PayloadType::PAYLOAD_NO_ACK )
    {
      cmd = CMD_W_TX_PAYLOAD_NO_ACK;
    }

    /*-------------------------------------------------
    Send the data
    -------------------------------------------------*/
    writeCommand( handle, cmd, buffer, length );

    return Chimera::Status::OK;
  }


  Chimera::Status_t stageAckPayload( Handle &handle, const PipeNumber pipe, const void *const buffer, size_t length )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if ( !buffer || !length || !( pipe < PIPE_NUM_MAX ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Perform the staging command
    -------------------------------------------------*/
    size_t writeLength = std::min( length, MAX_TX_PAYLOAD_SIZE );
    writeCommand( handle, CMD_W_ACK_PAYLOAD, buffer, writeLength );

    return Chimera::Status::OK;
  }


  /*-------------------------------------------------------------------------------
  Data Setters/Getters
  -------------------------------------------------------------------------------*/
  Reg8_t getStatusRegister( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return 0;
    }

    return writeCommand( handle, CMD_NOP );
  }


  bool rxFifoFull( Handle &handle )
  {
    Reg8_t status = readRegister( handle, REG_ADDR_FIFO_STATUS );
    return status & FIFO_STATUS_RX_FULL;
  }


  bool rxFifoEmpty( Handle &handle )
  {
    Reg8_t status = readRegister( handle, REG_ADDR_FIFO_STATUS );
    return status & FIFO_STATUS_RX_EMPTY;
  }


  bool txFifoFull( Handle &handle )
  {
    Reg8_t status = readRegister( handle, REG_ADDR_FIFO_STATUS );
    return status & FIFO_STATUS_TX_FULL;
  }


  bool txFifoEmpty( Handle &handle )
  {
    Reg8_t status = readRegister( handle, REG_ADDR_FIFO_STATUS );
    return status & FIFO_STATUS_TX_EMPTY;
  }


  Chimera::Status_t setPALevel( Handle &handle, const RFPower level )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
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


  RFPower getPALevel( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      Chimera::insert_debug_breakpoint();
      return RFPower::PA_LOW;
    }

    /*-------------------------------------------------
    Read and convert the latest data
    -------------------------------------------------*/
    uint8_t setup                 = readRegister( handle, REG_ADDR_RF_SETUP );
    handle.registerCache.RF_SETUP = setup;

    return static_cast<RFPower>( ( setup & RF_SETUP_RF_PWR ) >> 1 );
  }


  Chimera::Status_t setDataRate( Handle &handle, const DataRate speed )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
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


  DataRate getDataRate( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      Chimera::insert_debug_breakpoint();
      return DataRate::DR_250KBPS;
    }

    /*-------------------------------------------------
    Convert the current register setting
    -------------------------------------------------*/
    uint8_t setup                 = readRegister( handle, REG_ADDR_RF_SETUP );
    handle.registerCache.RF_SETUP = setup;
    return static_cast<DataRate>( setup & ( RF_SETUP_RF_DR_HIGH | RF_SETUP_RF_DR_LOW ) );
  }


  Chimera::Status_t setRetries( Handle &handle, const AutoRetransmitDelay delay, const size_t count )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
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


  AutoRetransmitDelay getRTXDelay( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return AutoRetransmitDelay::ART_DELAY_UNKNOWN;
    }

    /*-------------------------------------------------
    Read the register and convert
    -------------------------------------------------*/
    uint8_t val = readRegister( handle, REG_ADDR_SETUP_RETR );
    return static_cast<AutoRetransmitDelay>( ( val & SETUP_RETR_ARD_Msk ) >> SETUP_RETR_ARD_Pos );
  }


  AutoRetransmitCount getRTXCount( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return AutoRetransmitCount::ART_COUNT_INVALID;
    }

    /*-------------------------------------------------
    Read the register and convert
    -------------------------------------------------*/
    uint8_t val = readRegister( handle, REG_ADDR_SETUP_RETR );
    return static_cast<AutoRetransmitCount>( ( val & SETUP_RETR_ARC_Msk ) >> SETUP_RETR_ARC_Pos );
  }


  Chimera::Status_t setRFChannel( Handle &handle, const size_t channel )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
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


  size_t getRFChannel( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Channel is directly convertible from register value
    -------------------------------------------------*/
    auto channel               = readRegister( handle, REG_ADDR_RF_CH );
    handle.registerCache.RF_CH = channel;
    return static_cast<size_t>( channel );
  }


  Chimera::Status_t setISRMasks( Handle &handle, const uint8_t msk )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Read the current config settings and mask off the
    existing flag configuration.
    -------------------------------------------------*/
    uint8_t cfg = readRegister( handle, REG_ADDR_CONFIG );
    cfg |= ( CONFIG_MASK_MAX_RT | CONFIG_MASK_RX_DR | CONFIG_MASK_TX_DS );

    /*-------------------------------------------------
    Build the mask. Note the masks use negative logic,
    so clearing the bit means enabling the interrupt.
    -------------------------------------------------*/
    if ( msk & bfISRMask::ISR_MSK_MAX_RT )
    {
      cfg &= ~CONFIG_MASK_MAX_RT;
    }

    if ( msk & bfISRMask::ISR_MSK_RX_DR )
    {
      cfg &= ~CONFIG_MASK_RX_DR;
    }

    if ( msk & bfISRMask::ISR_MSK_TX_DS )
    {
      cfg &= ~CONFIG_MASK_TX_DS;
    }

    /*-------------------------------------------------
    Write the updated mask settings
    -------------------------------------------------*/
    writeRegister( handle, REG_ADDR_CONFIG, cfg );
    return Chimera::Status::OK;
  }


  uint8_t getISRMasks( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return bfISRMask::ISR_NONE;
    }

    /*-------------------------------------------------
    Get the config register and look at each bit. Note
    that the masks use negative logic, so a cleared bit
    means the interrupt is enabled.
    -------------------------------------------------*/
    uint8_t msk = 0;
    uint8_t cfg = readRegister( handle, REG_ADDR_CONFIG );

    if ( !( cfg & CONFIG_MASK_MAX_RT ) )
    {
      msk |= bfISRMask::ISR_MSK_MAX_RT;
    }

    if ( !( cfg & CONFIG_MASK_TX_DS ) )
    {
      msk |= bfISRMask::ISR_MSK_TX_DS;
    }

    if ( !( cfg & CONFIG_MASK_RX_DR ) )
    {
      msk |= bfISRMask::ISR_MSK_RX_DR;
    }

    return msk;
  }


  Chimera::Status_t clrISREvent( Handle &handle, const bfISRMask msk )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Events are cleared by writing a 1 to the STATUS
    register bits. Only the ISR flags are writable, so
    don't bother with a read/modify/write operation.
    -------------------------------------------------*/
    uint8_t sts = 0;

    if ( msk & bfISRMask::ISR_MSK_MAX_RT )
    {
      sts |= STATUS_MAX_RT;
    }

    if ( msk & bfISRMask::ISR_MSK_RX_DR )
    {
      sts |= STATUS_RX_DR;
    }

    if ( msk & bfISRMask::ISR_MSK_TX_DS )
    {
      sts |= STATUS_TX_DS;
    }

    /*-------------------------------------------------
    Write the updated mask settings
    -------------------------------------------------*/
    writeRegister( handle, REG_ADDR_STATUS, sts );
    return Chimera::Status::OK;
  }


  uint8_t getISREvent( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return bfISRMask::ISR_NONE;
    }

    /*-------------------------------------------------
    Get the status register and look at each bit
    -------------------------------------------------*/
    uint8_t msk = 0;
    uint8_t sts = readRegister( handle, REG_ADDR_STATUS );

    if ( sts & STATUS_MAX_RT )
    {
      msk |= bfISRMask::ISR_MSK_MAX_RT;
    }

    if ( sts & STATUS_TX_DS )
    {
      msk |= bfISRMask::ISR_MSK_TX_DS;
    }

    if ( sts & STATUS_RX_DR )
    {
      msk |= bfISRMask::ISR_MSK_RX_DR;
    }

    return msk;
  }


  Chimera::Status_t setAddressWidth( Handle &handle, const AddressWidth width )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Convert the enum into the appropriate register val
    -------------------------------------------------*/
    uint8_t val = 0;
    switch ( width )
    {
      case AddressWidth::AW_3Byte:
        val = 0x01;
        break;

      case AddressWidth::AW_4Byte:
        val = 0x02;
        break;

      case AddressWidth::AW_5Byte:
        val = 0x03;
        break;

      default:
        return Chimera::Status::INVAL_FUNC_PARAM;
        break;
    };

    writeRegister( handle, REG_ADDR_SETUP_AW, val );
    return Chimera::Status::OK;
  }


  AddressWidth getAddressWidth( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return AddressWidth::AW_INVALID;
    }

    /*-------------------------------------------------
    Parse the register setting
    -------------------------------------------------*/
    uint8_t val = readRegister( handle, REG_ADDR_SETUP_AW );
    switch ( val )
    {
      case 0x01:
        return AddressWidth::AW_3Byte;
        break;

      case 0x02:
        return AddressWidth::AW_4Byte;
        break;

      case 0x03:
        return AddressWidth::AW_5Byte;
        break;

      default:
        return AddressWidth::AW_INVALID;
        break;
    };
  }


  Chimera::Status_t setCRCLength( Handle &handle, const CRCLength length )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }

    /*-------------------------------------------------
    Clear the existing config and use the new one
    -------------------------------------------------*/
    uint8_t config = readRegister( handle, REG_ADDR_CONFIG );
    config &= ~( CONFIG_CRCO | CONFIG_EN_CRC );

    switch ( length )
    {
      case CRCLength::CRC_8:
        config |= CONFIG_EN_CRC;
        config &= ~CONFIG_CRCO;
        break;

      case CRCLength::CRC_16:
        config |= CONFIG_EN_CRC | CONFIG_CRCO;
        break;

      default:
        return Chimera::Status::INVAL_FUNC_PARAM;
        break;
    }

    writeRegister( handle, REG_ADDR_CONFIG, config );
    return Chimera::Status::OK;
  }


  CRCLength getCRCLength( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return CRCLength::CRC_UNKNOWN;
    }

    /*-------------------------------------------------
    CRC is either enabled manually, or automatically
    enabled by hardware when auto-ack is enabled.
    -------------------------------------------------*/
    CRCLength result = CRCLength::CRC_DISABLED;
    uint8_t config   = readRegister( handle, REG_ADDR_CONFIG );
    uint8_t en_aa    = readRegister( handle, REG_ADDR_EN_AA );


    if ( ( config & CONFIG_EN_CRC ) || en_aa )
    {
      if ( config & CONFIG_CRCO )
      {
        result = CRCLength::CRC_16;
      }
      else
      {
        result = CRCLength::CRC_8;
      }
    }

    return result;
  }


  Chimera::Status_t setStaticPayloadSize( Handle &handle, const size_t size, const PipeNumber pipe )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) )
    {
      return Chimera::Status::NOT_AVAILABLE;
    }
    else if ( !( pipe < ARRAY_COUNT( rxPipePayloadWidthRegister ) ) || !( size <= MAX_TX_PAYLOAD_SIZE ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Set the static payload size
    -------------------------------------------------*/
    if ( pipe == PIPE_NUM_ALL )
    {
      for ( auto x = 0; x < PIPE_NUM_MAX; x++ )
      {
        writeRegister( handle, rxPipePayloadWidthRegister[ x ], static_cast<uint8_t>( size ) );
      }
    }
    else
    {
      writeRegister( handle, rxPipePayloadWidthRegister[ pipe ], static_cast<uint8_t>( size ) );
    }

    return Chimera::Status::OK;
  }


  size_t getStaticPayloadSize( Handle &handle, const PipeNumber pipe )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) || !( pipe < ARRAY_COUNT( rxPipePayloadWidthRegister ) ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Read the static width
    -------------------------------------------------*/
    return readRegister( handle, rxPipePayloadWidthRegister[ pipe ] );
  }


  PipeNumber getAvailablePayloadPipe( Handle &handle )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) || rxFifoEmpty( handle ) )
    {
      return PipeNumber::PIPE_INVALID;
    }

    /*-------------------------------------------------
    Read the status register to get appropriate pipe
    -------------------------------------------------*/
    uint8_t sts = getStatusRegister( handle );
    return static_cast<PipeNumber>( ( sts & STATUS_RX_P_NO_Msk ) >> STATUS_RX_P_NO_Pos );
  }


  size_t getAvailablePayloadSize( Handle &handle, const PipeNumber pipe )
  {
    /*-------------------------------------------------
    Entrance Checks
    -------------------------------------------------*/
    if ( !driverReady( handle ) || !( pipe < ARRAY_COUNT( rxPipePayloadWidthRegister ) ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Using dynamic payloads? Grab the last reported size
    else get the pipe's static configuration.
    -------------------------------------------------*/
    if ( handle.flags & DEV_DYNAMIC_PAYLOADS )
    {
      uint8_t tmp = 0;
      readCommand( handle, CMD_R_RX_PL_WID, &tmp, 1u );
      return tmp;
    }
    else
    {
      return getStaticPayloadSize( handle, pipe );
    }
  }

}    // namespace Ripple::PHY
