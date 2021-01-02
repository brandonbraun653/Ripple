/********************************************************************************
 *  File Name:
 *    link_thread.cpp
 *
 *  Description:
 *    Implements the data link layer thread functions
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/system>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/session>
#include <Ripple/datalink>
#include <Ripple/physical>


namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/


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
    Verify the drivers have been registered
    -------------------------------------------------*/
    PHY::Handle *phyHandle     = PHY::getHandle( context );
    DATALINK::Handle *dlHandle = DATALINK::getHandle( context );

    if ( !phyHandle || !dlHandle )
    {
      Chimera::System::softwareReset();
    }

    /*-------------------------------------------------
    Prepare the service to run
    -------------------------------------------------*/
    if ( this->initialize( context ) == Chimera::Status::OK )
    {
      mFSMControl.receive( PHY::FSM::MsgPowerUp() );
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
      if ( pendingEvent || this_thread::pendTaskMsg( TSK_MSG_WAKEUP, dlHandle->hwIRQEventTimeout ) )
      {
        pendingEvent      = false;
        uint8_t eventMask = PHY::getISREvent( *phyHandle );

        /*-------------------------------------------------
        The last packet failed to transmit correctly
        -------------------------------------------------*/
        if ( eventMask & PHY::bfISRMask::ISR_MSK_MAX_RT )
        {
          processTXFail( context );
        }

        /*-------------------------------------------------
        A new packet was received
        -------------------------------------------------*/
        if ( eventMask & PHY::bfISRMask::ISR_MSK_RX_DR )
        {
          PHY::clrISREvent( *phyHandle, PHY::bfISRMask::ISR_MSK_RX_DR );
          processRXQueue( context );
        }

        /*-------------------------------------------------
        A packet successfully transmitted
        -------------------------------------------------*/
        if ( eventMask & PHY::bfISRMask::ISR_MSK_TX_DS )
        {
          /* Acknowledge the event */
          mTXInProgress = false;
          PHY::clrISREvent( *phyHandle, PHY::bfISRMask::ISR_MSK_TX_DS );

          /* Move the state machine forward */
          processTXSuccess( context );
        }
      }

      /*-------------------------------------------------
      Another thread may have woken this one to process
      new frame queue data. Check if any is available.

      Handle RX first to keep the HW FIFOs empty.
      -------------------------------------------------*/
      // processRXQueue( context );
      processTXQueue( context );
    }
  }


  Chimera::Status_t Service::enqueueFrame( const Frame &frame )
  {
    /*-------------------------------------------------
    Check if enough room is available
    -------------------------------------------------*/
    mTXMutex.lock();
    if ( mTXQueue.full() )
    {
      mTXMutex.unlock();
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


  Chimera::Status_t Service::addARPEntry( const NET::IPAddress ip, const PHY::MACAddress &mac )
  {
    this->lock();
    bool result = mAddressCache.insert( ip, mac );
    this->unlock();

    return result ? Chimera::Status::OK : Chimera::Status::FAIL;
  }


  Chimera::Status_t Service::dropARPEntry( const NET::IPAddress ip )
  {
    this->lock();
    mAddressCache.remove( ip );
    this->unlock();

    return Chimera::Status::OK;
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


  Chimera::Status_t Service::powerUpRadio( Session::Context session )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    auto result   = Chimera::Status::OK;
    auto physical = PHY::getHandle( session );
    auto datalink = getHandle( session );

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
    result |= PHY::openDevice( physical->cfg, *physical );
    result |= PHY::resetRegisterDefaults( *physical );

    /*-------------------------------------------------
    Apply basic global user settings
    -------------------------------------------------*/
    result |= PHY::setCRCLength( *physical, physical->cfg.hwCRCLength );
    result |= PHY::setAddressWidth( *physical, physical->cfg.hwAddressWidth );
    result |= PHY::setISRMasks( *physical, physical->cfg.hwISRMask );
    result |= PHY::setRFChannel( *physical, physical->cfg.hwRFChannel );
    result |= PHY::setDataRate( *physical, physical->cfg.hwDataRate );
    result |= PHY::setPALevel( *physical, physical->cfg.hwPowerAmplitude );
    result |= PHY::toggleDynamicAck( *physical, true );

    if ( datalink->dynamicPackets )
    {
      result |= PHY::toggleDynamicPayloads( *physical, PHY::PIPE_NUM_ALL, true );
    }
    else
    {
      result |= PHY::toggleDynamicPayloads( *physical, PHY::PIPE_NUM_ALL, false );
      result |= PHY::setStaticPayloadSize( *physical, datalink->staticPacketSize, PHY::PIPE_NUM_ALL );
    }

    /*-------------------------------------------------
    Flush hardware FIFOs to clear pre-existing data
    -------------------------------------------------*/
    result |= PHY::flushRX( *physical );
    result |= PHY::flushTX( *physical );

    /*-------------------------------------------------
    Initialize the FSM controller
    -------------------------------------------------*/
    _fsmStateList[ PHY::FSM::StateId::POWERED_OFF ] = &_fsmState_PoweredOff;
    _fsmStateList[ PHY::FSM::StateId::STANDBY_1 ]   = &_fsmState_Standby1;
    _fsmStateList[ PHY::FSM::StateId::RX_MODE ]     = &_fsmState_RXMode;
    _fsmStateList[ PHY::FSM::StateId::TX_MODE ]     = &_fsmState_TXMode;

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


  void Service::processTXSuccess( Session::Context context )
  {
  }


  void Service::processTXFail( Session::Context context )
  {
    PHY::Handle *phyHandle = PHY::getHandle( context );

    /*-------------------------------------------------------------------------------
    The only reason a TX fail event must be processed is due to a max retry IRQ. In
    this case, the data is not removed from the TX FIFO, so it must be done manually.
    (RM 8.4) First transition back to Standby-1 mode, then clear event flags and flush
    the TX FIFO. Otherwise, the IRQ will continuously fire.
    -------------------------------------------------------------------------------*/
    mFSMControl.receive( PHY::FSM::MsgGoToSTBY() );
    PHY::flushTX( *phyHandle );
    PHY::clrISREvent( *phyHandle, PHY::bfISRMask::ISR_MSK_MAX_RT );

    /*-------------------------------------------------
    Dump the frame queue and unlock the TX process
    -------------------------------------------------*/
    mTXInProgress = false;

    mTXMutex.lock();
    mTXQueue.pop();
    mTXMutex.unlock();

    /*-------------------------------------------------
    Notify the network layer of the failed frame
    -------------------------------------------------*/
    mDelegateRegistry.call<CallbackId::CB_ERROR_TX_MAX_RETRY>();
  }


  void Service::processTXQueue( Session::Context context )
  {
    using namespace Chimera::Threading;
    PHY::Handle *phy = nullptr;

    /*-------------------------------------------------
    Cannot process another frame until the last one
    either successfully transmitted, or errored out.
    -------------------------------------------------*/
    if ( mTXInProgress || !mTXMutex.try_lock_for( TIMEOUT_1MS ) )
    {
      return;
    }
    else if ( mTXQueue.empty() )
    {
      /*-------------------------------------------------
      Nothing to TX, so ensure hardware is listening
      -------------------------------------------------*/
      mTXMutex.unlock();
      mFSMControl.receive( PHY::FSM::MsgStartRX() );
      return;
    }

    /*-------------------------------------------------
    Look up the hardware address associated with the
    destination node.
    -------------------------------------------------*/
    const Frame &cacheFrame = mTXQueue.front();
    PHY::MACAddress dstAddress   = 0;

    if ( !mAddressCache.lookup( cacheFrame.nextHop, &dstAddress ) )
    {
      mDelegateRegistry.call<CallbackId::CB_ERROR_ARP_RESOLVE>();
      mTXQueue.pop();
      mTXMutex.unlock();
      mTXInProgress = false;

      return;
    }

    /*-------------------------------------------------
    All information needed to TX the frame is known, so
    it's safe to transition to standby mode in prep for
    moving to TX mode once the data is loaded.
    -------------------------------------------------*/
    mFSMControl.receive( PHY::FSM::MsgGoToSTBY() );

    /*-------------------------------------------------
    Open the proper port for writing
    -------------------------------------------------*/
    phy = PHY::getHandle( context );
    PHY::openWritePipe( *phy, dstAddress );

    /*-------------------------------------------------
    Determine the reliability required on the TX. This
    assumes hardware is initialized with DynamicACK and
    the proper payload length settings are set.
    -------------------------------------------------*/
    PHY::PayloadType txType;
    if ( cacheFrame.control & bfControlFlags::CTRL_PAYLOAD_ACK )
    {
      txType = PHY::PayloadType::PAYLOAD_REQUIRES_ACK;
      PHY::setRetries( *phy, cacheFrame.rtxDelay, cacheFrame.rtxCount );

      // TODO: Does TX actually need this? Just RX right?
      PHY::toggleAutoAck( *phy, true, PHY::PIPE_NUM_0 );
    }
    else
    {
      txType = PHY::PayloadType::PAYLOAD_NO_ACK;
    }

    /*-------------------------------------------------
    Write the data to the TX FIFO and transition to the
    active TX mode.
    -------------------------------------------------*/
    mTXInProgress = true;
    PHY::writePayload( *phy, cacheFrame.payload, cacheFrame.length, txType );
    mFSMControl.receive( PHY::FSM::MsgStartTX() );

    mTXMutex.unlock();
  }


  void Service::processRXQueue( Session::Context context )
  {
    using namespace Chimera::Threading;

    /*-------------------------------------------------
    Try to acquire the lock. Normally it will be free.
    -------------------------------------------------*/
    if ( !mRXMutex.try_lock_for( TIMEOUT_1MS ) )
    {
      return;
    }
    else if ( mRXQueue.full() )
    {
      /*-------------------------------------------------
      Try and dump data out of the queue
      -------------------------------------------------*/
      mRXMutex.unlock();
      mDelegateRegistry.call<CallbackId::CB_ERROR_RX_QUEUE_FULL>();
      return;
    }
    else if ( mTXInProgress )
    {
      /*-------------------------------------------------
      TX-ing and RX-ing are exclusive. Play nice.
      -------------------------------------------------*/
      mRXMutex.unlock();
      return;
    }

    /*-------------------------------------------------
    Read out the next frame from the device
    -------------------------------------------------*/
    Chimera::insert_debug_breakpoint();
    // TODO: Implement me!


    mRXMutex.unlock();
  }

}    // namespace Ripple::DATALINK
