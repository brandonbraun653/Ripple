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
#include <etl/vector.h>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_memory_config.hpp>
#include <Ripple/src/session/session_types.hpp>
#include <Ripple/src/transport/transport_types.hpp>
#include <Ripple/src/datalink/data_link_types.hpp>

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
  class Service : public Chimera::Threading::Lockable<Service>
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
     *  Writes a number of bytes to an endpoint and invokes a callback when the data is
     *  transfered successfully to the destination. Non-blocking.
     *
     *  @param[in]  ep          The endpoint to write to
     *  @param[in]  data        Data to write
     *  @param[in]  size        How many bytes to write
     *  @param[in]  onComplete  Callback to be invoked when the data is successfully transfered
     *  @return Chimera::Status_t
     */
    Chimera::Status_t writeEndpoint( const DataLink::Endpoint ep, const void *const data, const size_t size, EPCallback &onComplete );

    /**
     *  Reads a number of bytes from an endpoint
     *
     *  @param[in]  ep          The endpoint to read from
     *  @param[out] data        Buffer to write into
     *  @param[in]  size        How many bytes to read out. Data buffer must be able to hold this.
     *  @return Chimera::Status_t
     */
    Chimera::Status_t readEndpoint( const DataLink::Endpoint ep, void *const data, const size_t size );

    /**
     *  Checks the number of bytes available for reading. This will only
     *  succeed if a full packet has been received on an endpoint.
     *
     *  @param[in]  ep          The endpoint to query for sizing
     *  @param[out] bytes       If valid, how many bytes can be read
     *  @return bool            Validity status
     */
    bool bytesAvailable( const DataLink::Endpoint ep, size_t *const bytes );

  private:
    friend Chimera::Threading::Lockable<Service>;


    /*-------------------------------------------------
    Private Functions
    -------------------------------------------------*/
    /**
     *  Comparison function used to sort a list of frag
     */
    bool fragmentSortCompare( Network::Datagram &datagram );

    /**
     *  Initializes the memory pool for datagrams
     *  @return void
     */
    void initMemoryPool();

    /*-------------------------------------------------
    Private Data
    -------------------------------------------------*/
    size_t mUpdateRate;                     /**< Main thread periodic update rate ms */
    size_t mServiceStarvedThreshold;        /**< Thread delay time that indicates the process is starved for processing */
    Chimera::Threading::ThreadId mThreadId; /**< Thread registration ID */
    Session::Context mContext;              /**< User context for the network stack */

    /**
     *  Raw memory pool for storing all Datagram messages that are
     *  incoming or outgoing over the radio link.
     */
    DatagramPool<DATAGRAM_MESSAGE_POOL_SIZE> mDatagramPool;

    /**
     *  Primary storage for fragmented packets, divided up into endpoints. Each
     *  list in the vector shares a single memory pool such that there is no dynamic
     *  allocation, but this does mean that very active endpoints can tie up
     *  resources quickly, leaving few memory blocks for other endpoints. However,
     *  the advantage is that resources are easily reallocated at runtime to the
     *  endpoints with the most amount of data, which is very efficient. The user
     *  does not have to specify the exact amount of memory per endpoint.
     */
    etl::vector<DatagramList, DataLink::EP_NUM_OPTIONS> mEPTXData;
    etl::vector<DatagramList, DataLink::EP_NUM_OPTIONS> mEPRXData;


    /**
     *  Helper for tracking/invoking event callbacks
     */
    etl::delegate_service<CallbackId::CB_NUM_OPTIONS> mDelegateRegistry;
  };

}    // namespace Ripple::Transport

#endif /* !RIPPLE_TRANSPORT_SERVICE_HPP */
