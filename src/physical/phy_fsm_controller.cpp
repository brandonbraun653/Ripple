/********************************************************************************
 *  File Name:
 *    phy_fsm_controller.cpp
 *
 *  Description:
 *    Finite State Machine controller implementation for the NRF24L01+
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/common>

/* ETL Includes */
#include <etl/fsm.h>

/* Ripple Includes */
#include <Ripple/physical>
#include <Ripple/src/physical/phy_device_internal.hpp>

namespace Ripple::PHY::FSM
{
  /*-------------------------------------------------------------------------------
  Classes: System Controller Behaviors
  -------------------------------------------------------------------------------*/
  bool RadioControl::setChipEnableState( Chimera::GPIO::State state )
  {
    return ( mHandle->cePin->setState( state ) == Chimera::Status::OK );
  }


  bool RadioControl::setPowerUpState( const bool state )
  {
    StatusReg_t result;

    if( state )
    {
      result = setRegisterBits( *mHandle, REG_ADDR_CONFIG, CONFIG_PWR_UP );
      mHandle->registerCache.CONFIG |= CONFIG_PWR_UP;
    }
    else
    {
      result = clrRegisterBits( *mHandle, REG_ADDR_CONFIG, CONFIG_PWR_UP );
    }

    return ( result != INVALID_STATUS_REG );
  }


  bool RadioControl::setTranscieverMode( const TranscieverMode mode )
  {
    StatusReg_t result;

    if( mode == TranscieverMode::RECEIVE )
    {
      result = setRegisterBits( *mHandle, REG_ADDR_CONFIG, CONFIG_PRIM_RX );
    }
    else // Transmit
    {
      result = clrRegisterBits( *mHandle, REG_ADDR_CONFIG, CONFIG_PRIM_RX );
    }

    return ( result != INVALID_STATUS_REG );
  }


  bool RadioControl::transitionToRXMode()
  {
    if ( setTranscieverMode( TranscieverMode::RECEIVE ) && setChipEnableState( Chimera::GPIO::State::HIGH ) )
    {
      /*-------------------------------------------------
      Delay the minimum RX settling time
      -------------------------------------------------*/
      Chimera::blockDelayMicroseconds( 130 );
      return true;
    }
    else
    {
      Chimera::insert_debug_breakpoint();
      return false;
    }
  }


  bool RadioControl::transitionToTXMode()
  {
    if ( setTranscieverMode( TranscieverMode::TRANSMIT ) && setChipEnableState( Chimera::GPIO::State::HIGH ) )
    {
      /*-------------------------------------------------
      Delay the minimum TX settling time
      -------------------------------------------------*/
      Chimera::blockDelayMicroseconds( 130 );
      return true;
    }
    else
    {
      Chimera::insert_debug_breakpoint();
      return false;
    }
  }


  bool RadioControl::transitionToSTBYMode()
  {
    if ( setChipEnableState( Chimera::GPIO::State::LOW ) )
    {
      return true;
    }
    else
    {
      Chimera::insert_debug_breakpoint();
      return false;
    }
  }


  bool RadioControl::transitionToPowerDownMode()
  {
    if ( setPowerUpState( false ) && setChipEnableState( Chimera::GPIO::State::LOW ) )
    {
      return true;
    }
    else
    {
      Chimera::insert_debug_breakpoint();
      return false;
    }
  }


  void RadioControl::handleBadRequest( const etl::imessage &msg )
  {
    Chimera::insert_debug_breakpoint();
  }


