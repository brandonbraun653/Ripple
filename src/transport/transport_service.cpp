/********************************************************************************
 *  File Name:
 *    transport_service.cpp
 *
 *  Description:
 *    Implements the Transport layer services
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/assert>
#include <Chimera/common>
#include <Chimera/system>
#include <Chimera/thread>

/* Ripple Includes  */
#include <Ripple/transport>
#include <Ripple/network>

namespace Ripple::Transport
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Handle *getHandle( Session::Context session )
  {
    if ( !session )
    {
      return nullptr;
    }
    else
    {
      auto context   = reinterpret_cast<Session::Handle *>( session );
      auto transport = reinterpret_cast<Handle *>( context->transport );

      return transport;
    }
  }

  /*-------------------------------------------------------------------------------
  Service Class Public Methods
  -------------------------------------------------------------------------------*/
  Service::Service() :
      mContext( nullptr ), mUpdateRate( Chimera::Threading::TIMEOUT_50MS ),
      mServiceStarvedThreshold( 2 * Chimera::Threading::TIMEOUT_1MS )
  {
  }


  Service::~Service()
  {
  }


  void Service::run( Session::Context context )
  {
    using namespace Chimera::Threading;

    /*-------------------------------------------------
    Wait for this thread to be told to initialize
    -------------------------------------------------*/
    this_thread::pendTaskMsg( TSK_MSG_WAKEUP );
    mThreadId = this_thread::id();

    /*-------------------------------------------------
    Verify the handles used in the entire DataLink
    service have been registered correctly.
    -------------------------------------------------*/
    if ( !session )
    {
      Chimera::System::softwareReset();
    }
    else
    {
      mContext = context;
    }

    /*-------------------------------------------------
    Initialize the service
    -------------------------------------------------*/
    initMemoryPool();

    /*-------------------------------------------------
    Run the service
    -------------------------------------------------*/
    size_t lastWakeup  = 0;
    size_t pendTime    = mUpdateRate;
    size_t nextWakeup  = 0;
    size_t currentTick = 0;

    while ( 1 )
    {
      /*-------------------------------------------------
      Block this thread until periodic process timeout
      expires or another thread wakes this one up.
      -------------------------------------------------*/
      this_thread::pendTaskMsg( ITCMsg::TSK_MSG_WAKEUP, pendTime );
      lastWakeup = Chimera::millis();
      nextWakeup = lastWakeup + mUpdateRate;


      /*-------------------------------------------------
      Calculate the next time this thread should wake.
      -------------------------------------------------*/
      currentTick = Chimera::millis();
      pendTime    = nextWakeup - currentTick;

      /*-------------------------------------------------
      Notify a listener if the thread is doing too much
      in its time slice or was interrupted by a higher
      priority thread that consumed too much time.

      This callback firing is more of a warning bell then
      a sure-fire sign that there is a problem.
      -------------------------------------------------*/
      if ( ( currentTick >= nextWakeup ) || ( pendTime <= mServiceStarvedThreshold ) )
      {
        mDelegateRegistry.call<CallbackId::CB_SERVICE_OVERRUN>();
      }
    }
  }


  void Service::setUpdateRate( const size_t period )
  {
    this->lock();
    mUpdateRate = period;
    this->unlock();
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


  Chimera::Status_t Service::writeEndpoint( const DataLink::Endpoint ep, const void *const data, const size_t size,
                                            EPCallback &onComplete )
  {
    using namespace DataLink;

    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !( ep < EP_NUM_OPTIONS ) || !data || !size || ( size > MAX_PACKET_SIZE ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    this->lock();

    /*-------------------------------------------------
    Verify there is enough space to buffer the data
    -------------------------------------------------*/
    constexpr size_t maxUserPayload = ARRAY_BYTES( Network::Datagram::data );
    const size_t availUserBytes     = mEPTXData[ ep ].available() * maxUserPayload;

    if ( size > availUserBytes )
    {
      this->unlock();
      mDelegateRegistry.call<CallbackId::CB_EP_NO_MEMORY>();
      return Chimera::Status::MEMORY;
    }

    /*-------------------------------------------------
    Calculate the number of fragments to split into
    -------------------------------------------------*/
    const size_t partial_datagram = ( availUserBytes % maxUserPayload ) ? 1 : 0;
    const size_t numDatagrams     = ( availUserBytes / maxUserPayload ) + partial_datagram;

    /*-------------------------------------------------
    Push fragments into the buffer
    -------------------------------------------------*/
    size_t bytesRemaining = size;
    size_t bytesToCopy = 0;
    size_t bytesWritten = 0;

    for( auto fragment = 0; fragment < numDatagrams; fragment++ )
    {
      /* Protect against underflow */
      RT_HARD_ASSERT( bytesRemaining < MAX_PACKET_SIZE );

      bytesToCopy = std::min<size_t>( bytesRemaining, maxUserPayload );

      /*-------------------------------------------------
      Figure out how many bytes are packed in the message
      -------------------------------------------------*/
      Network::Datagram tmp;

      memcpy( tmp.data, data + bytesWritten, bytesToCopy );

      tmp.source


    };


  }

  /*-------------------------------------------------------------------------------
  Service Class Private Methods
  -------------------------------------------------------------------------------*/
  bool Service::fragmentSortCompare( Network::Datagram &datagram )
  {
  }


  void Service::initMemoryPool()
  {
    /*-------------------------------------------------
    Clear out the pool to be empty
    -------------------------------------------------*/
    mDatagramPool.release_all();

    /*-------------------------------------------------
    Assign the shared memory pool to the transmit and
    receive fragment buffers.
    -------------------------------------------------*/
    for ( auto x = 0; x < DataLink::EP_NUM_OPTIONS; x++ )
    {
      mEPTXData[ x ].set_pool( mDatagramPool );
      mEPRXData[ x ].set_pool( mDatagramPool );
    }
  }

}    // namespace Ripple::Transport
