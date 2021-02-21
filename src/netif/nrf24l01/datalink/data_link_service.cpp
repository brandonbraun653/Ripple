/********************************************************************************
 *  File Name:
 *    data_link_service.cpp
 *
 *  Description:
 *    Implements the Ripple data link layer service
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Aurora Includes */
#include <Aurora/logging>

/* Chimera Includes */
#include <Chimera/assert>
#include <Chimera/common>
#include <Chimera/system>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>
#include <Ripple/shared>


namespace Ripple::NetIf::NRF24::DataLink
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
  Public Functions
  -------------------------------------------------------------------------------*/
  DataLink *createNetIf( Context_rPtr context )
  {
    /*-------------------------------------------------
    Use placement new to allocate a handle on the heap
    -------------------------------------------------*/
    RT_HARD_ASSERT( context );
    return new( context->malloc( sizeof( DataLink ) ) ) DataLink();
  }


  /*-------------------------------------------------------------------------------
  Service Class Implementation
  -------------------------------------------------------------------------------*/
  DataLink::DataLink() : mSystemEnabled( false ), pendingEvent( false )
  {
  }


  DataLink::~DataLink()
  {
  }


  /*-------------------------------------------------------------------------------
  Service: Net Interface
  -------------------------------------------------------------------------------*/
  bool DataLink::powerUp( Context_rPtr context )
  {
    using namespace Aurora::Logging;
    using namespace Chimera::Thread;

    mContext = context;

    /*-------------------------------------------------
    First turn on the hardware drivers
    -------------------------------------------------*/
    getRootSink()->flog( Level::LVL_DEBUG, "Initializing NRF24...\r\n" );
    if( powerUpRadio( mPhyHandle ) != Chimera::Status::OK )
    {
      getRootSink()->flog( Level::LVL_DEBUG, "Failed initializing NRF24\r\n" );
      return false;
    }

    /*-------------------------------------------------
    Start the DataLink Service
    -------------------------------------------------*/
    TaskDelegate dlFunc = TaskDelegate::create<DataLink, &DataLink::run>( *this );

    Task datalink;
    datalink.initialize( dlFunc, nullptr, Priority::LEVEL_4, THREAD_STACK, THREAD_NAME.cbegin() );
    mTaskId = datalink.start();
    sendTaskMsg( mTaskId, ITCMsg::TSK_MSG_WAKEUP, TIMEOUT_DONT_WAIT );

    /* Give the hardware time to boot */
    Chimera::delayMilliseconds( 100 );
    return true;
  }


  void DataLink::powerDn()
  {

  }


  Chimera::Status_t DataLink::recv( Message& msg )
  {
    // /*-------------------------------------------------
    // Check if queue is empty
    // -------------------------------------------------*/
    // mRXMutex.lock();
    // if ( mRXQueue.empty() )
    // {
    //   mRXMutex.unlock();
    //   return Chimera::Status::EMPTY;
    // }

    // /*-------------------------------------------------
    // Otherwise dequeue
    // -------------------------------------------------*/
    // mRXQueue.pop_into( frame );
    // mRXMutex.unlock();
    return Chimera::Status::OK;
  }


  Chimera::Status_t DataLink::send( Message& msg, const IPAddress &ip )
  {
    // mTXMutex.lock();

    // /*-------------------------------------------------
    // Process the error handler if the queue is full
    // -------------------------------------------------*/
    // if ( mTXQueue.full() )
    // {
    //   mTXMutex.unlock();

    //   DataLink::getHandle( mContext )->txQueueOverflows++;
    //   mDLCallbacks.call<CallbackId::CB_ERROR_TX_QUEUE_FULL>();
    //   return Chimera::Status::FULL;
    // }

    // /*-------------------------------------------------
    // Otherwise enqueue
    // -------------------------------------------------*/
    // mTXQueue.push( frame );
    // mTXMutex.unlock();
    return Chimera::Status::OK;

  }


  IARP * DataLink::addressResolver()
  {
    return this;
  }


  size_t DataLink::maxTransferSize() const
  {
    return Physical::MAX_SPI_DATA_LEN;
  }


  size_t DataLink::linkSpeed() const
  {
    return 1000;
  }


  size_t DataLink::lastActive() const
  {
    // TODO: Need to actually fill this out
    return Chimera::millis();
  }


  /*-------------------------------------------------------------------------------
  Service: ARP Interface
  -------------------------------------------------------------------------------*/
  Chimera::Status_t DataLink::addARPEntry( const IPAddress &ip, const void *const mac, const size_t size )
  {
    if( size != sizeof( Physical::MACAddress ) )
    {
      return Chimera::Status::FAIL;
    }

    Physical::MACAddress addr = 0;
    memcpy( &addr, mac, sizeof( Physical::MACAddress ) );

    /*-------------------------------------------------
    Attempt to insert the new entry
    -------------------------------------------------*/
    this->lock();

    bool success = mAddressCache.insert( ip, addr );
    this->unlock();

    /*-------------------------------------------------
    Invoke the error callback on failure
    -------------------------------------------------*/
    if ( !success )
    {
      mDLCallbacks.call<CallbackId::CB_ERROR_ARP_LIMIT>();
    }

    return success ? Chimera::Status::OK : Chimera::Status::FAIL;

  }


  Chimera::Status_t DataLink::dropARPEntry( const IPAddress &ip )
  {
    /*-------------------------------------------------
    Does nothing if the entry doesn't exist
    -------------------------------------------------*/
    this->lock();
    mAddressCache.remove( ip );
    this->unlock();

    return Chimera::Status::OK;
  }


  void * DataLink::arpLookUp( const IPAddress &ip, size_t &size )
  {
    size = 0;
    return nullptr;
  }


  IPAddress DataLink::arpLookUp( const void *const mac, const size_t size )
  {
    return 0;
  }


  /*-------------------------------------------------------------------------------
  Service: Public Methods
  -------------------------------------------------------------------------------*/
  void DataLink::run( void *context )
  {
    using namespace Aurora::Logging;
    using namespace Chimera::Thread;

    /*-------------------------------------------------
    Wait for this thread to be told to initialize
    -------------------------------------------------*/
    Ripple::TaskWaitInit();
    this_thread::set_name( "DataLink_Service" );
    mTaskId = this_thread::id();

    getRootSink()->flog( Level::LVL_DEBUG, "Starting NRF24 network services\r\n" );

    /*-------------------------------------------------
    Establish communication with the radio and set up
    user configuration properties.
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgPowerUp() );
    mSystemEnabled = true;

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
      if ( pendingEvent || this_thread::pendTaskMsg( TSK_MSG_WAKEUP, 25 ) )
      {
        pendingEvent      = false;
        uint8_t eventMask = Physical::getISREvent( mPhyHandle );

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


  Chimera::Status_t DataLink::setRootEndpointMAC( const Physical::MACAddress &mac )
  {
    auto result = Physical::openReadPipe( mPhyHandle, sEndpointPipes[ EP_DEVICE_ROOT ], mac );
    if ( result == Chimera::Status::OK )
    {
      mPhyHandle.cfg.hwAddress = mac;
    }

    return result;
  }


  Chimera::Status_t DataLink::setEndpointAddress( const Endpoint endpoint, const uint8_t address )
  {
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
    Physical::MACAddress mac = mPhyHandle.cfg.hwAddress;
    mac &= ~0xFF;
    mac |= address;

    /*-------------------------------------------------
    Enable the RX endpoint address
    -------------------------------------------------*/
    return Physical::openReadPipe( mPhyHandle, sEndpointPipes[ endpoint ], mac );
  }


  Physical::MACAddress DataLink::getEndpointMAC( const Endpoint endpoint )
  {
    if ( endpoint >= Endpoint::EP_NUM_OPTIONS )
    {
      return 0;
    }

    return Physical::getRXPipeAddress( mPhyHandle, sEndpointPipes[ endpoint ] );
  }


  void DataLink::assignConfig( Physical::Handle &handle )
  {
    mPhyHandle = handle;
  }


  /*-------------------------------------------------------------------------------
  Service: Protected Methods
  -------------------------------------------------------------------------------*/
  Chimera::Status_t DataLink::powerUpRadio( Physical::Handle &handle )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    auto result = initPeripherals( handle );
    if( result != Chimera::Status::OK )
    {
      return result;
    }
    else
    {
      mPhyHandle = handle;
    }

    /*-------------------------------------------------
    Clear all memory
    -------------------------------------------------*/
    mTXQueue.clear();
    mRXQueue.clear();

    /*-------------------------------------------------
    GPIO interrupt configuration
    -------------------------------------------------*/
    auto cb = Chimera::Function::vGeneric::create<DataLink, &DataLink::irqPinAsserted>( *this );
    result |= mPhyHandle.irqPin->attachInterrupt( cb, mPhyHandle.cfg.irqEdge );

    /*-------------------------------------------------
    Reset the device to power on conditions
    -------------------------------------------------*/
    result |= Physical::openDevice( mPhyHandle.cfg, mPhyHandle );
    result |= Physical::resetRegisterDefaults( mPhyHandle );

    /*-------------------------------------------------
    Apply basic global user settings
    -------------------------------------------------*/
    result |= Physical::setCRCLength( mPhyHandle, mPhyHandle.cfg.hwCRCLength );
    result |= Physical::setAddressWidth( mPhyHandle, mPhyHandle.cfg.hwAddressWidth );
    result |= Physical::setISRMasks( mPhyHandle, mPhyHandle.cfg.hwISRMask );
    result |= Physical::setRFChannel( mPhyHandle, mPhyHandle.cfg.hwRFChannel );
    result |= Physical::setRFPower( mPhyHandle, mPhyHandle.cfg.hwPowerAmplitude );
    result |= Physical::setDataRate( mPhyHandle, mPhyHandle.cfg.hwDataRate );

    /* Allow the network driver to decide at runtime if a packet requires an ACK response */
    result |= Physical::toggleDynamicAck( mPhyHandle, true );
    result |= Physical::toggleAutoAck( mPhyHandle, true, Physical::PIPE_NUM_ALL );

    /* Static/Dynamic Payloads */
    if ( mPhyHandle.cfg.hwStaticPayloadWidth )
    {
      result |= Physical::toggleDynamicPayloads( mPhyHandle, Physical::PIPE_NUM_ALL, false );
      result |= Physical::setStaticPayloadSize( mPhyHandle, mPhyHandle.cfg.hwStaticPayloadWidth, Physical::PIPE_NUM_ALL );
    }
    else
    {
      result |= Physical::toggleDynamicPayloads( mPhyHandle, Physical::PIPE_NUM_ALL, true );
    }

    /*-------------------------------------------------
    Flush hardware FIFOs to clear pre-existing data
    -------------------------------------------------*/
    result |= Physical::flushRX( mPhyHandle );
    result |= Physical::flushTX( mPhyHandle );

    /*-------------------------------------------------
    Initialize the FSM controller
    -------------------------------------------------*/
    _fsmStateList[ Physical::FSM::StateId::POWERED_OFF ] = &_fsmState_PoweredOff;
    _fsmStateList[ Physical::FSM::StateId::STANDBY_1 ]   = &_fsmState_Standby1;
    _fsmStateList[ Physical::FSM::StateId::RX_MODE ]     = &_fsmState_RXMode;
    _fsmStateList[ Physical::FSM::StateId::TX_MODE ]     = &_fsmState_TXMode;

    mFSMControl.mHandle = &mPhyHandle;
    mFSMControl.set_states( _fsmStateList, etl::size( _fsmStateList ) );
    mFSMControl.start();

    return result;
  }


  Chimera::Status_t DataLink::initPeripherals( Physical::Handle &handle )
  {
    auto result = Chimera::Status::OK;

    /*-------------------------------------------------
    Discrete GPIO: Chip Enable Pin
    -------------------------------------------------*/
    handle.cePin = Chimera::GPIO::getDriver( handle.cfg.ce.port, handle.cfg.ce.pin );
    result |= handle.cePin->init( handle.cfg.ce );

    /*-------------------------------------------------
    Discrete GPIO: IRQ Pin
    -------------------------------------------------*/
    handle.irqPin = Chimera::GPIO::getDriver( handle.cfg.irq.port, handle.cfg.irq.pin );
    result |= handle.irqPin->init( handle.cfg.irq );

    /*-------------------------------------------------
    SPI: Chip Select Pin
    -------------------------------------------------*/
    handle.csPin = Chimera::GPIO::getDriver( handle.cfg.spi.CSInit.port, handle.cfg.spi.CSInit.pin );
    result |= handle.csPin->init( handle.cfg.spi.CSInit );

    /*-------------------------------------------------
    SPI: Protocol Settings
    -------------------------------------------------*/
    handle.spi = Chimera::SPI::getDriver( handle.cfg.spi.HWInit.hwChannel );
    result |= handle.spi->init( handle.cfg.spi );

    RT_HARD_ASSERT( result == Chimera::Status::OK );
    return result;
  }


  void DataLink::irqPinAsserted( void *arg )
  {
    using namespace Chimera::Thread;

    /*-------------------------------------------------
    Let the user space thread know it has an event to
    process. Unsure what kind at this point.
    -------------------------------------------------*/
    if ( mSystemEnabled )
    {
      pendingEvent = true;
      sendTaskMsg( mTaskId, TSK_MSG_WAKEUP, TIMEOUT_DONT_WAIT );
    }
  }


  void DataLink::processTXSuccess()
  {
    /*-------------------------------------------------
    Acknowledge the success
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );
    Physical::clrISREvent( mPhyHandle, Physical::bfISRMask::ISR_MSK_TX_DS );
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
    mDLCallbacks.call<CallbackId::CB_TX_SUCCESS>();
  }


  void DataLink::processTXFail()
  {
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
      Physical::flushTX( mPhyHandle );
      Physical::clrISREvent( mPhyHandle, Physical::bfISRMask::ISR_MSK_MAX_RT );
    }
    // else NO_ACK, which means there is nothing to clear. The data was lost to the ether.

    /*-------------------------------------------------
    Notify the network layer of the failed frame
    -------------------------------------------------*/
    mDLCallbacks.call<CallbackId::CB_ERROR_TX_FAILURE>();
  }


  void DataLink::processTXQueue()
  {
    using namespace Chimera::Thread;
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
    const Frame &cacheFrame         = mTXQueue.front();
    Physical::MACAddress dstAddress = 0;

    if ( !mAddressCache.lookup( cacheFrame.nextHop, &dstAddress ) )
    {
      mDLCallbacks.call<CallbackId::CB_ERROR_ARP_RESOLVE>();
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
    Physical::openWritePipe( mPhyHandle, dstAddress );

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
      Physical::setRetries( mPhyHandle, cacheFrame.rtxDelay, cacheFrame.rtxCount );
    }

    /*-------------------------------------------------
    Write the data to the TX FIFO and transition to the
    active TX mode.
    -------------------------------------------------*/
    mTCB.inProgress = true;
    mTCB.timeout    = 10;
    mTCB.start      = Chimera::millis();

    Physical::writePayload( mPhyHandle, cacheFrame.payload, cacheFrame.length, txType );
    mFSMControl.receive( Physical::FSM::MsgStartTX() );

    mTXMutex.unlock();

    return;
  }


  void DataLink::processRXQueue()
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
    else if ( !mRXMutex.try_lock_for( Chimera::Thread::TIMEOUT_1MS ) )
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
      mDLCallbacks.call<CallbackId::CB_ERROR_RX_QUEUE_FULL>();
    }

    /*-------------------------------------------------
    Transition to Standby-1 mode, else the data cannot
    be read from the RX FIFO (RM Appendix A)
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );

    /*-------------------------------------------------
    Acknowledge the RX event
    -------------------------------------------------*/
    Physical::clrISREvent( mPhyHandle, Physical::bfISRMask::ISR_MSK_RX_DR );

    /*-------------------------------------------------
    Read out all available data, regardless of whether
    or not the queue can store the information. Without
    this, the network will stall.
    -------------------------------------------------*/
    Physical::PipeNumber pipe = Physical::getAvailablePayloadPipe( mPhyHandle );
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
      if ( mPhyHandle.cfg.hwStaticPayloadWidth )
      {
        Physical::readPayload( mPhyHandle, tempFrame.payload, mPhyHandle.cfg.hwStaticPayloadWidth );
      }
      else    // Dynamic payloads
      {
        /*-------------------------------------------------
        Make noise if this is configured. Currently not
        supported but may be in the future.
        -------------------------------------------------*/
        RT_HARD_ASSERT( false );
        size_t bytes = Physical::getAvailablePayloadSize( mPhyHandle, pipe );
        Physical::readPayload( mPhyHandle, tempFrame.payload, bytes );
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
        mPhyHandle.rxQueueOverflows++;
        mDLCallbacks.call<CallbackId::CB_ERROR_RX_QUEUE_FULL>();

        if ( !mRXQueue.full() )
        {
          mRXQueue.push( tempFrame );
        }
        // else data is lost
      }

      /*-------------------------------------------------
      Check for more data to read from the RX FIFO
      -------------------------------------------------*/
      pipe = Physical::getAvailablePayloadPipe( mPhyHandle );
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
    mDLCallbacks.call<CallbackId::CB_RX_PAYLOAD>();
  }

}    // namespace Ripple::DataLink