  /*-------------------------------------------------------------------------------
  Classes: Powered Off State
  -------------------------------------------------------------------------------*/
  etl::fsm_state_id_t PoweredOff::on_enter_state()
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToPowerDownMode() ? StateId::POWERED_OFF : ctx.get_state_id();
  }


  etl::fsm_state_id_t PoweredOff::on_event( etl::imessage_router &sender, const MsgPowerDown &event )
  {
    auto &ctx = get_fsm_context();

    /*-------------------------------------------------
    Don't transition if already in the proper state
    -------------------------------------------------*/
    if ( ctx.get_state_id() == StateId::POWERED_OFF )
    {
      return StateId::POWERED_OFF;
    }

    return ctx.transitionToPowerDownMode() ? StateId::POWERED_OFF : ctx.get_state_id();
  }


  etl::fsm_state_id_t PoweredOff::on_event( etl::imessage_router &sender, const MsgPowerUp &event )
  {
    auto &ctx = get_fsm_context();
    if( ctx.setChipEnableState( Chimera::GPIO::State::LOW ) && ctx.setPowerUpState( true ) )
    {
      /*-------------------------------------------------
      In RM 5.5, the max startup time is Tpd2stby=1.5ms,
      which can be worsened if the Ls parameter exceeds
      30mH. Cover all reasonable startup delays here.
      -------------------------------------------------*/
      Chimera::delayMilliseconds( 5 );
      return StateId::STANDBY_1;
    }
    else
    {
      Chimera::insert_debug_breakpoint();
      return StateId::POWERED_OFF;
    }
  }


  etl::fsm_state_id_t PoweredOff::on_event_unknown( etl::imessage_router &sender, const etl::imessage &event )
  {
    get_fsm_context().handleBadRequest( event );
    return STATE_ID;
  }


  /*-------------------------------------------------------------------------------
  Classes: Standby 1 State
  -------------------------------------------------------------------------------*/
  etl::fsm_state_id_t Standby1::on_event( etl::imessage_router &sender, const MsgPowerDown &event )
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToPowerDownMode() ? StateId::POWERED_OFF : ctx.get_state_id();
  }


  etl::fsm_state_id_t Standby1::on_event( etl::imessage_router &sender, const MsgGoToSTBY &event )
  {
    auto &ctx = get_fsm_context();

    /*-------------------------------------------------
    Already in this mode?
    -------------------------------------------------*/
    if( ctx.get_state_id() == StateId::STANDBY_1)
    {
      return StateId::STANDBY_1;
    }

    /*-------------------------------------------------
    Transition to Standby-1
    -------------------------------------------------*/
    return ctx.transitionToSTBYMode() ? StateId::STANDBY_1 : ctx.get_state_id();
  }


  etl::fsm_state_id_t Standby1::on_event( etl::imessage_router &sender, const MsgStartRX &event )
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToRXMode() ? StateId::RX_MODE : ctx.get_state_id();
  }


  etl::fsm_state_id_t Standby1::on_event( etl::imessage_router &sender, const MsgStartTX &event )
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToTXMode() ? StateId::TX_MODE : ctx.get_state_id();
  }


  etl::fsm_state_id_t Standby1::on_event_unknown( etl::imessage_router &sender, const etl::imessage &event )
  {
    get_fsm_context().handleBadRequest( event );
    return STATE_ID;
  }


  /*-------------------------------------------------------------------------------
  Classes: Receiveing State
  -------------------------------------------------------------------------------*/
  etl::fsm_state_id_t RXMode::on_event( etl::imessage_router &sender, const MsgPowerDown &event )
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToPowerDownMode() ? StateId::POWERED_OFF : ctx.get_state_id();
  }


  etl::fsm_state_id_t RXMode::on_event( etl::imessage_router &sender, const MsgStartRX &event )
  {
    auto &ctx = get_fsm_context();

    /*-------------------------------------------------
    Already in RX mode?
    -------------------------------------------------*/
    if( ctx.get_state_id() == StateId::RX_MODE )
    {
      return StateId::RX_MODE;
    }

    /*-------------------------------------------------
    Transition to RX mode
    -------------------------------------------------*/
    return ctx.transitionToRXMode() ? StateId::RX_MODE : ctx.get_state_id();
  }


  etl::fsm_state_id_t RXMode::on_event( etl::imessage_router &sender, const MsgGoToSTBY &event )
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToSTBYMode() ? StateId::STANDBY_1 : ctx.get_state_id();
  }


  etl::fsm_state_id_t RXMode::on_event_unknown( etl::imessage_router &sender, const etl::imessage &event )
  {
    get_fsm_context().handleBadRequest( event );
    return STATE_ID;
  }


  /*-------------------------------------------------------------------------------
  Classes: Transmitting State
  -------------------------------------------------------------------------------*/
  etl::fsm_state_id_t TXMode::on_event( etl::imessage_router &sender, const MsgPowerDown &event )
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToPowerDownMode() ? StateId::POWERED_OFF : ctx.get_state_id();
  }


  etl::fsm_state_id_t TXMode::on_event( etl::imessage_router &sender, const MsgStartTX &event )
  {
    auto &ctx = get_fsm_context();

    /*-------------------------------------------------
    Already in TX mode?
    -------------------------------------------------*/
    if( ctx.get_state_id() == StateId::TX_MODE )
    {
      return StateId::TX_MODE;
    }

    /*-------------------------------------------------
    Transition to TX mode
    -------------------------------------------------*/
    return ctx.transitionToTXMode() ? StateId::TX_MODE : ctx.get_state_id();
  }


  etl::fsm_state_id_t TXMode::on_event( etl::imessage_router &sender, const MsgGoToSTBY &event )
  {
    auto &ctx = get_fsm_context();
    return ctx.transitionToSTBYMode() ? StateId::STANDBY_1 : ctx.get_state_id();
  }


  etl::fsm_state_id_t TXMode::on_event_unknown( etl::imessage_router &sender, const etl::imessage &event )
  {
    get_fsm_context().handleBadRequest( event );
    return STATE_ID;
  }

}  // namespace Ripple::PHY
