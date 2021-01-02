/********************************************************************************
 *  File Name:
 *    data_link_service.hpp
 *
 *  Description:
 *    Data link layer thread interface
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_DATA_LINK_THREAD_HPP
#define RIPPLE_DATA_LINK_THREAD_HPP

/* Chimera Includes */
#include <Chimera/thread>

/* ETL Includes */
#include <etl/delegate_service.h>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_memory_config.hpp>
#include <Ripple/src/datalink/data_link_types.hpp>
#include <Ripple/src/datalink/data_link_arp.hpp>
#include <Ripple/src/physical/phy_fsm_controller.hpp>
#include <Ripple/src/session/session_types.hpp>

namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t THREAD_STACK          = STACK_BYTES( 1024 );
  static constexpr std::string_view THREAD_NAME = "datalink";


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Thread class that performs the datalink layer services, whose primary
   *  responsibilities are to:
   *
   *    1. Control data TX/RX on the physical medium
   *    2. Data queuing and scheduling
   *    3. Translate logical network addresses in to physical addresses
   *
   *  A Frame is the primary data type this layer understands how to process.
   */
  class Service : Chimera::Threading::Lockable
  {
  public:
    Service();
    ~Service();

    /**
     *  Main thread that executes the datalink layer services
     *
     *  @param[in]  context     Critical net stack information
     *  @return void
     */
    void run( Session::Context context );

    /**
     *  Schedules a frame to be transmitted through the physical layer the next time
     *  the TX queues are processed.
     *
     *  @param[in]  frame       The frame to be transmitted
     *  @return Chimera::Status_t
     */
    Chimera::Status_t enqueueFrame( const Frame &frame );

    /**
     *  Returns the next frame, if available, from the RX queue
     *
     *  @param[out] frame       The frame to dump data into
     *  @return Chimera::Status_t
     */
    Chimera::Status_t dequeueFrame( Frame &frame );

    /**
     *  Adds a new entry to the layer's ARP table
     *
     *  @param[in]  ip          IP address to key off of
     *  @param[in]  mac         MAC address associated with the IP
     *  @return Chimera::Status_t
     */
    Chimera::Status_t addARPEntry( const NET::IPAddress ip, const PHY::MACAddress &mac );

    /**
     *  Removes an entry from the layer's ARP table
     *
     *  @param[in]  ip          IP address of entry to remove
     *  @return Chimera::Status_t
     */
    Chimera::Status_t dropARPEntry( const NET::IPAddress ip );

    /**
     *  Register a callback to be invoked upon some event that occurs during
     *  the service processing.
     *
     *  @param[in]  id          Which event to register against
     *  @param[in]  func        The function to register
     *  @return Chimera::Status_t
     */
    Chimera::Status_t registerCallback( const CallbackId id, etl::delegate<void( size_t )> func );

    /**
     *  Gets the event data associated with a particular callback ID
     *
     *  @param[in]  id          The callback event id to query
     *  @param[out] data        Where to copy the output data into
     *  @return bool            If the copy was successful
     */
    bool queryCallbackData( const CallbackId id, void *const data );

  protected:
    /**
     *  Initialize the service to prepare it for first time execution
     *
     *  @param[in]  session     User session information
     *  @return Chimera::Status_t
     */
    Chimera::Status_t initialize( Session::Context session );

    /**
     *  Initializes the radio with the user configured settings
     *
     *  @param[in]  session     User session information
     *  @return Chimera::Status_t
     */
    Chimera::Status_t powerUpRadio( Session::Context session );

    /**
     *  Callback that will be invoked when the radio's IRQ pin is asserted. This
     *  method will execute in ISR space, so take care with function execution time.
     *
     *  @param[in]  arg         Unused, but required for callback signature
     *  @return void
     */
    void irqPinAsserted( void *arg );

    /**
     *  Handle the IRQ event when a transmission succeeded
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processTXSuccess( Session::Context context );

    /**
     *  Handle the IRQ event when a transmission failed
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processTXFail( Session::Context context );

    /**
     *  Periodic process to transmit data that has been enqueued with the service
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processTXQueue( Session::Context context );

    /**
     *  Periodic process to read a frame off the radio and enqueues it
     *  for handling by higher layers.
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processRXQueue( Session::Context context );

  private:
    /*-------------------------------------------------
    Class State Data
    -------------------------------------------------*/
    bool mSystemEnabled;                    /**< Gating signal for the ISR handler to prevent spurrious interrupts */
    bool mTXInProgress;                     /**< TX is ongoing and hasn't been ACK'd yet */
    volatile bool pendingEvent;             /**< Signal flag set by an ISR that an event occurred */
    Chimera::Threading::ThreadId mThreadId; /**< Thread registration ID */

    /*-------------------------------------------------
    Helper for tracking/invoking callbacks
    -------------------------------------------------*/
    etl::delegate_service<CallbackId::CB_NUM_OPTIONS> mDelegateRegistry;

    /*-------------------------------------------------
    TX/RX Queues
    -------------------------------------------------*/
    FrameQueue<TX_QUEUE_ELEMENTS> mTXQueue;           /**< Queue for data destined for the physical layer */
    Chimera::Threading::RecursiveTimedMutex mTXMutex; /**< Thread safety lock */
    FrameQueue<RX_QUEUE_ELEMENTS> mRXQueue;           /**< Queue for data coming from the physical layer */
    Chimera::Threading::RecursiveTimedMutex mRXMutex; /**< Thread safety lock */

    /*-------------------------------------------------
    Lookup table for known devices
    -------------------------------------------------*/
    ARPCache mAddressCache;

    /*-------------------------------------------------
    FSM Controller
    -------------------------------------------------*/
    PHY::FSM::RadioControl mFSMControl;
    PHY::FSM::PoweredOff _fsmState_PoweredOff;
    PHY::FSM::Standby1 _fsmState_Standby1;
    PHY::FSM::RXMode _fsmState_RXMode;
    PHY::FSM::TXMode _fsmState_TXMode;
    etl::ifsm_state* _fsmStateList[ PHY::FSM::StateId::NUMBER_OF_STATES ];
  };
}    // namespace Ripple::DATALINK

#endif /* !RIPPLE_DATA_LINK_THREAD_HPP */
