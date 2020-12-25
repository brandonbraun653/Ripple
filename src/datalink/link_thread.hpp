/********************************************************************************
 *  File Name:
 *    link_thread.hpp
 *
 *  Description:
 *    Data link layer thread interface
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_DATA_LINK_THREAD_HPP
#define RIPPLE_DATA_LINK_THREAD_HPP

/* Chimera Includes */
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_memory_config.hpp>
#include <Ripple/src/datalink/link_types.hpp>

namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Thread class that performs the datalink layer services, whose primary
   *  responsibilities are to:
   *
   *    1. Control data TX/RX on the physical medium
   *    2. Data queuing and scheduling
   *    3. Translate logical network addresses in to physical addresses
   *
   *  A Frame is the primary data type this layer understands how to process.
   */
  class Service
  {
  public:
    Service();
    ~Service();

    /**
     *  Main thread that executes the datalink layer services
     *
     *  @param[in]  context     Critical net stack information
     *  @return void
     */
    void run( SessionContext context );

    /**
     *  Schedules a frame to be transmitted through the physical layer the next time
     *  the TX queues are processed.
     *
     *  @param[in]  frame       The frame to be transmitted
     *  @return Chimera::Status_t
     */
    Chimera::Status_t enqueueFrame( const Frame &frame );

    /**
     *  Returns the next frame, if available, from the RX queue
     *
     *  @param[out] frame       The frame to dump data into
     *  @return Chimera::Status_t
     */
    Chimera::Status_t dequeueFrame( Frame &frame );

    /**
     *  Register a callback to be invoked upon some event that occurs during
     *  the service processing.
     *
     *  @param[in]  id          Which event to register against
     *  @param[in]  func        The function to register
     *  @return Chimera::Status_t
     */
    Chimera::Status_t registerCallback( const CallbackId id, CBFunction &func );

  protected:
    /**
     *  Initialize the service to prepare it for first time execution
     *  @return Chimera::Status_t
     */
    Chimera::Status_t initialize();

    /**
     *  Internal unhandled callback function should the user not register anything
     *  for a particular callback id.
     *
     *  @param[in]  id          The callback ID that was not handled
     *  @return void
     */
    void unhandledCallback( const size_t id );

  private:
    CBVectors mCBRegistry;                     /**< Callback service vectors */
    FrameQueue<TX_QUEUE_ELEMENTS> mTXQueue;    /**< Queue for data destined for the physical layer */
    FrameQueue<RX_QUEUE_ELEMENTS> mRXQueue;    /**< Queue for data coming from the physical layer */
    Chimera::Threading::RecursiveMutex mMutex; /**< Thread safety lock */

    etl::function_mp<Service, size_t, &Service::unhandledCallback> mUHCFunction;
  };
}    // namespace Ripple::DATALINK

#endif /* !RIPPLE_DATA_LINK_THREAD_HPP */
