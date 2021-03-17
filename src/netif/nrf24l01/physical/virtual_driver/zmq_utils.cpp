/********************************************************************************
 *  File Name:
 *    zmq_utils.cpp
 *
 *  Description:
 *    Implementation of ZMQ utilities
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#if defined( SIMULATOR )

/* ZMQ Includes */
#include <zmq.hpp>

/* Ripple Includes */
#include <Ripple/src/netif/nrf24l01/physical/virtual_driver/zmq_utils.hpp>

namespace Ripple::NetIf::NRF24::Physical
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static const std::string zmq_com_type = "ipc://";

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  ZMQRegistry::ZMQRegistry()
  {

  }


  ZMQRegistry::~ZMQRegistry()
  {

  }


  void ZMQRegistry::create( const MACAddress mac )
  {

  }


  void ZMQRegistry::destroy( const MACAddress mac )
  {

  }


  void ZMQRegistry::open( const MACAddress mac )
  {
    open/close in pairs


    /*-------------------------------------------------
    Design notes: Publishers connect to subscribers
    that are bound to an address.
    -------------------------------------------------*/
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Create the path for IPC communication
    -------------------------------------------------*/
    std::filesystem::path ipcPath = "/tmp/ripple_ipc/rx/" + std::to_string( publish_to_mac ) + ".ipc";
    if( !ensureIPCFileExists( ipcPath ) )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Could not open TX pipe. Failed to create IPC path %s\r\n", ipcPath.c_str() );
      return;
    }

    /*-------------------------------------------------
    Check to see if the pipe is already open
    -------------------------------------------------*/
    std::string ep = zmq_com_type + ipcPath.string();
    if( handle.netCfg->txEndpoints[ pipe ] == ep )
    {
      return;
    }
    else if( handle.netCfg->txEndpoints[ pipe ] != "" )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Cannot open TX pipe. Previous pipe was not closed!\r\n" );
      return;
    }

    /*-------------------------------------------------
    Open the TX pipe. Only pipe 0 is used for dynamic
    data transfer to other nodes.
    -------------------------------------------------*/
    handle.netCfg->txPipes[ pipe ].bind( ep );
    getRootSink()->flog( Level::LVL_DEBUG, "Opened TX pipe %d to MAC %#010x on ZMQ Endpoint: %s\r\n", pipe, publish_to_mac, ep.c_str() );
    Chimera::delayMilliseconds( 5 );

    /*-------------------------------------------------
    Store the endpoint for future operations
    -------------------------------------------------*/
    handle.netCfg->txEndpoints[ pipe ] = ep;







    /*-------------------------------------------------
    Design notes: Subscribers bind to a stable address
    which publishers then connect with to send messages
    -------------------------------------------------*/
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Create the path for IPC communication
    -------------------------------------------------*/
    std::filesystem::path ipcPath = "/tmp/ripple_ipc/rx/" + std::to_string( receive_on_mac ) + ".ipc";
    if( !ensureIPCFileExists( ipcPath ) )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Could not open RX pipe. Failed to create IPC path %s\r\n", ipcPath.c_str() );
      return;
    }

    /*-------------------------------------------------
    Check to see if the pipe is already open
    -------------------------------------------------*/
    std::string ep = zmq_com_type + ipcPath.string();
    if( handle.netCfg->rxEndpoints[ pipe ] == ep )
    {
      return;
    }
    else if( handle.netCfg->rxEndpoints[ pipe ] != "" )
    {
      getRootSink()->flog( Level::LVL_ERROR, "Cannot open RX pipe. Previous pipe was not closed!\r\n" );
      return;
    }

    /*-------------------------------------------------
    Open the rx pipe
    -------------------------------------------------*/
    handle.netCfg->rxPipes[ pipe ].connect( ep );
    getRootSink()->flog( Level::LVL_DEBUG, "Opened RX pipe %d to MAC %#010x on ZMQ Endpoint: %s\r\n", pipe, receive_on_mac, ep.c_str() );
    Chimera::delayMilliseconds( 5 );

    /*-------------------------------------------------
    Store the endpoint for future operations
    -------------------------------------------------*/
    handle.netCfg->rxEndpoints[ pipe ] = ep;
  }


  void ZMQRegistry::close( const MACAddress mac )
  {
    using namespace Aurora::Logging;
    std::lock_guard<std::recursive_mutex> lk( handle.netCfg->lock );

    /*-------------------------------------------------
    Nothing to do if the pipe is already closed
    -------------------------------------------------*/
    if( handle.netCfg->txEndpoints[ pipe ] == "" )
    {
      return;
    }

    /*-------------------------------------------------
    Disconnect from the remote pipe
    -------------------------------------------------*/
    handle.netCfg->txPipes[ pipe ].disconnect( handle.netCfg->txEndpoints[ pipe ] );
    getRootSink()->flog( Level::LVL_DEBUG, "Disconnect TX pipe from ZMQ Endpoint: %s\r\n",
                         handle.netCfg->txEndpoints[ pipe ].c_str() );

    /*-------------------------------------------------
    Reset the cached endpoint information
    -------------------------------------------------*/
    handle.netCfg->txEndpoints[ pipe ] = "";
  }


  zmq::socket_ref ZMQRegistry::getSender( const MACAddress mac )
  {

  }


  zmq::socket_ref ZMQRegistry::getReceiver( const MACAddress mac )
  {

  }


  bool ZMQRegistry::ensureIPCFileExists( const std::filesystem::path &path )
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

}  // namespace

#endif  /* SIMULATOR */
