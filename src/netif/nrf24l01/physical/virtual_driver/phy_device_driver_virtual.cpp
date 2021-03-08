/********************************************************************************
 *  File Name:
 *    phy_device_driver_virtual.cpp
 *
 *  Description:
 *    Virtual device driver used in simulation
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#if defined( SIMULATOR )
/* STL Includes */
#include <fstream>
#include <mutex>
#include <filesystem>

/* Network Includes */
#include <zmq.hpp>

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>
#include <Ripple/src/netif/nrf24l01/datalink/data_link_frame.hpp>

/* Aurora Includes */
#include <Aurora/logging>

/* Chimera Includes */
#include <Chimera/thread>

namespace Ripple::NetIf::NRF24::Physical
{
  /*-------------------------------------------------------------------------------
  Static Functions
  -------------------------------------------------------------------------------*/
  static bool ensureIPCFileExists( const std::filesystem::path &path )
  {
    /*-------------------------------------------------
    Already exists? No problem
    -------------------------------------------------*/
    if( std::filesystem::exists( path ) )
    {
      return true;
    }

    /*-------------------------------------------------
    Create the file if not
    -------------------------------------------------*/
    std::filesystem::create_directories( path.parent_path() );
    std::ofstream file;
    file.open( path.string(), std::ios::out );
    file.close();

    /*-------------------------------------------------
    Check one more time...
    -------------------------------------------------*/
    return std::filesystem::exists( path );
  }


  /*-------------------------------------------------------------------------------
  Open/Close Functions
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openDevice( const DeviceConfig &cfg, Handle &handle )
  {
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Store the common configuration items
    -------------------------------------------------*/
    handle.opened = true;
    handle.cfg = cfg;

    /*-------------------------------------------------
    Create the "device" context with a thread pool
    -------------------------------------------------*/
    handle.netCfg->context = zmq::context_t( MAX_NUM_PIPES );

    /*-------------------------------------------------
    Initialize the TX/RX pipes. All TX sockets for
    pipes 1-5 are solely for internally mimicking the
    ShockBurst behavior of the device.
    -------------------------------------------------*/
    for( size_t pipe = 0; pipe < ARRAY_COUNT( ZMQConfig::rxPipes ); pipe++ )
    {
      handle.netCfg->txPipes[ pipe ] = zmq::socket_t( handle.netCfg->context, zmq::socket_type::push );
      handle.netCfg->rxPipes[ pipe ] = zmq::socket_t( handle.netCfg->context, zmq::socket_type::pull );
    }

    /*-------------------------------------------------
    Start the message pump
    -------------------------------------------------*/
    handle.messagePump = new std::thread( ShockBurst::fifoMessagePump, handle.netCfg );

