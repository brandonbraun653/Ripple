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
    /*-------------------------------------------------
    Prepare the service to run
    -------------------------------------------------*/
    if ( auto result = this->initialize(); result != Chimera::Status::OK )
    {
      mCBRegistry.callback<INIT_ERROR>();
    }

    /*-------------------------------------------------
    Exectue the service
    -------------------------------------------------*/
    while ( 1 ) {}
  }


  Chimera::Status_t Service::enqueueFrame( const Frame &frame )
  {
  }


  Chimera::Status_t Service::dequeueFrame( Frame &frame )
  {
  }


  Chimera::Status_t Service::registerCallback( const CallbackId id, CBFunction &func )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( ( id < CallbackId::VECTOR_ID_START ) || ( id >= CallbackId::VECTOR_ID_END ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    mCBRegistry.register_callback( id, func );
  }


  /*-------------------------------------------------
  Protected Methods
  -------------------------------------------------*/
  Chimera::Status_t Service::initialize()
  {
    /*-------------------------------------------------
    Register the unhandled event callback
    -------------------------------------------------*/
    mCBRegistry.register_unhandled_callback( mUHCFunction );
  }

}    // namespace Ripple::DATALINK
