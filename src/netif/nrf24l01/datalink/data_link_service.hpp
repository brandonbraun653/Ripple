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
#include <Ripple/src/netif/device_intf.hpp>
#include <Ripple/src/netif/nrf24l01/cmn_memory_config.hpp>
#include <Ripple/src/netif/nrf24l01/datalink/data_link_types.hpp>
#include <Ripple/src/netif/nrf24l01/physical/phy_fsm_controller.hpp>


namespace Ripple::NetIf::NRF24::DataLink
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
  class DataLink : public Chimera::Thread::Lockable<DataLink>, public INetIf, public IARP
  {
  public:
    DataLink();
    ~DataLink();

    /*-------------------------------------------------------------------------------
    Net Interface
    -------------------------------------------------------------------------------*/
    bool powerUp( Context_rPtr context ) final override;
    void powerDn() final override;
    Chimera::Status_t recv( Message& msg ) final override;
    Chimera::Status_t send( Message& msg, const IPAddress &ip ) final override;
    IARP * addressResolver() final override;
    size_t maxTransferSize() const final override;
    size_t linkSpeed() const final override;
    size_t lastActive() const  final override;

    /*-------------------------------------------------------------------------------
    ARP Interface
    -------------------------------------------------------------------------------*/
    Chimera::Status_t addARPEntry( const IPAddress &ip, const void *const mac, const size_t size ) final override;
    Chimera::Status_t dropARPEntry( const IPAddress &ip ) final override;
    void * arpLookUp( const IPAddress &ip, size_t &size ) final override;
    IPAddress arpLookUp( const void *const mac, const size_t size ) final override;

    /*-------------------------------------------------------------------------------
    NRF24 Specific Functions
    -------------------------------------------------------------------------------*/
    /**
     *  Main thread that executes the DataLink layer services. This method should be
     *  bound to a delegate that can be executed by the Chimera::Thread module. If
     *  this thread is not created, the DataLink will not execute.
     *
     *  @param[in]  context     Unused
     *  @return void
     */
    void run( void *context );

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


    void assignConfig( Physical::Handle &handle );

  protected:
    /**
     *  Initializes the radio with the user configured settings
     *
     *  @param[in]  session     User session information
     *  @return Chimera::Status_t
     */
    Chimera::Status_t powerUpRadio( Physical::Handle &handle );

    Chimera::Status_t initPeripherals( Physical::Handle &handle );

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
    friend Chimera::Thread::Lockable<DataLink>;


    /*-------------------------------------------------
    Class State Data
    -------------------------------------------------*/
    bool mSystemEnabled;                    /**< Gating signal for the ISR handler to prevent spurrious interrupts */
    volatile bool pendingEvent;             /**< Signal flag set by an ISR that an event occurred */
    Chimera::Thread::TaskId mTaskId;        /**< Thread registration ID */

    TransferControlBlock mTCB;              /**< TX control block */

    /*-------------------------------------------------
    Helper for tracking/invoking callbacks
    -------------------------------------------------*/
    etl::delegate_service<CallbackId::CB_NUM_OPTIONS> mDLCallbacks;

    /*-------------------------------------------------
    TX/RX Queues
    -------------------------------------------------*/
    FrameQueue<TX_QUEUE_ELEMENTS> mTXQueue;           /**< Queue for data destined for the physical layer */
    Chimera::Thread::RecursiveTimedMutex mTXMutex; /**< Thread safety lock */
    FrameQueue<RX_QUEUE_ELEMENTS> mRXQueue;           /**< Queue for data coming from the physical layer */
    Chimera::Thread::RecursiveTimedMutex mRXMutex; /**< Thread safety lock */

    /*-------------------------------------------------
    Lookup table for known device IP->MAC mappings
    -------------------------------------------------*/
    ARPCache mAddressCache;

    Context_rPtr mContext;
    Physical::Handle mPhyHandle;

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

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Creates a handle to a new NRF24 device
   *
   *  @param[in]  context       The current network context
   *  @return Handle *
   */
  DataLink *createNetIf( Context_rPtr context );

}    // namespace Ripple::DataLink

#endif /* !RIPPLE_DATA_LINK_THREAD_HPP */