    return Chimera::Status::OK;
  }


  Chimera::Status_t closeDevice( Handle &handle )
  {
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Destroy all pipes
    -------------------------------------------------*/
    for( size_t pipe = 0; pipe < ARRAY_COUNT( ZMQConfig::rxPipes ); pipe++ )
    {
      handle.netCfg->txPipes[ pipe ].close();
      handle.netCfg->rxPipes[ pipe ].close();
    }

    /*-------------------------------------------------
    Destroy the "device" context
    -------------------------------------------------*/
    handle.netCfg->context.close();

    return Chimera::Status::OK;
  }


  /*-------------------------------------------------------------------------------
  Device Commands
  -------------------------------------------------------------------------------*/
  Chimera::Status_t resetRegisterDefaults( Handle &handle )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t flushTX( Handle &handle )
  {
    /*-------------------------------------------------
    Nothing to flush
    -------------------------------------------------*/
    return Chimera::Status::OK;
  }


  Chimera::Status_t flushRX( Handle &handle )
  {
    std::lock_guard<std::recursive_mutex>( handle.netCfg->lock );
    handle.netCfg->fifo.clear();

    return Chimera::Status::OK;
  }


  Chimera::Status_t startListening( Handle &handle )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t stopListening( Handle &handle )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t pauseListening( Handle &handle )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t resumeListening( Handle &handle )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleAckPayloads( Handle &handle, const bool state )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleDynamicPayloads( Handle &handle, const PipeNumber pipe, const bool state )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleDynamicAck( Handle &handle, const bool state )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleAutoAck( Handle &handle, const bool state, const PipeNumber pipe )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t toggleFeatures( Handle &handle, const bool state )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t togglePower( Handle &handle, const bool state )
  {
    return Chimera::Status::OK;
  }


  /*-------------------------------------------------------------------------------
  Data Pipe Operations
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openWritePipe( Handle &handle, const MACAddress address )
  {
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Create the path for IPC communication
    -------------------------------------------------*/
    std::filesystem::path ipcPath = "/tmp/ripple_ipc/rx/" + std::to_string( address ) + ".ipc";
    if( !ensureIPCFileExists( ipcPath ) )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Could not open TX pipe. Failed to create IPC path %s\r\n", ipcPath.c_str() );
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Check to see if the pipe is already open
    -------------------------------------------------*/
    //std::string ep = "ipc://" + ipcPath.string();
    std::string ep = "tcp://127.0.0.1:1234";

    if( handle.netCfg->txEndpoints[ 0 ] == ep )
    {
      return Chimera::Status::OK;
    }
    else if( handle.netCfg->txEndpoints[ 0 ] != "" )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Cannot open TX pipe. Previous pipe was not closed!\r\n" );
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Open the TX pipe. Only pipe 0 is used for dynamic
    data transfer to other nodes.
    -------------------------------------------------*/
    handle.netCfg->txPipes[ 0 ].bind( ep );
    getRootSink()->flog( Level::LVL_DEBUG, "Opened TX pipe 0 to MAC %#010x on ZMQ Endpoint: %s\r\n", address, ep.c_str() );

    /*-------------------------------------------------
    Store the endpoint for future operations
    -------------------------------------------------*/
    handle.netCfg->txEndpoints[ 0 ] = ep;
    return Chimera::Status::OK;
  }


  Chimera::Status_t closeWritePipe( Handle &handle )
  {
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Nothing to do if the pipe is already closed
    -------------------------------------------------*/
    if( handle.netCfg->txEndpoints[ 0 ] == "" )
    {
      return Chimera::Status::OK;
    }

    /*-------------------------------------------------
    Disconnect from the remote pipe
    -------------------------------------------------*/
    handle.netCfg->txPipes[0 ].disconnect( handle.netCfg->txEndpoints[ 0 ] );
    getRootSink()->flog( Level::LVL_DEBUG, "Disconnect TX pipe from ZMQ Endpoint: %s\r\n",
                         handle.netCfg->txEndpoints[ 0 ].c_str() );

    /*-------------------------------------------------
    Reset the cached endpoint information
    -------------------------------------------------*/
    handle.netCfg->txEndpoints[ 0 ] = "";
    return Chimera::Status::OK;
  }


  Chimera::Status_t openReadPipe( Handle &handle, const PipeNumber pipe, const MACAddress address )
  {
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Create the path for IPC communication
    -------------------------------------------------*/
    std::filesystem::path ipcPath = "/tmp/ripple_ipc/rx/" + std::to_string( address ) + ".ipc";
    if( !ensureIPCFileExists( ipcPath ) )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Could not open RX pipe. Failed to create IPC path %s\r\n", ipcPath.c_str() );
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Check to see if the pipe is already open
    -------------------------------------------------*/
    std::string ep = "ipc://" + ipcPath.string();

    if( handle.netCfg->rxEndpoints[ pipe ] == ep )
    {
      return Chimera::Status::OK;
    }
    else if( handle.netCfg->rxEndpoints[ pipe ] != "" )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Cannot open RX pipe. Previous pipe was not closed!\r\n", pipe );
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Open the RX pipe. Subscribe to all messages sent
    with the "packet" topic.
    -------------------------------------------------*/
    handle.netCfg->rxPipes[ pipe ].connect( ep );
    //handle.netCfg->rxPipes[ pipe ].setsockopt( ZMQ_SUBSCRIBE, TOPIC_DATA.data(), 0 );
    //handle.netCfg->rxPipes[ pipe ].setsockopt( ZMQ_SUBSCRIBE, TOPIC_SHOCKBURST.data(), 0 );

    getRootSink()->flog( Level::LVL_DEBUG, "Opened RX pipe %d to MAC %#010x on ZMQ Endpoint: %s\r\n", pipe, address, ep.c_str() );

    /*-------------------------------------------------
    Open the TX pipe for internal shockburst use only
    -------------------------------------------------*/
    // if( pipe != 0 )
    // {
    //   handle.netCfg->txPipes[ pipe ].connect( ep );
    //   handle.netCfg->txEndpoints[ pipe ] = ep;
    // }

    /*-------------------------------------------------
    Store the endpoint for future operations
    -------------------------------------------------*/
    handle.netCfg->rxEndpoints[ pipe ] = ep;
    return Chimera::Status::OK;
  }


  Chimera::Status_t closeReadPipe( Handle &handle, const PipeNumber pipe )
  {
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Nothing to do if the pipe is already closed
    -------------------------------------------------*/
    if( handle.netCfg->rxEndpoints[ pipe ] == "" )
    {
      return Chimera::Status::OK;
    }

    /*-------------------------------------------------
    Disconnect the pipe
    -------------------------------------------------*/
    handle.netCfg->rxPipes[ pipe ].disconnect( handle.netCfg->rxEndpoints[ pipe ] );
    getRootSink()->flog( Level::LVL_DEBUG, "Closed RX pipe on ZMQ Endpoint: %s\r\n", handle.netCfg->rxEndpoints[ pipe ].c_str() );

    /*-------------------------------------------------
    Open the TX pipe for internal shockburst use only
    -------------------------------------------------*/
    if( pipe != 0 )
    {
      handle.netCfg->txPipes[ pipe ].disconnect( handle.netCfg->txEndpoints[ pipe ] );
      handle.netCfg->txEndpoints[ pipe ] = "";
    }

    /*-------------------------------------------------
    Reset the cached endpoint information
    -------------------------------------------------*/
    handle.netCfg->rxEndpoints[ pipe ] = "";
    return Chimera::Status::OK;
  }


  Chimera::Status_t readPayload( Handle &handle, void *const buffer, const size_t length )
  {
    using namespace Aurora::Logging;

    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !buffer || !length || ( length > sizeof( HWFifoType::payload ) ) )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Grab the next message
    -------------------------------------------------*/
    std::lock_guard<std::recursive_mutex>( handle.netCfg->lock );
    if( !handle.netCfg->fifo.empty() )
    {
      HWFifoType nextMessage;
      handle.netCfg->fifo.pop_into( nextMessage );
      memcpy( buffer, nextMessage.payload.data(), length );

      /*-------------------------------------------------
      Temporarily unpack the frame and see if it needs
      a shockburst ACK.
      -------------------------------------------------*/
      DataLink::Frame frame;
      frame.unpack( nextMessage.payload );

      if( frame.wireData.control.requireACK )
      {
        ShockBurst::sendACK( handle, nextMessage.rxPipe );
      }

      return Chimera::Status::OK;
    }

    return Chimera::Status::EMPTY;
  }


  Chimera::Status_t writePayload( Handle &handle, const void *const buffer, const size_t length, const PayloadType type )
  {
    using namespace Aurora::Logging;
    auto result = Chimera::Status::OK;

    /*-------------------------------------------------
    Transmit the data to the destination
    -------------------------------------------------*/
    //zmq::message_t topic( TOPIC_DATA.data(), TOPIC_DATA.size() );
    zmq::message_t data( buffer, length );

    //handle.netCfg->txPipes[ 0 ].send( topic, zmq::send_flags::sndmore );
    auto byteSent = handle.netCfg->txPipes[ 0 ].send( data, zmq::send_flags::none );

    if( byteSent != length )
    {
      result = Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Optionally wait for a simulated ShockBurst ACK
    -------------------------------------------------*/
    if( type == PayloadType::PAYLOAD_REQUIRES_ACK )
    {
      result = ShockBurst::waitForACK( handle, 5 * Chimera::Thread::TIMEOUT_1S );
    }

    /*-------------------------------------------------
    Assign success/fail to packet TX
    -------------------------------------------------*/
    if( result == Chimera::Status::OK )
    {
      handle.cfg.hwISREvent |= bfISRMask::ISR_MSK_TX_DS;
    }
    else
    {
      handle.cfg.hwISREvent &= ~bfISRMask::ISR_MSK_TX_DS;
      handle.cfg.hwISREvent |= bfISRMask::ISR_MSK_MAX_RT;
    }

    return result;
  }


  Chimera::Status_t stageAckPayload( Handle &handle, const PipeNumber pipe, const void *const buffer, size_t length )
  {
    using namespace Chimera::Thread;

    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !buffer || !length || !( pipe < PipeNumber::PIPE_NUM_ALL ) )
    {
      return Chimera::Status::FAIL;
    }

    /*-------------------------------------------------
    Copy in the data to the buffer
    -------------------------------------------------*/
    auto usrBuffer = reinterpret_cast<const uint8_t *const>( buffer );

    handle.netCfg->ackPayloads[ pipe ].fill( 0 );
    std::copy( usrBuffer, usrBuffer + length, handle.netCfg->ackPayloads[ pipe ].begin() );

    return Chimera::Status::OK;
  }


  /*-------------------------------------------------------------------------------
  Data Setters/Getters
  -------------------------------------------------------------------------------*/
  void readAllRegisters( Handle &handle )
  {
    RT_HARD_ASSERT( false );
  }


  Reg8_t getStatusRegister( Handle &handle )
  {
    RT_HARD_ASSERT( false );
    return 0;
  }


  bool rxFifoFull( Handle &handle )
  {
    std::lock_guard<std::recursive_mutex>( handle.netCfg->lock );
    return handle.netCfg->fifo.full();
  }


  bool rxFifoEmpty( Handle &handle )
  {
    std::lock_guard<std::recursive_mutex>( handle.netCfg->lock );
    return handle.netCfg->fifo.empty();
  }


  bool txFifoFull( Handle &handle )
  {
    // Always not full due to immediate TX in sim
    return false;
  }


  bool txFifoEmpty( Handle &handle )
  {
    // Always empty due to immediate TX in sim
    return true;
  }


  Chimera::Status_t setRFPower( Handle &handle, const RFPower level )
  {
    handle.cfg.hwPowerAmplitude = level;
    return Chimera::Status::OK;
  }


  RFPower getRFPower( Handle &handle )
  {
    return handle.cfg.hwPowerAmplitude;
  }


  Chimera::Status_t setDataRate( Handle &handle, const DataRate speed )
  {
    handle.cfg.hwDataRate = speed;
    return Chimera::Status::OK;
  }


  DataRate getDataRate( Handle &handle )
  {
    return handle.cfg.hwDataRate;
  }


  Chimera::Status_t setRetries( Handle &handle, const AutoRetransmitDelay delay, const size_t count )
  {
    handle.cfg.hwRTXDelay = delay;
    handle.cfg.hwRTXCount = static_cast<AutoRetransmitCount>( count );
    return Chimera::Status::OK;
  }


  AutoRetransmitDelay getRTXDelay( Handle &handle )
  {
    return handle.cfg.hwRTXDelay;
  }


  AutoRetransmitCount getRTXCount( Handle &handle )
  {
    return handle.cfg.hwRTXCount;
  }


  Chimera::Status_t setRFChannel( Handle &handle, const size_t channel )
  {
    handle.cfg.hwRFChannel = channel;
    return Chimera::Status::OK;
  }


  size_t getRFChannel( Handle &handle )
  {
    return handle.cfg.hwRFChannel;
  }


  Chimera::Status_t setISRMasks( Handle &handle, const uint8_t msk )
  {
    /*-------------------------------------------------
    The last packet failed to transmit correctly
    -------------------------------------------------*/
    if ( msk & Physical::bfISRMask::ISR_MSK_MAX_RT )
    {
      handle.cfg.hwISRMask |= bfISRMask::ISR_MSK_MAX_RT;
    }
    else
    {
      handle.cfg.hwISRMask &= ~bfISRMask::ISR_MSK_MAX_RT;
    }

    /*-------------------------------------------------
    A new packet was received
    -------------------------------------------------*/
    if ( msk & Physical::bfISRMask::ISR_MSK_RX_DR )
    {
      handle.cfg.hwISRMask |= bfISRMask::ISR_MSK_RX_DR;
    }
    else
    {
      handle.cfg.hwISRMask &= ~bfISRMask::ISR_MSK_RX_DR;
    }

    /*-------------------------------------------------
    A packet successfully transmitted
    -------------------------------------------------*/
    if ( msk & Physical::bfISRMask::ISR_MSK_TX_DS )
    {
      handle.cfg.hwISRMask |= bfISRMask::ISR_MSK_TX_DS;
    }
    else
    {
      handle.cfg.hwISRMask &= ~bfISRMask::ISR_MSK_TX_DS;
    }

    return Chimera::Status::OK;
  }


  uint8_t getISRMasks( Handle &handle )
  {
    return handle.cfg.hwISRMask;
  }


  Chimera::Status_t clrISREvent( Handle &handle, const bfISRMask msk )
  {
    /*-------------------------------------------------
    The last packet failed to transmit correctly
    -------------------------------------------------*/
    if ( msk & Physical::bfISRMask::ISR_MSK_MAX_RT )
    {
      handle.cfg.hwISREvent &= ~bfISRMask::ISR_MSK_MAX_RT;
    }

    /*-------------------------------------------------
    A new packet was received
    -------------------------------------------------*/
    if ( msk & Physical::bfISRMask::ISR_MSK_RX_DR )
    {
      handle.cfg.hwISREvent &= ~bfISRMask::ISR_MSK_RX_DR;
    }

    /*-------------------------------------------------
    A packet successfully transmitted
    -------------------------------------------------*/
    if ( msk & Physical::bfISRMask::ISR_MSK_TX_DS )
    {
      handle.cfg.hwISREvent &= ~bfISRMask::ISR_MSK_TX_DS;
    }

    return Chimera::Status::OK;
  }


  uint8_t getISREvent( Handle &handle )
  {
    return handle.cfg.hwISREvent;
  }


  Chimera::Status_t setAddressWidth( Handle &handle, const AddressWidth width )
  {
    handle.cfg.hwAddressWidth = width;
    return Chimera::Status::OK;
  }


  AddressWidth getAddressWidth( Handle &handle )
  {
    return handle.cfg.hwAddressWidth;
  }


  MACAddress getRXPipeAddress( Handle &handle, const PipeNumber pipe )
  {
    RT_HARD_ASSERT( false ); // TODO
    return 0;
  }


  Chimera::Status_t setCRCLength( Handle &handle, const CRCLength length )
  {
    handle.cfg.hwCRCLength = length;
    return Chimera::Status::OK;
  }


  CRCLength getCRCLength( Handle &handle )
  {
    return handle.cfg.hwCRCLength;
  }


  Chimera::Status_t setStaticPayloadSize( Handle &handle, const size_t size, const PipeNumber pipe )
  {
    handle.cfg.hwStaticPayloadWidth = size;
    return Chimera::Status::OK;
  }


  size_t getStaticPayloadSize( Handle &handle, const PipeNumber pipe )
  {
    return handle.cfg.hwStaticPayloadWidth;
  }


  PipeNumber getAvailablePayloadPipe( Handle &handle )
  {
    /*-------------------------------------------------
    Grab the next message
    -------------------------------------------------*/
    std::lock_guard<std::recursive_mutex>( handle.netCfg->lock );
    if( !handle.netCfg->fifo.empty() )
    {
      return handle.netCfg->fifo.front().rxPipe;
    }
    else
    {
      return PipeNumber::PIPE_INVALID;
    }
  }


  size_t getAvailablePayloadSize( Handle &handle, const PipeNumber pipe )
  {
    // Only supported in dynamic mode
    RT_HARD_ASSERT( false );
    return 0;
  }

}    // namespace Ripple::Physical

#endif /* SIMULATOR */
