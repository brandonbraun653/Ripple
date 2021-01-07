/********************************************************************************
 *  File Name:
 *    session_process.hpp
 *
 *  Description:
 *    Process interface for the Session layer
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SESSION_PROCESS_HPP
#define RIPPLE_SESSION_PROCESS_HPP

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/function>


namespace Ripple::Session::Process
{
  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  /**
   *  The available processes that can be run on the network
   */
  enum class Type
  {
    JOIN_NETWORK,
    ESTABLISH_CONNECTION,
    TERMINATE_CONNECTION,

    NUM_OPTIONS,
    UNKNOWN
  };

  /**
   *  Allows the process to describe what runtime status it's currently in.
   *  This is used to inform the Session Service when it's time to transition
   *  to a new state.
   */
  enum class Status
  {
    IDLE,           /**< Process reports it's idling */
    INITIALIZE,     /**< Process is currently initializing */
    RUNNING,        /**< Process is currently running */
    COMPLETE,       /**< Process has finished */
    ERROR,          /**< Process has an error state that needs handling */

    NUM_OPTIONS,
    UNKNOWN
  };


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Describes a process that can execute in the session layer. Examples
   *  of these are making connections, requesting network path information,
   *  closing connections, etc. The purpose of this class is to provide a
   *  state machine framework that allows the session layer to execute many
   *  processes at one time.
   */
  class IProcess
  {
  public:
    virtual ~IProcess() = default;

    /**
     *  Gets the latest reported status of the process
     *  @return ProcessStatus
     */
    virtual Status getStatus() = 0;

    /**
     *  Prepares the process for the running state
     *  @return Chimera::Status_t
     */
    virtual Chimera::Status_t initialize() = 0;

    /**
     *  Starts executing the process
     *  @return Chimera::Status_t
     */
    virtual Chimera::Status_t start() = 0;

    /**
     *  Performs a single update step on the process
     *  @return Chimera::Status_t
     */
    virtual Chimera::Status_t update() = 0;

    /**
     *  Closes the process and terminates any resources
     *  @return Chimera::Status_t
     */
    virtual Chimera::Status_t close() = 0;

    /**
     *  Handles an error state within the process
     *  @return Chimera::Status_t
     */
    virtual Chimera::Status_t onError() = 0;
  };


  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   *  Control block to track the running state of the process
   */
  struct ControlBlock
  {
    IProcess *controller;
    Type type;                           /**< Which process this control block represents */
    Status status;                       /**< Reported status of the process */
    Chimera::Function::Opaque onSuccess; /**< Callback to execute on success */
    Chimera::Function::Opaque onFail;    /**< Callback to execute on failure */
  };
}    // namespace Ripple::Session::Process

#endif /* !RIPPLE_SESSION_PROCESS_HPP */
