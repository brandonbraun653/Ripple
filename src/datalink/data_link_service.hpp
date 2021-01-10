/********************************************************************************
 *  File Name:
 *    data_link_service.hpp
 *
 *  Description:
 *    Ripple data link layer service interface
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_DATA_LINK_THREAD_HPP
#define RIPPLE_DATA_LINK_THREAD_HPP

/* Chimera Includes */
#include <Chimera/thread>

/* ETL Includes */
#include <etl/delegate.h>
#include <etl/delegate_service.h>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_memory_config.hpp>
#include <Ripple/src/datalink/data_link_types.hpp>
#include <Ripple/src/datalink/data_link_arp.hpp>
#include <Ripple/src/physical/phy_fsm_controller.hpp>
#include <Ripple/src/session/session_types.hpp>


namespace Ripple::DataLink
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t THREAD_STACK          = STACK_BYTES( 1024 );
  static constexpr std::string_view THREAD_NAME = "DataLink";


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Thread class that performs the DataLink layer services, whose primary
   *  responsibilities are to:
   *
   *    1. Control data TX/RX on the physical medium
   *    2. Data queuing and scheduling
   *    3. Translate logical network addresses in to physical addresses
   *
   *  A Frame is the primary data type this layer understands how to process.
   */
  class Service : Chimera::Threading::LockableCRTP<Service>
  {
  public:
    Service();
    ~Service();

    /**
     *  Main thread that executes the DataLink layer services. This method should be
     *  bound to a delegate that can be executed by the Chimera::Threading module. If
     *  this thread is not created, the Service will not execute.
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
    Chimera::Status_t addARPEntry( const Network::IPAddress ip, const Physical::MACAddress &mac );

    /**
     *  Removes an entry from the layer's ARP table
     *
     *  @param[in]  ip          IP address of entry to remove
     *  @return Chimera::Status_t
     */
    Chimera::Status_t dropARPEntry( const Network::IPAddress ip );

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

    /**
     *  Assigns the MAC address associated with this device. This address will
     *  be tied to the EP_DEVICE_ROOT endpoint and serve as the base for all other
     *  endpoint addresses.
     *
     *  @note See RM 7.6 for more info on addressing.
     *
     *  @param[in]  mac         The MAC address to assign to the EP_DEVICE_ROOT endpoint
     *  @return Chimera::Status_t
     */
    Chimera::Status_t setRootEndpointMAC( const Physical::MACAddress &mac );

    /**
     *  Assigns the address modification byte to the pipe associated with the
     *  given endpoint. Uses the EP_DEVICE_ROOT endpoint address as a base.
     *
     *  @note See RM 7.6 for more info on addressing.
     *
     *  @param[in]  endpoint    Which endpoint to modify
     *  @param[in]  address     Address to give the endpoint
     *  @return Chimera::Status_t
     */
    Chimera::Status_t setEndpointAddress( const Endpoint endpoint, const uint8_t address );

    /**
     *  Gets the currently configured MAC address for the given endpoint
     *
     *  @param[in]  endpoint    Which endpoint to read
     *  @return Physical::MACAddress
     */
    Physical::MACAddress getEndpointMAC( const Endpoint endpoint );

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
     *  @return void
     */
    void processTXSuccess();

    /**
     *  Handle the IRQ event when a transmission failed
     *  @return void
     */
    void processTXFail();

    /**
     *  Periodic process to transmit a frame enqueued with the service
     *  @return void
     */
    void processTXQueue();

    /**
     *  Periodic process to read a frame from the radio and queue it
     *  @return void
     */
    void processRXQueue();

  private:
    friend Chimera::Threading::LockableCRTP<Service>;
    Chimera::Threading::RecursiveTimedMutex mClsMutex;

    /*-------------------------------------------------
    Class State Data
    -------------------------------------------------*/
    bool mSystemEnabled;                    /**< Gating signal for the ISR handler to prevent spurrious interrupts */
    volatile bool pendingEvent;             /**< Signal flag set by an ISR that an event occurred */
    Chimera::Threading::ThreadId mThreadId; /**< Thread registration ID */
    Session::Context mContext;              /**< User context for the network stack */
    TransferControlBlock mTCB;              /**< TX control block */

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
    Lookup table for known device IP->MAC mappings
    -------------------------------------------------*/
    ARPCache mAddressCache;

    /*-------------------------------------------------
    Finite State Machine Controller
    -------------------------------------------------*/
    Physical::FSM::RadioControl mFSMControl;
    Physical::FSM::PoweredOff _fsmState_PoweredOff;
    Physical::FSM::Standby1 _fsmState_Standby1;
    Physical::FSM::RXMode _fsmState_RXMode;
    Physical::FSM::TXMode _fsmState_TXMode;
    etl::ifsm_state *_fsmStateList[ Physical::FSM::StateId::NUMBER_OF_STATES ];
  };
}    // namespace Ripple::DataLink

#endif /* !RIPPLE_DATA_LINK_THREAD_HPP */
