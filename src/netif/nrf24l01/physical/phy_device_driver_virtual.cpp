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
#include <mutex>

/* Network Includes */
#include <zmq.hpp>

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>

/* Aurora Includes */
#include <Aurora/logging>

namespace Ripple::NetIf::NRF24::Physical
{
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
    Initialize the TX pipe
    -------------------------------------------------*/
    handle.netCfg->txPipe = zmq::socket_t( handle.netCfg->context, zmq::socket_type::push );

    /*-------------------------------------------------
    Initialize the RX pipes
    -------------------------------------------------*/
    for( size_t pipe = 0; pipe < ARRAY_COUNT( ZMQConfig::rxPipes ); pipe++ )
    {
      handle.netCfg->rxPipes[ pipe ] = zmq::socket_t( handle.netCfg->context, zmq::socket_type::pull );
    }

    return Chimera::Status::OK;
  }


  Chimera::Status_t closeDevice( Handle &handle )
  {
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Destroy the TX pipe
    -------------------------------------------------*/
    handle.netCfg->txPipe.close();

    /*-------------------------------------------------
    Destroy all RX pipes
    -------------------------------------------------*/
    for( size_t pipe = 0; pipe < ARRAY_COUNT( ZMQConfig::rxPipes ); pipe++ )
    {
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
    /*-------------------------------------------------
    Nothing to flush
    -------------------------------------------------*/
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
    Open the TX pipe
    -------------------------------------------------*/
    std::string ep = "ipc:///ripple/tx_endpoint/" + std::to_string( address );
    handle.netCfg->txPipe.connect( ep );
    getRootSink()->flog( Level::LVL_DEBUG, "Connect TX pipe to MAC %#016x on ZMQ Endpoint: %s\r\n", address, ep.c_str() );

    /*-------------------------------------------------
    Store the endpoint for future operations
    -------------------------------------------------*/
    handle.netCfg->txEndpoint = ep;
    return Chimera::Status::OK;
  }


  Chimera::Status_t closeWritePipe( Handle &handle )
  {
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Disconnect from the remote pipe
    -------------------------------------------------*/
    handle.netCfg->txPipe.disconnect( handle.netCfg->txEndpoint );
    getRootSink()->flog( Level::LVL_DEBUG, "Disconnect TX pipe from ZMQ Endpoint: %s\r\n", handle.netCfg->txEndpoint.c_str() );

    /*-------------------------------------------------
    Reset the cached endpoint information
    -------------------------------------------------*/
    handle.netCfg->txEndpoint = "";
    return Chimera::Status::OK;
  }


  Chimera::Status_t openReadPipe( Handle &handle, const PipeNumber pipe, const MACAddress address )
  {
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Open an RX pipe and listen for data
    -------------------------------------------------*/
    std::string ep = "ipc:///ripple/rx_endpoint/" + std::to_string( address );
    handle.netCfg->rxPipes[ pipe ].bind( ep );
    getRootSink()->flog( Level::LVL_DEBUG, "Open RX pipe on MAC %#016x with ZMQ Endpoint: %s\r\n", address, ep.c_str() );

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
    Disconnect the pipe
    -------------------------------------------------*/
    handle.netCfg->rxPipes[ pipe ].disconnect( handle.netCfg->rxEndpoints[ pipe ] );
    getRootSink()->flog( Level::LVL_DEBUG, "Closed RX pipe on ZMQ Endpoint: %s\r\n", handle.netCfg->rxEndpoints[ pipe ].c_str() );

    /*-------------------------------------------------
    Reset the cached endpoint information
    -------------------------------------------------*/
    handle.netCfg->rxEndpoints[ pipe ] = "";
    return Chimera::Status::OK;
  }


  Chimera::Status_t readPayload( Handle &handle, void *const buffer, const size_t length )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t writePayload( Handle &handle, const void *const buffer, const size_t length, const PayloadType type )
  {
    return Chimera::Status::OK;
  }


  Chimera::Status_t stageAckPayload( Handle &handle, const PipeNumber pipe, const void *const buffer, size_t length )
  {
    return Chimera::Status::OK;
  }


  /*-------------------------------------------------------------------------------
  Data Setters/Getters
  -------------------------------------------------------------------------------*/
  void readAllRegisters( Handle &handle )
  {
  }


  Reg8_t getStatusRegister( Handle &handle )
  {
    return 0;
  }


  bool rxFifoFull( Handle &handle )
  {
    return false;
  }


  bool rxFifoEmpty( Handle &handle )
  {
    return true;
  }


  bool txFifoFull( Handle &handle )
  {
    return false;
  }


  bool txFifoEmpty( Handle &handle )
  {
    return true;
  }


  Chimera::Status_t setRFPower( Handle &handle, const RFPower level )
  {
    return Chimera::Status::OK;
  }


  RFPower getRFPower( Handle &handle )
  {
    return RFPower::PA_LVL_3;
  }


  Chimera::Status_t setDataRate( Handle &handle, const DataRate speed )
  {
    return Chimera::Status::OK;
  }


  DataRate getDataRate( Handle &handle )
  {
    return DataRate::DR_1MBPS;
  }


  Chimera::Status_t setRetries( Handle &handle, const AutoRetransmitDelay delay, const size_t count )
  {
    return Chimera::Status::OK;
  }


  AutoRetransmitDelay getRTXDelay( Handle &handle )
  {
    return AutoRetransmitDelay::ART_DELAY_2000uS;
  }


  AutoRetransmitCount getRTXCount( Handle &handle )
  {
    return AutoRetransmitCount::ART_COUNT_15;
  }


  Chimera::Status_t setRFChannel( Handle &handle, const size_t channel )
  {
    return Chimera::Status::OK;
  }


  size_t getRFChannel( Handle &handle )
  {
    return 0;
  }


  Chimera::Status_t setISRMasks( Handle &handle, const uint8_t msk )
  {
    return Chimera::Status::OK;
  }


  uint8_t getISRMasks( Handle &handle )
  {
    return 0;
  }


  Chimera::Status_t clrISREvent( Handle &handle, const bfISRMask msk )
  {
    return Chimera::Status::OK;
  }


  uint8_t getISREvent( Handle &handle )
  {
    return 0;
  }


  Chimera::Status_t setAddressWidth( Handle &handle, const AddressWidth width )
  {
    return Chimera::Status::OK;
  }


  AddressWidth getAddressWidth( Handle &handle )
  {
    return AddressWidth::AW_5Byte;
  }


  MACAddress getRXPipeAddress( Handle &handle, const PipeNumber pipe )
  {
    return 0;
  }


  Chimera::Status_t setCRCLength( Handle &handle, const CRCLength length )
  {
    return Chimera::Status::OK;
  }


  CRCLength getCRCLength( Handle &handle )
  {
    return CRCLength::CRC_16;
  }


  Chimera::Status_t setStaticPayloadSize( Handle &handle, const size_t size, const PipeNumber pipe )
  {
    return Chimera::Status::OK;
  }


  size_t getStaticPayloadSize( Handle &handle, const PipeNumber pipe )
  {
    return 0;
  }


  PipeNumber getAvailablePayloadPipe( Handle &handle )
  {
    return PipeNumber::PIPE_NUM_0;
  }


  size_t getAvailablePayloadSize( Handle &handle, const PipeNumber pipe )
  {
    return 0;
  }

}    // namespace Ripple::Physical

#endif /* SIMULATOR */
