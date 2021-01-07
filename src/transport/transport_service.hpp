/********************************************************************************
 *  File Name:
 *    transport_service.hpp
 *
 *  Description:
 *    Interface to the Transport layer services
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_TRANSPORT_SERVICE_HPP
#define RIPPLE_TRANSPORT_SERVICE_HPP

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/thread>

/* ETL Includes */
#include <etl/delegate.h>
#include <etl/delegate_service.h>

/* Ripple Includes */
#include <Ripple/src/session/session_types.hpp>
#include <Ripple/src/transport/transport_types.hpp>


namespace Ripple::Transport
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t THREAD_STACK          = STACK_BYTES( 1024 );
  static constexpr std::string_view THREAD_NAME = "transport";

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Gets the session layer handle from the context information
   *
   *  @param[in]  context       User context
   *  @return Handle *
   */
  Handle *getHandle( Session::Context context );


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Main service that executes the Transport layer functionalities, which include
   *  connection oriented communication, reliability, flow control, segmentation, and
   *  multiplexing.
   */
  class Service : public Chimera::Threading::Lockable
  {
  public:
    Service();
    ~Service();

    /**
     *  Main thread that executes the session manager process
     *
     *  @param[in]  context     Context used for the session
     *  @return void
     */
    void run( Session::Context context );

    /**
     *  Adjusts the periodic processing rate of the run() thread. This
     *  will directly effect the network performance, so be careful.
     *
     *  @param[in]  period      Rate in milliseconds to process the Session layer
     *  @return void
     */
    void setUpdateRate( const size_t period );

    /**
     *  Register a callback to be invoked upon some event that occurs during
     *  the service processing.
     *
     *  @param[in]  id          Which event to register against
     *  @param[in]  func        The function to register
     *  @return Chimera::Status_t
     */
    Chimera::Status_t registerCallback( const CallbackId id, etl::delegate<void( size_t )> func );


  private:
    /*-------------------------------------------------
    Private Functions
    -------------------------------------------------*/

    /*-------------------------------------------------
    Private Data
    -------------------------------------------------*/
    size_t mUpdateRate;                     /**< Main thread periodic update rate ms */
    size_t mServiceStarvedThreshold;        /**< Thread delay time that indicates the process is starved for processing */
    Chimera::Threading::ThreadId mThreadId; /**< Thread registration ID */
    Session::Context mContext;              /**< User context for the network stack */


    /**
     *  Helper for tracking/invoking event callbacks
     */
    etl::delegate_service<CallbackId::CB_NUM_OPTIONS> mDelegateRegistry;
  };

}    // namespace Ripple::Transport

#endif /* !RIPPLE_TRANSPORT_SERVICE_HPP */
