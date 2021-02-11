/********************************************************************************
 *  File Name:
 *    session_service.hpp
 *
 *  Description:
 *    Interface to the session layer services
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SESSION_SERVICE_HPP
#define RIPPLE_SESSION_SERVICE_HPP

/* Chimera Includes */
#include <Chimera/common>

/* ETL Includes */
#include <etl/delegate.h>
#include <etl/delegate_service.h>

/* Ripple Includes */
#include <Ripple/src/session/process/process_intf.hpp>
#include <Ripple/src/session/session_process.hpp>
#include <Ripple/src/session/session_types.hpp>


namespace Ripple::Session
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t THREAD_STACK          = STACK_BYTES( 1024 );
  static constexpr std::string_view THREAD_NAME = "session";

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Gets the session layer handle from the context information
   *
   *  @param[in]  context       User context
   *  @return Handle *
   */
  Handle *getHandle( Context context );


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Main service that executes the Session layer functionality. The majority
   *  of the methods here are not meant to be called directly by the user but
   *  rather by higher level session methods that control Process activation.
   */
  class Service : public Chimera::Thread::Lockable<Service>
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

    /**
     *  Attempts to find an existing network and then join it. This will
     *  assign an IP and MAC address to the device should a connection succeed.
     *
     *  @note Blocks until connected or timeout expires.
     *
     *  @param[in]  timeout     How long to wait (ms) for success
     *  @return bool
     */
    bool joinNetwork( const size_t timeout );

  private:
    friend Chimera::Thread::Lockable<Service>;


    /*-------------------------------------------------
    Private Methods
    -------------------------------------------------*/
    /**
     *  Registers the available Session layer processes with the class
     *  @return void
     */
    void registerProcess();

    /**
     *  After processes have been registered, initializes them to
     *  be ready to run when the user requests it.
     *
     *  @return void
     */
    void initializeProcess();

    /*-------------------------------------------------
    Private Data
    -------------------------------------------------*/
    size_t mUpdateRate;                     /**< Main thread periodic update rate ms */
    size_t mServiceStarvedThreshold;        /**< Thread delay time that indicates the process is starved for processing */
    Chimera::Thread::TaskId mTaskId; /**< Thread registration ID */
    Session::Context mContext;              /**< User context for the network stack */

    /**
     *  Helper for tracking/invoking event callbacks
     */
    etl::delegate_service<CallbackId::CB_NUM_OPTIONS> mDelegateRegistry;

    /**
     *  Tracks the runtime state of each process that can
     *  be executed by the service. Most will be idle.
     */
    Process::ControlBlock mProcessCB[ static_cast<size_t>( Process::Type::NUM_OPTIONS ) ];

    /**
     *  Process controller instances. These are what perform the real
     *  work in the functionality expressed by the service.
     */
    Process::JoinNetwork mProc_JoinNetwork;
    Process::EstablishConnection mProc_EstablishConn;
    Process::TerminateConnection mProc_TerminateConn;
  };
}    // namespace Ripple::Session

#endif /* !RIPPLE_SESSION_SERVICE_HPP */
