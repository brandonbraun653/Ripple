/********************************************************************************
 *  File Name:
 *    phy_fsm_controller.hpp
 *
 *  Description:
 *    Finite State Machine Controller interface to manage the radio's state
 *    transition diagram, found in section 6.1.1 of the NRF24L01+ datasheet.
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PHYSICAL_FSM_HPP
#define RIPPLE_PHYSICAL_FSM_HPP

/* Chimera Includes */
#include <Chimera/gpio>

/* ETL Includes */
#include <etl/fsm.h>

/* Project Includes */
#include <Ripple/src/netif/nrf24l01/physical/phy_device_types.hpp>

namespace Ripple::NetIf::NRF24::Physical::FSM
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr etl::message_router_id_t RF_MODE_CONTROL = 0;


  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  /**
   *  Events types that can cause a state transition.
   *
   *  Declared like this because Id must be resolvable to etl::message_id_t
   */
  struct EventId
  {
    enum
    {
      POWER_DOWN,
      POWER_UP,
      START_LISTENING,
      START_TRANSMITTING,
      GO_TO_STANDBY_1
    };
  };


  /**
   *  Primary operating states of the radio. Intermediary states are either
   *  handled implicitly or accounted for in the state transition handler.
   *
   *  Declared like this because Id must be resolvable to etl::message_id_t
   */
  struct StateId
  {
    enum
    {
      POWERED_OFF,
      STANDBY_1,
      RX_MODE,
      TX_MODE,
      NUMBER_OF_STATES
    };
  };

  /*-------------------------------------------------------------------------------
  Classes: Event Messages
  -------------------------------------------------------------------------------*/
  class MsgPowerDown : public etl::message<EventId::POWER_DOWN>
  {
  };

  class MsgPowerUp : public etl::message<EventId::POWER_UP>
  {
  };

  class MsgStartRX : public etl::message<EventId::START_LISTENING>
  {
  };

  class MsgStartTX : public etl::message<EventId::START_TRANSMITTING>
  {
  };

  class MsgGoToSTBY : public etl::message<EventId::GO_TO_STANDBY_1>
  {
  };


  /*-------------------------------------------------------------------------------
  Classes: System Controller Behaviors
  -------------------------------------------------------------------------------*/
  class RadioControl : public etl::fsm
  {
  public:
    RadioControl() : mHandle( nullptr ), fsm( RF_MODE_CONTROL )
    {
    }
    ~RadioControl() = default;

    Handle *mHandle; /**< Device handle used to perform operations */

    /**
     *  Transitions the hardware to RX mode, without regard for the
     *  pre-existing state.
     *
     *  @return bool
     */
    bool transitionToRXMode();

    /**
     *  Transitions the hardware to TX mode, without regard for the
     *  pre-existing state.
     *
     *  @return bool
     */
    bool transitionToTXMode();

    /**
     *  Transitions the hardware to Standby1 mode, without regard for the
     *  pre-existing state.
     *
     *  @return bool
     */
    bool transitionToSTBYMode();

    /**
     *  Transitions the hardware to power down mode, without regard for the
     *  pre-existing state.
     *
     *  @return bool
     */
    bool transitionToPowerDownMode();

    /**
     *  Handle a bad state transition request
     *
     *  @param[in]  msg         The message that was sent
     *  @return void
     */
    void handleBadRequest( const etl::imessage &msg );

    /**
     *  Controls the CE pin for mode transitions
     *
     *  @param[in]  state       Which state to place the GPIO in
     *  @return bool
     */
    bool setChipEnableState( Chimera::GPIO::State state );

    /**
     *  Change the PWR_UP bit in the CONFIG register to the desired state
     *
     *  @param[in]  state       Enable/disable power
     *  @return bool
     */
    bool setPowerUpState( const bool state );

    /**
     *  Change the PRIM_RX bit in the CONFIG register to the desired state
     *
     *  @param[in]  mode        Which state the transciever should go to
     *  @return bool
     */
    bool setTranscieverMode( const TranscieverMode mode );
  };


  /*-------------------------------------------------------------------------------
  Classes: State Implementations
  -------------------------------------------------------------------------------*/
  class PoweredOff : public etl::fsm_state<RadioControl, PoweredOff, StateId::POWERED_OFF, MsgPowerDown, MsgPowerUp>
  {
  public:
    etl::fsm_state_id_t on_enter_state();
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgPowerDown &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgPowerUp &event );
    etl::fsm_state_id_t on_event_unknown( etl::imessage_router &sender, const etl::imessage &event );
  };


  class Standby1 : public etl::fsm_state<RadioControl, Standby1, StateId::STANDBY_1, MsgPowerDown, MsgGoToSTBY, MsgStartRX, MsgStartTX>
  {
  public:
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgPowerDown &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgGoToSTBY &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgStartRX &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgStartTX &event );
    etl::fsm_state_id_t on_event_unknown( etl::imessage_router &sender, const etl::imessage &event );
  };


  class RXMode : public etl::fsm_state<RadioControl, RXMode, StateId::RX_MODE, MsgPowerDown, MsgStartRX, MsgGoToSTBY>
  {
  public:
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgPowerDown &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgStartRX &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgGoToSTBY &event );
    etl::fsm_state_id_t on_event_unknown( etl::imessage_router &sender, const etl::imessage &event );
  };


  class TXMode : public etl::fsm_state<RadioControl, TXMode, StateId::TX_MODE, MsgPowerDown, MsgStartTX, MsgGoToSTBY>
  {
  public:
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgPowerDown &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgStartTX &event );
    etl::fsm_state_id_t on_event( etl::imessage_router &sender, const MsgGoToSTBY &event );
    etl::fsm_state_id_t on_event_unknown( etl::imessage_router &sender, const etl::imessage &event );
  };

}    // namespace Ripple::Physical

#endif /* !RIPPLE_PHYSICAL_FSM_HPP */
