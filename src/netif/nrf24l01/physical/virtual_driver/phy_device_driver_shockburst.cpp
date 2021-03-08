/********************************************************************************
 *  File Name:
 *    phy_device_driver_shockburst.cpp
 *
 *  Description:
 *    Shockburst driver layer for the virtual NRF24
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Aurora Includes */
#include <Aurora/logging>

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>

namespace Ripple::NetIf::NRF24::Physical::ShockBurst
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Chimera::Status_t waitForACK( Handle &handle, const size_t timeout )
  {
    using namespace Aurora::Logging;

    /*-------------------------------------------------
    Receive the message
    -------------------------------------------------*/
    zmq::message_t ackMsg;
    size_t start_time = Chimera::millis();

    bool recv_ok = false;
    while( ( Chimera::millis() - start_time ) < timeout )
    {
      auto bytes = handle.netCfg->rxPipes[ 0 ].recv( ackMsg, zmq::recv_flags::dontwait );
      if ( bytes == sizeof( DataLink::PackedFrame ) )
      {
        recv_ok = true;
        break;
      }

      Chimera::Thread::this_thread::yield();
    }

    if( !recv_ok )
    {
      return Chimera::Status::TIMEOUT;
    }

    /*-------------------------------------------------
    Copy out the frame information
    -------------------------------------------------*/
    DataLink::PackedFrame tmpFrame;
    memcpy( &tmpFrame, ackMsg.data(), sizeof( DataLink::PackedFrame ) );

    /*-------------------------------------------------
    Parse the frame message
    -------------------------------------------------*/
    ShockBurstMsg_t msg = HW_NACK_MESSAGE;
    if( tmpFrame.control.dataLength == sizeof( ShockBurstMsg_t))
    {
      memcpy( &msg, tmpFrame.userData, sizeof( msg ) );
    }

    switch ( msg )
    {
      case HW_ACK_MESSAGE:
        getRootSink()->flog( Level::LVL_DEBUG, "ACK\r\n" );
        return Chimera::Status::OK;
        break;

      case HW_NACK_MESSAGE:
      default:
        getRootSink()->flog( Level::LVL_DEBUG, "NACK\r\n" );
        return Chimera::Status::FAIL;
        break;
    }
  }


  Chimera::Status_t sendACK( Handle &handle, const PipeNumber pipe )
  {
    /*-------------------------------------------------
    Build the message to send
    -------------------------------------------------*/
    DataLink::Frame frame;
    DataLink::FrameBuffer buffer;

    // Eventually send more intelligent ACK messages.
    //  - Which message is being ACK'd
    //  - Message type
    frameFactory( frame, FrameType::ACK_FRAME );
    frame.pack( buffer );

    /*-------------------------------------------------
    Assume that at this point everything is set up
    -------------------------------------------------*/
    zmq::message_t topic( TOPIC_SHOCKBURST.data(), TOPIC_SHOCKBURST.size() );
    zmq::message_t message( buffer.data(), buffer.size() );
    handle.netCfg->txPipes[ pipe ].send( topic, zmq::send_flags::sndmore );
    handle.netCfg->txPipes[ pipe ].send( message, zmq::send_flags::dontwait );
  }


  void fifoMessagePump( ZMQConfig *const cfg )
  {
    using namespace Aurora::Logging;

    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if( !cfg )
    {
      getRootSink()->flog( Level::LVL_DEBUG, "Bad network config for message pump\r\n" );
      RT_HARD_ASSERT( cfg );
    }
    else
    {
      getRootSink()->flog( Level::LVL_DEBUG, "Starting ShockBurst message pump\r\n" );
    }

    /*-------------------------------------------------
    Perform the message pump
    -------------------------------------------------*/
    while( !cfg->killMessagePump )
    {
      for( size_t pipe = PipeNumber::PIPE_NUM_1; pipe <= PipeNumber::PIPE_NUM_5; pipe++ )
      {
        zmq::message_t rxMsg;
        auto bytes = cfg->rxPipes[ pipe ].recv( rxMsg, zmq::recv_flags::dontwait );

        if ( ( bytes == sizeof( DataLink::PackedFrame ) ) && ( bytes == rxMsg.size() ))
        {
          /*-------------------------------------------------
          Construct the queue entry
          -------------------------------------------------*/
          HWFifoType qEntry;
          qEntry.rxPipe = static_cast<PipeNumber>( pipe );
          memcpy( qEntry.payload.data(), rxMsg.data(), rxMsg.size() );

          /*-------------------------------------------------
          Push the data into the queue
          -------------------------------------------------*/
          std::lock_guard<std::recursive_mutex>( cfg->lock );
          if( !cfg->fifo.full() )
          {
            cfg->fifo.push( qEntry );
          }
          else
          {
            getRootSink()->flog( Level::LVL_ERROR, "ShockBurst dropped packet due to RX queue full\r\n" );
          }
        }
      }

      Chimera::delayMilliseconds( 5 );
    }

    getRootSink()->flog( Level::LVL_INFO, "ShockBurst msg pump kill signal set. Terminating pump.\r\n" );
  }


  void frameFactory( DataLink::Frame &frame, FrameType type )
  {
    ShockBurstMsg_t data;

    switch( type )
    {
      case FrameType::ACK_FRAME:
        data = HW_ACK_MESSAGE;
        break;

      case FrameType::NACK_FRAME:
        data = HW_NACK_MESSAGE;
        break;

      default:
        return;
        break;
    };

    frame.writeUserData( &data, sizeof( data ) );
  }

}    // namespace Ripple::NetIf::NRF24::Physical::ShockBurst
