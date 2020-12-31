/********************************************************************************
 *  File Name:
 *    data_link_service.hpp
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
#include <Ripple/src/datalink/data_link_types.hpp>
#include <Ripple/src/datalink/data_link_arp.hpp>

namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t THREAD_STACK          = STACK_BYTES( 1024 );
  static constexpr std::string_view THREAD_NAME = "heartbeat";


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
  class Service : Chimera::Threading::Lockable
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
     *
     *  @param[in]  session     User session information
     *  @return Chimera::Status_t
     */
    Chimera::Status_t initialize( SessionContext session );

    /**
     *  Initializes the radio with the user configured settings
     *
     *  @param[in]  session     User session information
     *  @return Chimera::Status_t
     */
    Chimera::Status_t powerUpRadio( SessionContext session );

    /**
     *  Invokes the requested callback
     *
     *  @param[in]  id          The callback to invoke
     *  @return void
     */
    void invokeCallback( const CallbackId id );

    /**
     *  Internal unhandled callback function should the user not register anything
     *  for a particular callback id.
     *
     *  @param[in]  data        Data associated with the callback
     *  @return void
     */
    void unhandledCallback( CBData &id );

    /**
     *  Callback that will be invoked when the radio's IRQ pin is asserted. This
     *  method will execute in ISR space, so take care with function execution time.
     *
     *  @param[in]  arg         Unused, but required for callback signature
     *  @return void
     */
    void irqPinAsserted( void *arg );

    /**
     *  Handle the IRQ event when a transmission succeeded
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processTXSuccess( SessionContext context );

    /**
     *  Handle the IRQ event when a transmission failed
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processTXFail( SessionContext context );

    /**
     *  Periodic process to transmit data that has been enqueued with the service
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processTXQueue( SessionContext context );

    /**
     *  Periodic process to read a frame off the radio and enqueues it
     *  for handling by higher layers.
     *
     *  @param[in]  session     User session information
     *  @return void
     */
    void processRXQueue( SessionContext context );

  private:
    volatile bool pendingEvent;                       /**< Signal flag set by an ISR that an event occurred */
    CBVectors mCBRegistry;                            /**< Callback service vectors */
    FrameQueue<TX_QUEUE_ELEMENTS> mTXQueue;           /**< Queue for data destined for the physical layer */
    Chimera::Threading::RecursiveTimedMutex mTXMutex; /**< Thread safety lock */
    FrameQueue<RX_QUEUE_ELEMENTS> mRXQueue;           /**< Queue for data coming from the physical layer */
    Chimera::Threading::RecursiveTimedMutex mRXMutex; /**< Thread safety lock */
    Chimera::Threading::ThreadId mThreadId;           /**< Thread registration ID */

    ARPCache mAddressCache;

    /*-------------------------------------------------
    Fields associated with a TX procedure
    -------------------------------------------------*/
    bool txInProgress;  /**< TX is ongoing and hasn't been ACK'd yet */
    size_t txPacketId;  /**< Which packet number is being processed */
  };
}    // namespace Ripple::DATALINK

#endif /* !RIPPLE_DATA_LINK_THREAD_HPP */
