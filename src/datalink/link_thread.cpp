/********************************************************************************
 *  File Name:
 *    link_thread.cpp
 *
 *  Description:
 *    Implements the data link layer thread functions
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/src/datalink/link_thread.hpp>
#include <Ripple/src/physical/phy_device_driver.hpp>
#include <Ripple/src/physical/phy_device_types.hpp>

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

  /*-------------------------------------------------
  Public Methods
  -------------------------------------------------*/
  void Service::run( SessionContext context )
  {
    using namespace Chimera::Threading;

    /*-------------------------------------------------
    Wait for this thread to be told to initialize
    -------------------------------------------------*/
    ThreadMsg msg = ITCMsg::ITC_NOP;
    while ( true )
    {
      if ( this_thread::receiveTaskMsg( msg, TIMEOUT_BLOCK ) && ( msg == ITCMsg::ITC_WAKEUP ) )
      {
        break;
      }
      else
      {
        this_thread::yield();
      }
    }

    /*-------------------------------------------------
    Prepare the service to run
    -------------------------------------------------*/
    if ( auto result = this->initialize( context ); result != Chimera::Status::OK )
    {
      CBData tmp;
      tmp.id = VECT_UNHANDLED;
      mCBRegistry[ VECT_UNHANDLED ]( tmp );
    }

    auto hndl = PHY::getHandle( context );

    Ripple::PHY::openDevice( hndl->cfg, *hndl );
    Ripple::PHY::resetRegisterDefaults( *hndl );

    /*-------------------------------------------------
    Exectue the service
    -------------------------------------------------*/
    while ( 1 )
    {
      Chimera::delayMilliseconds( 100 );
    }
  }


  Chimera::Status_t Service::enqueueFrame( const Frame &frame )
  {
    /*-------------------------------------------------
    Check if enough room is available
    -------------------------------------------------*/
    mMutex.lock();
    if( mTXQueue.full() )
    {
      mMutex.unlock();
      return Chimera::Status::FULL;
    }

    /*-------------------------------------------------
    Otherwise enqueue
    -------------------------------------------------*/
    mTXQueue.push( frame );
    mMutex.unlock();
    return Chimera::Status::OK;
  }


  Chimera::Status_t Service::dequeueFrame( Frame &frame )
  {
    /*-------------------------------------------------
    Check if queue is empty
    -------------------------------------------------*/
    mMutex.lock();
    if( mRXQueue.empty() )
    {
      mMutex.unlock();
      return Chimera::Status::EMPTY;
    }

    /*-------------------------------------------------
    Otherwise dequeue
    -------------------------------------------------*/
    mRXQueue.pop_into( frame );
    mMutex.unlock();
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
    mMutex.lock();
    mCBRegistry[ id ] = func;
    mMutex.unlock();
    return Chimera::Status::OK;
  }


  /*-------------------------------------------------
  Protected Methods
  -------------------------------------------------*/
  Chimera::Status_t Service::initialize( SessionContext context )
  {
    /*-------------------------------------------------
    Input protections
    -------------------------------------------------*/
    if( !context )
    {
      return Chimera::Status::FAIL;
    }

    mMutex.lock();

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
    auto result = this->powerUpRadio( context );

    mMutex.unlock();
    return result;
  }


  Chimera::Status_t Service::powerUpRadio( SessionContext session )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    auto result   = Chimera::Status::OK;
    auto physical = PHY::getHandle( session );
    if( !physical )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    GPIO interrupt configuration
    -------------------------------------------------*/
    // Attach a member function

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

    return result;
  }


  void Service::unhandledCallback( CBData &id )
  {
    // Do nothing
  }
}    // namespace Ripple::DATALINK
