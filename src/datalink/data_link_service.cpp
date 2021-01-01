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
  Service::Service()
  {
  }


  Service::~Service()
  {
  }


  /*-------------------------------------------------------------------------------
  Service: Public Methods
  -------------------------------------------------------------------------------*/
  void Service::run( SessionContext context )
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
          /* Acknowledge the event */
          txInProgress = false;
          PHY::clrISREvent( *phyHandle, PHY::bfISRMask::ISR_MSK_MAX_RT );

          /* Move the state machine to process the fail state */
          processTXFail( context );
          // TODO: Notify the network layer
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
          txInProgress = false;
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
      //processRXQueue( context );
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


  Chimera::Status_t Service::registerCallback( const CallbackId id, CBFunction &func )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !( id < CallbackId::VECT_NUM_OPTIONS ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Register the callback
    -------------------------------------------------*/
    this->lock();
    mCBRegistry[ id ] = func;
    this->unlock();
    return Chimera::Status::OK;
  }


  /*-------------------------------------------------------------------------------
  Service: Protected Methods
  -------------------------------------------------------------------------------*/
  Chimera::Status_t Service::initialize( SessionContext context )
  {
    /*-------------------------------------------------
    Input protections
    -------------------------------------------------*/
    if ( !context )
    {
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Register the unhandled event callback
    -------------------------------------------------*/
    mCBRegistry[ VECT_UNHANDLED ] = CBFunction::create<Service, &Service::unhandledCallback>( *this );

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


  Chimera::Status_t Service::powerUpRadio( SessionContext session )
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


  void Service::invokeCallback( const CallbackId id )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !( id < CallbackId::VECT_NUM_OPTIONS ) )
    {
      return;
    }

    /*-------------------------------------------------
    Invoke the callback
    -------------------------------------------------*/
    CBData tmp;
    tmp.id = id;

    if ( mCBRegistry[ id ] )
    {
      mCBRegistry[ id ]( tmp );
    }
    else
    {
      mCBRegistry[ VECT_UNHANDLED ]( tmp );
    }
  }


  void Service::unhandledCallback( CBData &id )
  {
    /*-------------------------------------------------
    This really shouldn't be happening. Make noise if
    it does.
    -------------------------------------------------*/
    Chimera::System::softwareReset();
  }


  void Service::irqPinAsserted( void *arg )
  {
    /*-------------------------------------------------
    Let the user space thread know it has an event to
    process. Unsure what kind at this point.
    -------------------------------------------------*/
    using namespace Chimera::Threading;
    pendingEvent = true;
    sendTaskMsg( mThreadId, TSK_MSG_WAKEUP, TIMEOUT_DONT_WAIT );
  }


  void Service::processTXSuccess( SessionContext context )
  {
  }


  void Service::processTXFail( SessionContext context )
  {
  }


  void Service::processTXQueue( SessionContext context )
  {
    using namespace Chimera::Threading;
    PHY::Handle *phy = nullptr;

    /*-------------------------------------------------
    Cannot process another frame until the last one
    either successfully transmitted, or errored out.
    -------------------------------------------------*/
    if ( txInProgress || !mTXMutex.try_lock_for( TIMEOUT_1MS ) )
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
    MACAddress dstAddress   = 0;

    if ( !mAddressCache.lookup( cacheFrame.nextHop, &dstAddress ) )
    {
      CBData tmp;
      tmp.frameNumber = cacheFrame.frameNumber;
      tmp.id          = VECT_UNKNOWN_DESTINATION;

      mCBRegistry[ VECT_UNKNOWN_DESTINATION ]( tmp );
      mTXQueue.pop();
      mTXMutex.unlock();
      txInProgress = false;
      return;
    }

    /*-------------------------------------------------
    Open the proper port for writing
    -------------------------------------------------*/
    phy = PHY::getHandle( context );
    PHY::stopListening( *phy );
    PHY::openWritePipe( *phy, dstAddress );

    /*-------------------------------------------------
    Determine the reliability required on the TX. This
    assumes hardware is initialized with DynamicACK and
    the proper payload length settings are set.
    -------------------------------------------------*/
    PHY::PayloadType txType;
    if ( cacheFrame.control & bfControlFlags::CTRL_NO_ACK )
    {
      txType = PHY::PayloadType::PAYLOAD_NO_ACK;
    }
    else
    {
      txType = PHY::PayloadType::PAYLOAD_REQUIRES_ACK;
      PHY::setRetries( *phy, cacheFrame.rtxDelay, cacheFrame.rtxCount );

      // TODO: Does TX actually need this? Just RX right?
      PHY::toggleAutoAck( *phy, true, PHY::PIPE_NUM_0 );
    }

    /*-------------------------------------------------
    Write the data and start the transmission. The chip
    enable pin will be reset on successful TX or error.

    The length needs to be set by the Network layer in
    accordance with the expected hardware payload size.
    -------------------------------------------------*/
    PHY::writePayload( *phy, cacheFrame.payload, cacheFrame.length, txType );
    phy->cePin->setState( Chimera::GPIO::State::HIGH );

    txInProgress = true;
    mTXMutex.unlock();
  }


  void Service::processRXQueue( SessionContext context )
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
      this->invokeCallback( CallbackId::VECT_RX_QUEUE_FULL );
      return;
    }
    else if ( txInProgress )
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
