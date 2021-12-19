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
#include <Ripple/netstack>
#include <Ripple/shared>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_internal.hpp>

/*-------------------------------------------------------------------------------
Literals
-------------------------------------------------------------------------------*/
#define DEBUG_MODULE ( false )

#if !defined( NRF_LINK_FRAME_RETRIES )
#define NRF_LINK_FRAME_RETRIES ( 3 )
#endif

#if !defined( NRF_STAT_UPDATE_PERIOD_MS )
#define NRF_STAT_UPDATE_PERIOD_MS ( Chimera::Thread::TIMEOUT_100MS )
#endif

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
  Public Functions
  -------------------------------------------------------------------------------*/
  DataLink *createNetIf( Context_rPtr context )
  {
    RT_HARD_ASSERT( context );
    void *ptr = context->malloc( sizeof( DataLink ) );
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
  bool DataLink::powerUp( void *context )
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
    LOG_INFO( "Initializing NRF24...\r\n" );
    if ( powerUpRadio( mPhyHandle ) != Chimera::Status::OK )
    {
      LOG_ERROR( "Failed initializing NRF24\r\n" );
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
    cfg.priority                              = 4;
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


  Chimera::Status_t DataLink::recv( Fragment_sPtr &fragmentList )
  {
    /*-------------------------------------------------
    Is the RX queue empty?
    -------------------------------------------------*/
    Chimera::Thread::LockGuard queueLock( mRXMutex );
    if ( mRXQueue.empty() )
    {
      return Chimera::Status::EMPTY;
    }

    /*-------------------------------------------------
    Pull out each packet and assemble into a list
    -------------------------------------------------*/
    Chimera::Thread::LockGuard contextLock( *mContext );

    Fragment_sPtr rootMsg    = Fragment_sPtr();
    Chimera::Status_t result = Chimera::Status::READY;

    while ( !mRXQueue.empty() )
    {
      /*-------------------------------------------------
      Always pull the next frame to prevent stalling the
      low level hardware processing.
      -------------------------------------------------*/
      Frame &tmpFrame = mRXQueue.front();
      mRXQueue.pop();

      /*-------------------------------------------------
      Make sure enough memory exists to allocate. If it
      fails, keep emptying the hardware queue.
      -------------------------------------------------*/
      Fragment_sPtr newFrag = allocFragment( &mContext->mHeap, tmpFrame.wireData.control.dataLength );
      if ( !newFrag )
      {
        LOG_DEBUG_IF( DEBUG_MODULE, "No memory to allocate for incoming fragment\r\n" );
        result = Chimera::Status::MEMORY;
        continue;
      }

      /*-------------------------------------------------
      Copy in fragment fields
      -------------------------------------------------*/
      newFrag->length = tmpFrame.wireData.control.dataLength;
      newFrag->number = tmpFrame.wireData.control.frameNumber;
      newFrag->uuid   = tmpFrame.wireData.control.uuid;
      newFrag->total  = tmpFrame.wireData.control.totalFrames;

      void **payload_buffer = newFrag->data.get();
      tmpFrame.readUserData( *payload_buffer, newFrag->length );

      /*-------------------------------------------------
      Link the fragments by inserting at the front. Order
      does not matter here.
      -------------------------------------------------*/
      if ( !rootMsg )
      {
        newFrag->next = Fragment_sPtr();
        rootMsg       = newFrag;
      }
      else
      {
        Fragment_sPtr prevRoot = rootMsg;
        rootMsg                = newFrag;
        rootMsg->next          = prevRoot;
      }
    }

    /*-------------------------------------------------
    Let the caller know of the newly created list
    -------------------------------------------------*/
    if ( result == Chimera::Status::READY )
    {
      fragmentList = std::move( rootMsg );
    }

    return result;
  }


  Chimera::Status_t DataLink::send( const Fragment_sPtr msg, const IPAddress &ip )
  {
    /*-------------------------------------------------
    Input Protections
    -------------------------------------------------*/
    if ( !msg )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Construct the NRF24 data link layer packets
    -------------------------------------------------*/
    Chimera::Thread::LockGuard txLock( mTXMutex );
    Fragment_sPtr fragPtr = msg;
    size_t fragCounter    = 0;

    while ( fragPtr )
    {
      /*-------------------------------------------------
      Check the incoming data for validity. Fragmented
      messages must not exceed a certain size.
      -------------------------------------------------*/
      if ( !fragPtr->data || ( fragPtr->length > sizeof( PackedFrame::userData ) ) )
      {
        LOG_DEBUG_IF( DEBUG_MODULE, "Fragment %d is invalid\r\n", fragCounter );
        return Chimera::Status::MEMORY;
      }
      else if ( mTXQueue.full() )
      {
        LOG_DEBUG_IF( DEBUG_MODULE, "TX queue full\r\n" );
        return Chimera::Status::FULL;
      }

      /*-------------------------------------------------
      Build up the raw frame information
      -------------------------------------------------*/
      Frame tmpFrame;

      /* Some high level packet parameters */
      tmpFrame.txAttempts   = 1;
      tmpFrame.nextHop      = ip;
      tmpFrame.receivedPipe = Physical::PipeNumber::PIPE_INVALID;
      tmpFrame.rtxCount     = mPhyHandle.cfg.hwRTXCount;
      tmpFrame.rtxDelay     = mPhyHandle.cfg.hwRTXDelay;

      /* Set packet control parameters */
      memset( &tmpFrame.wireData, 0, sizeof( PackedFrame ) );
      tmpFrame.wireData.control.multicast   = false;
      tmpFrame.wireData.control.requireACK  = true;
      tmpFrame.wireData.control.frameNumber = static_cast<uint8_t>( fragPtr->number );
      tmpFrame.wireData.control.totalFrames = static_cast<uint8_t>( fragPtr->total );
      tmpFrame.wireData.control.endpoint    = Endpoint::EP_APPLICATION_DATA_0;
      tmpFrame.wireData.control.uuid        = fragPtr->uuid;

      /* Copy in the data */
      void **data = fragPtr->data.get();
      tmpFrame.writeUserData( *data, fragPtr->length );

      /*-------------------------------------------------
      Enqueue and prep for the next frame
      -------------------------------------------------*/
      mTXQueue.push( tmpFrame );
      fragPtr = fragPtr->next;
      fragCounter++;
    }

    return Chimera::Status::READY;
  }


  void DataLink::getStats( PerfStats &stats )
  {
    Chimera::Thread::LockGuard _lock( *this );
    stats = mStats;
  }


  IARP *DataLink::addressResolver()
  {
    return this;
  }


  size_t DataLink::maxTransferSize() const
  {
    return sizeof( PackedFrame::userData );
  }


  size_t DataLink::maxNumFragments() const
  {
    return static_cast<size_t>( pow( 2, FRAME_NUMBER_BITS ) );
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

    /*-------------------------------------------------------------------------
    Wait for thread to be told to initialize
    -------------------------------------------------------------------------*/
    Ripple::TaskWaitInit();
    this_thread::set_name( "DataLink_Service" );
    mTaskId = this_thread::id();

    LOG_INFO( "Starting NRF24 network services\r\n" );

    /*-------------------------------------------------------------------------
    Establish communication with the radio and set up user configuration
    -------------------------------------------------------------------------*/
    memset( &mStats, 0, sizeof( mStats ) );
    mFSMControl.receive( Physical::FSM::MsgPowerUp() );
    mSystemEnabled = true;

    /*-------------------------------------------------------------------------
    Execute the service
    -------------------------------------------------------------------------*/
    while ( 1 )
    {
      /*-----------------------------------------------------------------------
      Process the core radio events. This is driven by a GPIO interrupt tied to
      the IRQ pin, or by another task informing that it's time to process data.
      -----------------------------------------------------------------------*/
      if ( pendingEvent || this_thread::pendTaskMsg( TSK_MSG_WAKEUP, 10 ) )
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

      /*-----------------------------------------------------------------------
      Handle packet TX timeouts. Getting here means there is likely a setup
      issue with the hardware or no receiver exists to accept the data.
      -----------------------------------------------------------------------*/
      if ( mTCB.inProgress && ( ( Chimera::millis() - mTCB.start ) > mTCB.timeout ) )
      {
        processTXFail();
      }

      /*-----------------------------------------------------------------------
      Another thread may have woken this one to process new frame queue data.
      Check if any is available. Handle RX first to keep the HW FIFOs empty.
      -----------------------------------------------------------------------*/
      processRXQueue();
      processTXQueue();

      /*-----------------------------------------------------------------------
      Calculate performance metrics
      -----------------------------------------------------------------------*/
      updateStats();

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
    LOG_ERROR_IF( ( Physical::powerUpDrivers( mPhyHandle ) != Chimera::Status::OK ), "Failed RF24 HW driver init\r\n" );

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
    mFSMControl.set_states( _fsmStateList, std::size( _fsmStateList ) );
    mFSMControl.start();

    /*-------------------------------------------------
    Record the final power up status
    -------------------------------------------------*/
    LOG_ERROR_IF( ( result != Chimera::Status::OK ), "Failed RF24 power up sequence\r\n" );

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

    /*-------------------------------------------------------------------------
    Let user space thread know it has an event to process. Unsure what kind.
    -------------------------------------------------------------------------*/
    if ( mSystemEnabled )
    {
      pendingEvent = true;
      sendTaskMsg( mTaskId, TSK_MSG_WAKEUP, TIMEOUT_DONT_WAIT );
    }
  }


  void DataLink::processTXSuccess()
  {
    using namespace Aurora::Logging;

    /*-------------------------------------------------------------------------
    Ack the success
    -------------------------------------------------------------------------*/
    mFSMControl.receive( Physical::FSM::MsgGoToSTBY() );
    Physical::clrISREvent( mPhyHandle, Physical::bfISRMask::ISR_MSK_TX_DS );
    mTCB.inProgress = false;

    /*-------------------------------------------------------------------------
    Remove the now TX'd data off the queue
    -------------------------------------------------------------------------*/
    mTXMutex.lock();
    mTXQueue.pop();
    mTXMutex.unlock();

    /*-------------------------------------------------------------------------
    Update runtime stats
    -------------------------------------------------------------------------*/
    this->lock();
    mStats.tx_bytes += sizeof( PackedFrame );
    mStats.frame_tx += 1;
    this->unlock();

    /*-------------------------------------------------------------------------
    Notify the network layer of the success
    -------------------------------------------------------------------------*/
    mCBService_registry.call<CallbackId::CB_TX_SUCCESS>();
    LOG_DEBUG_IF( DEBUG_MODULE, "Transmit Success\r\n" );
  }


  void DataLink::processTXFail()
  {
    using namespace Aurora::Logging;

    /*-------------------------------------------------------------------------
    Dump the unsuccessful frame from the queue
    -------------------------------------------------------------------------*/
    Frame failedFrame;

    mTXMutex.lock();
    mTXQueue.pop_into( failedFrame );
    mTXMutex.unlock();

    /*-------------------------------------------------------------------------
    Update stats
    -------------------------------------------------------------------------*/
    this->lock();
    mStats.frame_tx_fail += 1;
    this->unlock();

    /*-------------------------------------------------------------------------
    Transition back to an idle state
    -------------------------------------------------------------------------*/
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

    /*-------------------------------------------------------------------------
    Notify the network layer of the failed frame
    -------------------------------------------------------------------------*/
    mCBService_registry.call<CallbackId::CB_ERROR_TX_FAILURE>();
    LOG_DEBUG_IF( DEBUG_MODULE, "Transmit Fail\r\n" );

    /*-------------------------------------------------------------------------
    QOS: Retransmit the frame, using a backoff strategy.
    -------------------------------------------------------------------------*/
    this->retransmitFrame( failedFrame, true );
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
    Frame &cacheFrame                  = mTXQueue.front();
    Physical::MACAddress deviceAddress = 0;

    if ( !mAddressCache.lookup( cacheFrame.nextHop, &deviceAddress ) )
    {
      mCBService_registry.call<CallbackId::CB_ERROR_ARP_RESOLVE>();
      mTXQueue.pop();
      mTXMutex.unlock();
      mTCB.inProgress = false;
      LOG_ERROR_IF( DEBUG_MODULE, "NRF24 ARP lookup failure for next hop: %d\r\n", cacheFrame.nextHop );

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
    Open the proper port for writing
    -------------------------------------------------*/
    Physical::openWritePipe( mPhyHandle, dstAddress );

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

    FrameBuffer data;
    cacheFrame.pack( data );

    LOG_DEBUG_IF( DEBUG_MODULE, "Transmit Packet\r\n" );
    Physical::writePayload( mPhyHandle, data.data(), data.size(), txType );
    mFSMControl.receive( Physical::FSM::MsgStartTX() );

    /*-------------------------------------------------
    Clean up and return
    -------------------------------------------------*/
    mTXMutex.unlock();
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
    else if ( !mRXMutex.try_lock_for( 25 * Chimera::Thread::TIMEOUT_1MS ) )
    {
      /*-------------------------------------------------
      Who is holding the RX lock for so long?
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
      tempFrame.unpack( tmpBuffer );
      tempFrame.receivedPipe = pipe;

      /*-------------------------------------------------
      Enqueue the frame if possible, call the network
      handler if not. Otherwise the data is simply lost.
      -------------------------------------------------*/
      Chimera::Thread::LockGuard _lock( *this );
      if ( !mRXQueue.full() )
      {
        mStats.rx_bytes += readSize;
        mStats.frame_rx += 1;

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
        else
        {
          mStats.rx_bytes_lost += readSize;
          mStats.frame_rx_drop += 1;
          LOG_DEBUG_IF( DEBUG_MODULE, "RX frame lost due to netif queue full\r\n" );
        }
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


  void DataLink::retransmitFrame( Frame &frame, const bool rtxBackoff )
  {
    /*-------------------------------------------------------------------------
    Input Protection
    -------------------------------------------------------------------------*/
    if ( frame.txAttempts > NRF_LINK_FRAME_RETRIES )
    {
      LOG_ERROR( "Frame exceeded link layer retry attempts\r\n" );
      return;
    }

    /*-------------------------------------------------------------------------
    Update the runtime data
    -------------------------------------------------------------------------*/
    frame.txAttempts++;
    if ( rtxBackoff && ( frame.rtxDelay < Physical::AutoRetransmitDelay::ART_DELAY_MAX ) )
    {
      frame.rtxDelay = static_cast<Physical::AutoRetransmitDelay>( static_cast<size_t>( frame.rtxDelay ) + 1 );
    }

    /*-------------------------------------------------------------------------
    Push to the transmit queue
    -------------------------------------------------------------------------*/
    Chimera::Thread::LockGuard _lock( mTXMutex );

    if ( !mTXQueue.full() )
    {
      mTXQueue.push( frame );
    }
    else
    {
      Chimera::Thread::LockGuard _lock2( *this );
      mStats.frame_tx_drop += 1;
      mStats.tx_bytes_lost += frame.size();
      LOG_ERROR( "Lost frame in retransmit attempt. TX queue full.\r\n" );
    }
  }


  void DataLink::updateStats()
  {
    /*-------------------------------------------------------------------------
    Local Constants
    -------------------------------------------------------------------------*/
    static constexpr size_t UPDATES_PER_SECOND = Chimera::Thread::TIMEOUT_1S / NRF_STAT_UPDATE_PERIOD_MS;

    /*-------------------------------------------------------------------------
    Calculation period elapsed?
    -------------------------------------------------------------------------*/
    static size_t last_update   = Chimera::millis();
    static PerfStats last_stats = mStats;

    if ( ( Chimera::millis() - last_update ) < NRF_STAT_UPDATE_PERIOD_MS )
    {
      return;
    }

    /*-------------------------------------------------------------------------
    Update statistics
    -------------------------------------------------------------------------*/
    last_update = Chimera::millis();
    Chimera::Thread::LockGuard _lock( *this );

    mStats.link_speed_tx = ( mStats.tx_bytes - last_stats.tx_bytes ) * UPDATES_PER_SECOND;
    mStats.link_speed_rx = ( mStats.rx_bytes - last_stats.rx_bytes ) * UPDATES_PER_SECOND;

    last_stats = mStats;
  }

}    // namespace Ripple::NetIf::NRF24::DataLink
