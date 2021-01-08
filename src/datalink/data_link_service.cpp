/********************************************************************************
 *  File Name:
 *    data_link_service.cpp
 *
 *  Description:
 *    Implements the Ripple data link layer service
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/assert>
#include <Chimera/common>
#include <Chimera/system>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/session>
#include <Ripple/datalink>
#include <Ripple/physical>
#include <Ripple/src/physical/phy_device_internal.hpp>


namespace Ripple::DataLink
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr Physical::PipeNumber PIPE_DEVICE_ROOT  = Physical::PIPE_NUM_1;
  static constexpr Physical::PipeNumber PIPE_NET_SERVICES = Physical::PIPE_NUM_2;
  static constexpr Physical::PipeNumber PIPE_DATA_FWD     = Physical::PIPE_NUM_3;
  static constexpr Physical::PipeNumber PIPE_APP_DATA_0   = Physical::PIPE_NUM_4;
  static constexpr Physical::PipeNumber PIPE_APP_DATA_1   = Physical::PIPE_NUM_5;

  static const Physical::PipeNumber sEndpointPipes[] = { PIPE_DEVICE_ROOT, PIPE_NET_SERVICES, PIPE_DATA_FWD, PIPE_APP_DATA_0,
                                                    PIPE_APP_DATA_1 };
  static_assert( ARRAY_COUNT( sEndpointPipes ) == EP_NUM_OPTIONS );
  static_assert( PIPE_DEVICE_ROOT == Physical::PIPE_NUM_1 );

  /*-------------------------------------------------------------------------------
  Service Class Implementation
  -------------------------------------------------------------------------------*/
  Service::Service() : mSystemEnabled( false ), pendingEvent( false )
  {
  }


  Service::~Service()
  {
  }


  /*-------------------------------------------------------------------------------
  Service: Public Methods
  -------------------------------------------------------------------------------*/
  void Service::run( Session::Context context )
  {
    using namespace Chimera::Threading;

    /*-------------------------------------------------
    Wait for this thread to be told to initialize
    -------------------------------------------------*/
    this_thread::pendTaskMsg( TSK_MSG_WAKEUP );
    mThreadId = this_thread::id();

    /*-------------------------------------------------
    Verify the handles used in the entire DataLink
    service have been registered correctly.
    -------------------------------------------------*/
    Physical::Handle *physical      = Physical::getHandle( context );
    DataLink::Handle *DataLink = DataLink::getHandle( context );
    Session::Handle *session   = Session::getHandle( context );

    if ( !physical || !DataLink || !session )
    {
      Chimera::System::softwareReset();
    }
    else
    {
      mContext = context;
    }

    /*-------------------------------------------------
    Establish communication with the radio and set up
    user configuration properties.
    -------------------------------------------------*/
    if ( this->initialize( mContext ) == Chimera::Status::OK )
    {
      mFSMControl.receive( Physical::FSM::MsgPowerUp() );
      mSystemEnabled = true;
    }
    else
    {
      Chimera::System::softwareReset();
    }

    /*-------------------------------------------------
    Execute the service
    -------------------------------------------------*/
    while ( 1 )
    {
      /*-------------------------------------------------
      Process the core radio events. This is driven by a
      GPIO interrupt tied to the IRQ pin, or by another
      task informing that it's time to process data.
      -------------------------------------------------*/
      if ( pendingEvent || this_thread::pendTaskMsg( TSK_MSG_WAKEUP, DataLink->hwIRQEventTimeout ) )
      {
        pendingEvent      = false;
        uint8_t eventMask = Physical::getISREvent( *physical );

        /*-------------------------------------------------
        The last packet failed to transmit correctly
        -------------------------------------------------*/
        if ( eventMask & Physical::bfISRMask::ISR_MSK_MAX_RT )
        {
          processTXFail();
        }

        /*-------------------------------------------------
        A new packet was received
        -------------------------------------------------*/
        if ( eventMask & Physical::bfISRMask::ISR_MSK_RX_DR )
        {
          processRXQueue();
        }

        /*-------------------------------------------------
        A packet successfully transmitted
        -------------------------------------------------*/
        if ( eventMask & Physical::bfISRMask::ISR_MSK_TX_DS )
        {
          processTXSuccess();
        }
      }

      /*-------------------------------------------------
      Handle packet TX timeouts. Getting here means there
      is likely a setup issue with the hardware.
      -------------------------------------------------*/
      if ( mTCB.inProgress && ( ( Chimera::millis() - mTCB.start ) > mTCB.timeout ) )
      {
        processTXFail();
      }

      /*-------------------------------------------------
      Another thread may have woken this one to process
      new frame queue data. Check if any is available.
      Handle RX first to keep the HW FIFOs empty.
      -------------------------------------------------*/
      processRXQueue();
      processTXQueue();
    }
  }


  Chimera::Status_t Service::enqueueFrame( const Frame &frame )
  {
    mTXMutex.lock();

    /*-------------------------------------------------
    Process the error handler if the queue is full
    -------------------------------------------------*/
    if ( mTXQueue.full() )
    {
      mTXMutex.unlock();

      DataLink::getHandle( mContext )->txQueueOverflows++;
      mDelegateRegistry.call<CallbackId::CB_ERROR_TX_QUEUE_FULL>();
      return Chimera::Status::FULL;
    }

    /*-------------------------------------------------
    Otherwise enqueue
    -------------------------------------------------*/
    mTXQueue.push( frame );
    mTXMutex.unlock();
    return Chimera::Status::OK;
  }


  Chimera::Status_t Service::dequeueFrame( Frame &frame )
  {
    /*-------------------------------------------------
    Check if queue is empty
    -------------------------------------------------*/
    mRXMutex.lock();
    if ( mRXQueue.empty() )
    {
      mRXMutex.unlock();
      return Chimera::Status::EMPTY;
    }

    /*-------------------------------------------------
    Otherwise dequeue
    -------------------------------------------------*/
    mRXQueue.pop_into( frame );
    mRXMutex.unlock();
    return Chimera::Status::OK;
  }


  Chimera::Status_t Service::addARPEntry( const Network::IPAddress ip, const Physical::MACAddress &mac )
  {
    /*-------------------------------------------------
    Attempt to insert the new entry
    -------------------------------------------------*/
    this->lock();
    bool success = mAddressCache.insert( ip, mac );
    this->unlock();

    /*-------------------------------------------------
    Invoke the error callback on failure
    -------------------------------------------------*/
    if ( !success )
    {
      mDelegateRegistry.call<CallbackId::CB_ERROR_ARP_LIMIT>();
    }

    return success ? Chimera::Status::OK : Chimera::Status::FAIL;
  }


  Chimera::Status_t Service::dropARPEntry( const Network::IPAddress ip )
  {
    /*-------------------------------------------------
    Does nothing if the entry doesn't exist
    -------------------------------------------------*/
    this->lock();
    mAddressCache.remove( ip );
    this->unlock();

    return Chimera::Status::OK;
  }


  Chimera::Status_t Service::registerCallback( const CallbackId id, etl::delegate<void( size_t )> func )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !( id < CallbackId::CB_NUM_OPTIONS ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Register the callback
    -------------------------------------------------*/
    this->lock();
    if ( id == CallbackId::CB_UNHANDLED )
    {
      mDelegateRegistry.register_unhandled_delegate( func );
    }
    else
    {
      mDelegateRegistry.register_delegate( id, func );
    }
    this->unlock();
    return Chimera::Status::OK;
  }


  bool Service::queryCallbackData( const CallbackId id, void *const data )
  {
    // TODO: Once the network layer needs this
    return false;
  }


  Chimera::Status_t Service::setRootEndpointMAC( const Physical::MACAddress &mac )
  {
    Physical::Handle *phyHandle = Physical::getHandle( mContext );

    auto result = Physical::openReadPipe( *phyHandle, sEndpointPipes[ EP_DEVICE_ROOT ], mac );
    if ( result == Chimera::Status::OK )
    {
      Session::getHandle( mContext )->radioConfig.advanced.mac = mac;
    }

    return result;
  }


  Chimera::Status_t Service::setEndpointAddress( const Endpoint endpoint, const uint8_t address )
  {
    Physical::Handle *phyHandle = Physical::getHandle( mContext );

    /*-------------------------------------------------
    Can't set the root address directly in this func
    -------------------------------------------------*/
    if ( ( endpoint >= EP_NUM_OPTIONS ) || ( endpoint == EP_DEVICE_ROOT ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Build the endpoint address based on the currently
    assigned device root address.
    -------------------------------------------------*/
    Physical::MACAddress mac = Session::getHandle( mContext )->radioConfig.advanced.mac;
    mac &= ~0xFF;
    mac |= address;

    /*-------------------------------------------------
    Enable the RX endpoint address
    -------------------------------------------------*/
    return Physical::openReadPipe( *phyHandle, sEndpointPipes[ endpoint ], mac );
  }


  Physical::MACAddress Service::getEndpointMAC( const Endpoint endpoint )
  {
    if ( endpoint >= Endpoint::EP_NUM_OPTIONS )
    {
      return 0;
    }

    Physical::Handle *phyHandle = Physical::getHandle( mContext );
    return Physical::getRXPipeAddress( *phyHandle, sEndpointPipes[ endpoint ] );
  }


  /*-------------------------------------------------------------------------------
  Service: Protected Methods
  -------------------------------------------------------------------------------*/
  Chimera::Status_t Service::initialize( Session::Context context )
  {
    /*-------------------------------------------------
    Input protections
    -------------------------------------------------*/
    if ( !context )
    {
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Clear all memory
    -------------------------------------------------*/
    mTXQueue.clear();
    mRXQueue.clear();

    /*-------------------------------------------------
    Power up the radio
    -------------------------------------------------*/
    return this->powerUpRadio( context );
  }


  Chimera::Status_t Service::powerUpRadio( Session::Context context )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    auto result   = Chimera::Status::OK;
    auto session  = Session::getHandle( context );
    auto physical = Physical::getHandle( context );
    auto DataLink = getHandle( context );

    if ( !physical )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    GPIO interrupt configuration
    -------------------------------------------------*/
    auto cb = Chimera::Function::vGeneric::create<Service, &Service::irqPinAsserted>( *this );
    result |= physical->irqPin->attachInterrupt( cb, physical->cfg.irqEdge );

    /*-------------------------------------------------
    Reset the device to power on conditions
    -------------------------------------------------*/
    result |= Physical::openDevice( physical->cfg, *physical );
    result |= Physical::resetRegisterDefaults( *physical );

    /*-------------------------------------------------
    Apply basic global user settings
    -------------------------------------------------*/
    result |= Physical::setCRCLength( *physical, physical->cfg.hwCRCLength );
    result |= Physical::setAddressWidth( *physical, physical->cfg.hwAddressWidth );
    result |= Physical::setISRMasks( *physical, physical->cfg.hwISRMask );
    result |= Physical::setRFChannel( *physical, physical->cfg.hwRFChannel );
    result |= Physical::setRFPower( *physical, physical->cfg.hwPowerAmplitude );
    result |= Physical::setDataRate( *physical, physical->cfg.hwDataRate );

    /* Allow the network driver to decide at runtime if a packet requires an ACK response */
    result |= Physical::toggleDynamicAck( *physical, true );
    result |= Physical::toggleAutoAck( *physical, true, Physical::PIPE_NUM_ALL );

    /* Static/Dynamic Payloads */
    if ( session->radioConfig.advanced.staticPayloads )
    {
      result |= Physical::toggleDynamicPayloads( *physical, Physical::PIPE_NUM_ALL, false );
      result |= Physical::setStaticPayloadSize( *physical, session->radioConfig.advanced.staticPayloadSize, Physical::PIPE_NUM_ALL );
    }
    else
    {
      result |= Physical::toggleDynamicPayloads( *physical, Physical::PIPE_NUM_ALL, true );
    }

    /*-------------------------------------------------
    Flush hardware FIFOs to clear pre-existing data
    -------------------------------------------------*/
    result |= Physical::flushRX( *physical );
    result |= Physical::flushTX( *physical );

    /*-------------------------------------------------
    Initialize the FSM controller
    -------------------------------------------------*/
    _fsmStateList[ Physical::FSM::StateId::POWERED_OFF ] = &_fsmState_PoweredOff;
    _fsmStateList[ Physical::FSM::StateId::STANDBY_1 ]   = &_fsmState_Standby1;
    _fsmStateList[ Physical::FSM::StateId::RX_MODE ]     = &_fsmState_RXMode;
    _fsmStateList[ Physical::FSM::StateId::TX_MODE ]     = &_fsmState_TXMode;

    mFSMControl.mHandle = physical;
    mFSMControl.set_states( _fsmStateList, etl::size( _fsmStateList ) );
    mFSMControl.start();

    return result;
  }


  void Service::irqPinAsserted( void *arg )
  {
    using namespace Chimera::Threading;

    /*-------------------------------------------------
    Let the user space thread know it has an event to
    process. Unsure what kind at this point.
    -------------------------------------------------*/
    if ( mSystemEnabled )
    {
      pendingEvent = true;
      sendTaskMsg( mThreadId, TSK_MSG_WAKEUP, TIMEOUT_DONT_WAIT );
    }
  }


  void Service::processTXSuccess()
  {
    Physical::Handle *phyHandle = Physical::getHandle( mContext );

    /*-------------------------------------------------
    Acknowledge the success
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );
    Physical::clrISREvent( *phyHandle, Physical::bfISRMask::ISR_MSK_TX_DS );
    mTCB.inProgress = false;

    /*-------------------------------------------------
    Remove the now TX'd data off the queue
    -------------------------------------------------*/
    mTXMutex.lock();
    mTXQueue.pop();
    mTXMutex.unlock();

    /*-------------------------------------------------
    Notify the network layer of the success
    -------------------------------------------------*/
    mDelegateRegistry.call<CallbackId::CB_TX_SUCCESS>();
  }


  void Service::processTXFail()
  {
    Physical::Handle *phyHandle = Physical::getHandle( mContext );

    /*-------------------------------------------------
    Dump the unsuccessful frame from the queue
    -------------------------------------------------*/
    Frame failedFrame;

    mTXMutex.lock();
    mTXQueue.pop_into( failedFrame );
    mTXMutex.unlock();

    /*-------------------------------------------------
    Transition back to an idle state
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );
    mTCB.inProgress = false;

    /*-------------------------------------------------------------------------------
    One reason why a TX fail event must be processed is due to a max retry IRQ. In
    this case, the data is not removed from the TX FIFO, so it must be done manually.
    (RM 8.4) First transition back to Standby-1 mode, then clear event flags and flush
    the TX FIFO. Otherwise, the IRQ will continuously fire.
    -------------------------------------------------------------------------------*/
    if ( failedFrame.control & bfControlFlags::CTRL_PAYLOAD_ACK )
    {
      Physical::flushTX( *phyHandle );
      Physical::clrISREvent( *phyHandle, Physical::bfISRMask::ISR_MSK_MAX_RT );
    }
    // else NO_ACK, which means there is nothing to clear. The data was lost to the ether.

    /*-------------------------------------------------
    Notify the network layer of the failed frame
    -------------------------------------------------*/
    mDelegateRegistry.call<CallbackId::CB_ERROR_TX_FAILURE>();
  }


  void Service::processTXQueue()
  {
    using namespace Chimera::Threading;
    Physical::Handle *phy = nullptr;

    /*-------------------------------------------------
    Cannot process another frame until the last one
    either successfully transmitted, or errored out.
    -------------------------------------------------*/
    if ( mTCB.inProgress || !mTXMutex.try_lock_for( TIMEOUT_1MS ) )
    {
      return;
    }
    else if ( mTXQueue.empty() )
    {
      /*-------------------------------------------------
      Nothing to TX, so ensure hardware is listening
      -------------------------------------------------*/
      mTXMutex.unlock();
      mFSMControl.receive( Physical::FSM::MsgStartRX() );
      return;
    }

    /*-------------------------------------------------
    Look up the hardware address associated with the
    destination node.
    -------------------------------------------------*/
    const Frame &cacheFrame    = mTXQueue.front();
    Physical::MACAddress dstAddress = 0;

    if ( !mAddressCache.lookup( cacheFrame.nextHop, &dstAddress ) )
    {
      mDelegateRegistry.call<CallbackId::CB_ERROR_ARP_RESOLVE>();
      mTXQueue.pop();
      mTXMutex.unlock();
      mTCB.inProgress = false;

      return;
    }

    /*-------------------------------------------------
    All information needed to TX the frame is known, so
    it's safe to transition to standby mode in prep for
    moving to TX mode once the data is loaded.
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );

    /*-------------------------------------------------
    Open the proper port for writing
    -------------------------------------------------*/
    phy = Physical::getHandle( mContext );
    Physical::openWritePipe( *phy, dstAddress );

    /*-------------------------------------------------
    Determine the reliability required on the TX. This
    assumes hardware is initialized with DynamicACK,
    the proper payload length settings are set, and any
    pipes needing auto-ack are enabled.
    -------------------------------------------------*/
    Physical::PayloadType txType = Physical::PayloadType::PAYLOAD_NO_ACK;

    if ( cacheFrame.control & bfControlFlags::CTRL_PAYLOAD_ACK )
    {
      txType = Physical::PayloadType::PAYLOAD_REQUIRES_ACK;
      Physical::setRetries( *phy, cacheFrame.rtxDelay, cacheFrame.rtxCount );
    }

    /*-------------------------------------------------
    Write the data to the TX FIFO and transition to the
    active TX mode.
    -------------------------------------------------*/
    mTCB.inProgress = true;
    mTCB.timeout    = 10;
    mTCB.start      = Chimera::millis();

    Physical::writePayload( *phy, cacheFrame.payload, cacheFrame.length, txType );
    mFSMControl.receive( Physical::FSM::MsgStartTX() );

    mTXMutex.unlock();

    return;
  }


  void Service::processRXQueue()
  {
    /*-------------------------------------------------
    Ensure it is safe to process the RX Queue/FIFO
    -------------------------------------------------*/
    if ( mTCB.inProgress )
    {
      /*-------------------------------------------------
      TX-ing and RX-ing are exclusive. Play nice.
      -------------------------------------------------*/
      return;
    }
    else if ( !mRXMutex.try_lock_for( Chimera::Threading::TIMEOUT_1MS ) )
    {
      /*-------------------------------------------------
      Who is holding the RX lock for more than 1 ms? This
      is likely a thread lock condition.
      -------------------------------------------------*/
      Chimera::insert_debug_breakpoint();
      return;
    }
    else if ( mRXQueue.full() )
    {
      /*-------------------------------------------------
      Give the network layer an opportunity to pull data
      -------------------------------------------------*/
      mDelegateRegistry.call<CallbackId::CB_ERROR_RX_QUEUE_FULL>();
    }

    /*-------------------------------------------------
    Acquire the contexts used for the RX procedure
    -------------------------------------------------*/
    Physical::Handle *physical      = Physical::getHandle( mContext );
    DataLink::Handle *DataLink = DataLink::getHandle( mContext );
    Session::Handle *session   = Session::getHandle( mContext );

    /*-------------------------------------------------
    Transition to Standby-1 mode, else the data cannot
    be read from the RX FIFO (RM Appendix A)
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );

    /*-------------------------------------------------
    Acknowledge the RX event
    -------------------------------------------------*/
    Physical::clrISREvent( *physical, Physical::bfISRMask::ISR_MSK_RX_DR );

    /*-------------------------------------------------
    Read out all available data, regardless of whether
    or not the queue can store the information. Without
    this, the network will stall.
    -------------------------------------------------*/
    Physical::PipeNumber pipe = Physical::getAvailablePayloadPipe( *physical );
    while ( pipe != Physical::PipeNumber::PIPE_INVALID )
    {
      /*-------------------------------------------------
      Prepare a clean frame to copy data into. The more
      complex aspects of the payload will be decoded in
      the network layer.
      -------------------------------------------------*/
      Frame tempFrame;
      tempFrame.clear();
      tempFrame.rxPipe = pipe;

      /*-------------------------------------------------
      Read out the data associated with the frame
      -------------------------------------------------*/
      if ( session->radioConfig.advanced.staticPayloads )
      {
        Physical::readPayload( *physical, tempFrame.payload, physical->cfg.hwStaticPayloadWidth );
      }
      else    // Dynamic payloads
      {
        /*-------------------------------------------------
        Make noise if this is configured. Currently not
        supported but may be in the future.
        -------------------------------------------------*/
        RT_HARD_ASSERT( false );
        size_t bytes = Physical::getAvailablePayloadSize( *physical, pipe );
        Physical::readPayload( *physical, tempFrame.payload, bytes );
      }

      /*-------------------------------------------------
      Enqueue the frame if possible, call the network
      handler if not. Otherwise the data is simply lost.
      -------------------------------------------------*/
      if ( !mRXQueue.full() )
      {
        mRXQueue.push( tempFrame );
      }
      else
      {
        DataLink->rxQueueOverflows++;
        mDelegateRegistry.call<CallbackId::CB_ERROR_RX_QUEUE_FULL>();

        if ( !mRXQueue.full() )
        {
          mRXQueue.push( tempFrame );
        }
        // else data is lost
      }

      /*-------------------------------------------------
      Check for more data to read from the RX FIFO
      -------------------------------------------------*/
      pipe = Physical::getAvailablePayloadPipe( *physical );
    }

    /*-------------------------------------------------
    Go back to listening. The TX process is mutually
    exclusive, so it's ok to transition to this state.
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgStartRX() );

    /*-------------------------------------------------
    Let the network layer know data is ready
    -------------------------------------------------*/
    mRXMutex.unlock();
    mDelegateRegistry.call<CallbackId::CB_RX_PAYLOAD>();
  }

}    // namespace Ripple::DataLink
