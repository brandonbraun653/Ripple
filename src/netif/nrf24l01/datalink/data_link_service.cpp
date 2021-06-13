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
#include <Aurora/tracing>

/* Chimera Includes */
#include <Chimera/assert>
#include <Chimera/common>
#include <Chimera/system>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>
#include <Ripple/shared>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_internal.hpp>


namespace Ripple::NetIf::NRF24::DataLink
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t THREAD_STACK_BYTES    = 2048;
  static constexpr size_t THREAD_STACK_WORDS    = STACK_BYTES( THREAD_STACK_BYTES );
  static constexpr std::string_view THREAD_NAME = "DataLink";


  static constexpr Physical::PipeNumber PIPE_TX           = Physical::PIPE_NUM_0;
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
  Enumerations
  -------------------------------------------------------------------------------*/
  enum EventMarker : size_t
  {

  };


  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  DataLink *createNetIf( Context_rPtr context )
  {
    /*-------------------------------------------------
    Use placement new to allocate a handle on the heap
    -------------------------------------------------*/
    RT_HARD_ASSERT( context );
    void * ptr = context->malloc( sizeof( DataLink ) );
    return new ( ptr ) DataLink();
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
  bool DataLink::powerUp( void * context )
  {
    using namespace Aurora::Logging;
    using namespace Chimera::Thread;

    /*-------------------------------------------------
    Initialize the Datalink layer
    -------------------------------------------------*/
    mContext = reinterpret_cast<Context_rPtr>( context );
    memset( &mEndpointMAC, 0, ARRAY_BYTES( mEndpointMAC ) );

    /*-------------------------------------------------
    First turn on the hardware drivers
    -------------------------------------------------*/
    LOG_DEBUG( "Initializing NRF24...\r\n" );
    if ( powerUpRadio( mPhyHandle ) != Chimera::Status::OK )
    {
      LOG_DEBUG( "Failed initializing NRF24\r\n" );
      return false;
    }

    /*-------------------------------------------------
    Start the DataLink Service
    -------------------------------------------------*/
    TaskDelegate dlFunc = TaskDelegate::create<DataLink, &DataLink::run>( *this );

    Task datalink;
    TaskConfig cfg;

    cfg.arg                                   = nullptr;
    cfg.function                              = dlFunc;
    cfg.priority                              = Priority::LEVEL_4;
    cfg.stackWords                            = THREAD_STACK_WORDS;
    cfg.type                                  = TaskInitType::STATIC;
    cfg.name                                  = THREAD_NAME.data();
    cfg.specialization.staticTask.stackBuffer = mContext->malloc( THREAD_STACK_BYTES );
    cfg.specialization.staticTask.stackSize   = THREAD_STACK_BYTES;

    datalink.create( cfg );
    mTaskId = datalink.start();
    sendTaskMsg( mTaskId, ITCMsg::TSK_MSG_WAKEUP, TIMEOUT_DONT_WAIT );

    /* Give the hardware time to boot */
    Chimera::delayMilliseconds( 100 );
    return true;
  }


  void DataLink::powerDn()
  {
  }


  Chimera::Status_t DataLink::recv( MsgFrag ** fragmentList )
  {
    Chimera::insert_debug_breakpoint();

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
    // Acquire access to the memory allocator
    // -------------------------------------------------*/
    // mContext->lock();

    // /*-------------------------------------------------
    // Make sure enough memory exists to allocate
    // -------------------------------------------------*/
    // const Frame &tmpFrame = mRXQueue.front();

    // if ( mContext->availableMemory() < tmpFrame.wireData.control.dataLength )
    // {
    //   mContext->unlock();
    //   mRXMutex.unlock();
    //   return Chimera::Status::MEMORY;
    // }

    // /*-------------------------------------------------
    // Allocate the memory and push out the data
    // -------------------------------------------------*/
    // msg.fragmentData   = mContext->malloc( tmpFrame.wireData.control.dataLength );
    // msg.fragmentLength = tmpFrame.wireData.control.dataLength;

    // RT_HARD_ASSERT( msg.fragmentData );
    // memcpy( msg.fragmentData, tmpFrame.wireData.userData, msg.fragmentLength );

    // /*-------------------------------------------------
    // Parse the dst socket into a string
    // -------------------------------------------------*/
    // // Consider encoding the destination socket into a checksum?
    // // Is there a reversible hash-like function? I guess this might
    // // be some type of compression algorithm.

    // /*-------------------------------------------------
    // Clean up the queue and return
    // -------------------------------------------------*/
    // mRXQueue.pop();
    // mRXMutex.unlock();

    return Chimera::Status::READY;
  }


  Chimera::Status_t DataLink::send( const MsgFrag *const msg, const IPAddress &ip )
  {
    Chimera::insert_debug_breakpoint();

    // /*-------------------------------------------------
    // Check the incoming data for validity. Fragmented
    // messages must not exceed a certain number.
    // -------------------------------------------------*/
    // if ( !msg.fragmentData || ( msg.fragmentLength > sizeof( PackedFrame::userData ) ) ||
    //      ( msg.fragmentNumber >= static_cast<uint16_t>( pow( 2, FRAME_NUMBER_BITS ) ) * sizeof( PackedFrame::userData ) ) )
    // {
    //   return Chimera::Status::MEMORY;
    // }

    // /*-------------------------------------------------
    // Process the error handler if the queue is full
    // -------------------------------------------------*/
    // mTXMutex.lock();
    // if ( mTXQueue.full() )
    // {
    //   mTXMutex.unlock();

    //   mPhyHandle.txQueueOverflows++;
    //   mCBService_registry.call<CallbackId::CB_ERROR_TX_QUEUE_FULL>();
    //   return Chimera::Status::FULL;
    // }

    // /*-------------------------------------------------
    // Build up the raw frame information
    // -------------------------------------------------*/
    // Frame tmpFrame;

    // /* Some high level packet parameters */
    // tmpFrame.nextHop      = ip;
    // tmpFrame.receivedPipe = Physical::PipeNumber::PIPE_INVALID;
    // tmpFrame.rtxCount     = Physical::AutoRetransmitCount::ART_COUNT_3;
    // tmpFrame.rtxDelay     = Physical::AutoRetransmitDelay::ART_DELAY_500uS;

    // /* Set packet control parameters */
    // tmpFrame.wireData.control.multicast   = false;
    // tmpFrame.wireData.control.requireACK  = true;
    // tmpFrame.wireData.control.frameNumber = msg.fragmentNumber;
    // tmpFrame.wireData.control.endpoint    = Endpoint::EP_APPLICATION_DATA_0;

    // /* Copy in the data */
    // tmpFrame.writeUserData( msg.fragmentData, msg.fragmentLength );

    // /*-------------------------------------------------
    // Enqueue and clean up
    // -------------------------------------------------*/
    // mTXQueue.push( tmpFrame );
    // mTXMutex.unlock();

    return Chimera::Status::READY;
  }


  IARP *DataLink::addressResolver()
  {
    return this;
  }


  size_t DataLink::maxTransferSize() const
  {
    return sizeof( PackedFrame::userData );
  }


  size_t DataLink::linkSpeed() const
  {
    return 1024;    // 1kB per second
  }


  size_t DataLink::lastActive() const
  {
    return mLastActive;
  }


  /*-------------------------------------------------------------------------------
  Service: ARP Interface
  -------------------------------------------------------------------------------*/
  Chimera::Status_t DataLink::addARPEntry( const IPAddress &ip, const void *const mac, const size_t size )
  {
    /*-------------------------------------------------
    Copy over the mac address if the size matches
    -------------------------------------------------*/
    if ( size != sizeof( Physical::MACAddress ) )
    {
      return Chimera::Status::FAIL;
    }

    Physical::MACAddress addr = 0;
    memcpy( &addr, mac, sizeof( Physical::MACAddress ) );

    /*-------------------------------------------------
    Attempt to insert the new entry
    -------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *this );
    bool success = mAddressCache.insert( ip, addr );

    /*-------------------------------------------------
    Invoke the error callback on failure
    -------------------------------------------------*/
    if ( !success )
    {
      mCBService_registry.call<CallbackId::CB_ERROR_ARP_LIMIT>();
    }

    return success ? Chimera::Status::OK : Chimera::Status::FAIL;
  }


  Chimera::Status_t DataLink::dropARPEntry( const IPAddress &ip )
  {
    /*-------------------------------------------------
    Does nothing if the entry doesn't exist
    -------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *this );
    mAddressCache.remove( ip );

    return Chimera::Status::OK;
  }


  bool DataLink::arpLookUp( const IPAddress &ip, void *const mac, const size_t size )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !mac || ( size != sizeof( Physical::MACAddress ) ) )
    {
      return false;
    }

    Chimera::Thread::LockGuard lck( *this );
    bool result = mAddressCache.lookup( ip, reinterpret_cast<Physical::MACAddress *>( mac ) );

    return result;
  }


  IPAddress DataLink::arpLookUp( const void *const mac, const size_t size )
  {
    // Currently not supported but might be in the future?
    RT_HARD_ASSERT( false );
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

    LOG_DEBUG( "Starting NRF24 network services\r\n" );

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

      mLastActive = Chimera::millis();
    }
  }


  Chimera::Status_t DataLink::setRootMAC( const Physical::MACAddress &mac )
  {
    /*-------------------------------------------------
    Make sure to gain exclusive access to the addresses
    -------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *this );

    /*-------------------------------------------------
    Reconfigure each endpoint's address
    -------------------------------------------------*/
    /* Endpoint zero (physical pipe #1) is the full address width */
    mEndpointMAC[ 0 ] = mac;

    /* Assign the unique pipe addresses for the remaining endpoints */
    for ( size_t ep = 1; ep < ARRAY_COUNT( sEndpointPipes ); ep++ )
    {
      mEndpointMAC[ ep ] = ( mac & ~0xFF ) | EndpointAddrModifiers[ ep + 1 ];
    }

    /*-------------------------------------------------
    Open each endpoint with the new addresses
    -------------------------------------------------*/
    Chimera::Status_t result = Chimera::Status::OK;
    for ( size_t ep = 0; ep < ARRAY_COUNT( sEndpointPipes ); ep++ )
    {
      result |= Physical::openReadPipe( mPhyHandle, sEndpointPipes[ ep ], mEndpointMAC[ ep ] );
    }

    /*-------------------------------------------------
    Officially assign the address if all good
    -------------------------------------------------*/
    if ( result == Chimera::Status::OK )
    {
      mPhyHandle.cfg.hwAddress = mac;
    }

    return result;
  }


  Physical::MACAddress DataLink::getEndpointMAC( const Endpoint endpoint )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( endpoint >= Endpoint::EP_NUM_OPTIONS )
    {
      return 0;
    }

    /*-------------------------------------------------
    Make sure to gain exclusive access to the addresses
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<DataLink>( *this );
    return mEndpointMAC[ static_cast<size_t>( endpoint ) ];
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
    if ( result != Chimera::Status::OK )
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
    Configure the hardware resources
    -------------------------------------------------*/
    LOG_IF_ERROR( ( Physical::powerUpDrivers( mPhyHandle ) == Chimera::Status::OK ), "Failed RF24 HW driver init\r\n" );

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

    /*-------------------------------------------------
    Record the final power up status
    -------------------------------------------------*/
    LOG_IF_ERROR( ( result == Chimera::Status::OK ), "Failed RF24 power up sequence\r\n" );

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
    using namespace Aurora::Logging;

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
    mCBService_registry.call<CallbackId::CB_TX_SUCCESS>();
    LOG_DEBUG( "Transmit Success\r\n" );
  }


  void DataLink::processTXFail()
  {
    using namespace Aurora::Logging;

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
    if ( failedFrame.wireData.control.requireACK )
    {
      Physical::flushTX( mPhyHandle );
      Physical::clrISREvent( mPhyHandle, Physical::bfISRMask::ISR_MSK_MAX_RT );
    }
    // else NO_ACK, which means there is nothing to clear. The data was lost to the ether.

    /*-------------------------------------------------
    Notify the network layer of the failed frame
    -------------------------------------------------*/
    mCBService_registry.call<CallbackId::CB_ERROR_TX_FAILURE>();
    LOG_DEBUG( "Transmit Fail\r\n" );
  }


  void DataLink::processTXQueue()
  {
    using namespace Aurora::Logging;
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
    Frame &cacheFrame               = mTXQueue.front();
    Physical::MACAddress deviceAddress = 0;

    if ( !mAddressCache.lookup( cacheFrame.nextHop, &deviceAddress ) )
    {
      mCBService_registry.call<CallbackId::CB_ERROR_ARP_RESOLVE>();
      mTXQueue.pop();
      mTXMutex.unlock();
      mTCB.inProgress = false;

      return;
    }

    /*-------------------------------------------------
    Modify the destination address to go to the correct
    pipe. Currently only pipes 4 & 5 are allocated for
    user data.
    -------------------------------------------------*/
    auto dstAddress = ( deviceAddress & ~0xFF ) | EndpointAddrModifiers[ PIPE_APP_DATA_0 ];

    /*-------------------------------------------------
    All information needed to TX the frame is known, so
    it's safe to transition to standby mode in prep for
    moving to TX mode once the data is loaded.
    -------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );

    /*-------------------------------------------------
    Open the proper port for writing. By default open
    the RX port to listen for ACKS or other responses.
    -------------------------------------------------*/
    Physical::openWritePipe( mPhyHandle, dstAddress );
    //Physical::openReadPipe( mPhyHandle, PIPE_TX, dstAddress );

    /*-------------------------------------------------
    Determine the reliability required on the TX. This
    assumes hardware is initialized with DynamicACK,
    the proper payload length settings are set, and any
    pipes needing auto-ack are enabled.
    -------------------------------------------------*/
    auto txType = Physical::PayloadType::PAYLOAD_NO_ACK;
    if ( cacheFrame.wireData.control.requireACK )
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

#if defined( SIMULATOR )
    mTCB.timeout = 5 * Chimera::Thread::TIMEOUT_1S;
#endif

    FrameBuffer data;
    cacheFrame.pack( data );

    LOG_DEBUG( "Transmit Packet\r\n" );
    Physical::writePayload( mPhyHandle, data.data(), data.size(), txType );
    mFSMControl.receive( Physical::FSM::MsgStartTX() );

    /*-------------------------------------------------
    Clean up and return
    -------------------------------------------------*/
    mTXMutex.unlock();

    /*-------------------------------------------------
    Simulators don't have interrupt processing, so set
    the pending event flag.
    -------------------------------------------------*/
#if defined( SIMULATOR )
    pendingEvent = true;
#endif /* SIMULATOR */
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
      mCBService_registry.call<CallbackId::CB_ERROR_RX_QUEUE_FULL>();
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
      Read out the data associated with the frame
      -------------------------------------------------*/
      size_t readSize = mPhyHandle.cfg.hwStaticPayloadWidth;
      if ( !readSize )
      {
        RT_HARD_ASSERT( false );    // Currently not supported
        readSize = Physical::getAvailablePayloadSize( mPhyHandle, pipe );
      }

      FrameBuffer tmpBuffer{ 0 };
      auto read_result = Physical::readPayload( mPhyHandle, tmpBuffer.data(), readSize );
      RT_HARD_ASSERT( read_result == Chimera::Status::OK );

      /*-------------------------------------------------
      Create a new frame
      -------------------------------------------------*/
      Frame tempFrame;
      tempFrame.receivedPipe = pipe;
      tempFrame.writeUserData( tmpBuffer.data(), readSize );

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
        mCBService_registry.call<CallbackId::CB_ERROR_RX_QUEUE_FULL>();

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
    mCBService_registry.call<CallbackId::CB_RX_SUCCESS>();
  }

}    // namespace Ripple::NetIf::NRF24::DataLink
