/********************************************************************************
 *  File Name:
 *    zmq_utils.hpp
 *
 *  Description:
 *    Utilities for managing ZMQ connections
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#if defined( SIMULATOR )

#pragma once
#ifndef RF24_ZMQ_UTILITIES_HPP
#define RF24_ZMQ_UTILITIES_HPP

/* STL Includes */
#include <filesystem>
#include <map>
#include <string>
#include <vector>

/* ZMQ Includes */
#include <zmq.hpp>

/* Ripple Includes */
#include <Ripple/src/netif/nrf24l01/physical/phy_device_types.hpp>

namespace Ripple::NetIf::NRF24::Physical
{
  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct ZMQEndpoint
  {
    MACAddress mac;
    zmq::socket_t txSocket;
    zmq::socket_t rxSocket;
    std::string txURL;
    std::string rxURL;
  };

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Manages connections for a ZMQ context. The idea here is to keep connections
   *  alive until explicitly closed so as to not incur time penalties or missed
   *  packets when switching out ShockBurst destination nodes rapidly.
   */
  class ZMQRegistry
  {
  public:
    ZMQRegistry();
    ~ZMQRegistry();

    void createDevice( const MACAddress mac );

    void destroyDevice( const MACAddress mac );

    void open( const MACAddress mac );

    void close( const MACAddress mac );

    zmq::socket_ref getSender( const MACAddress mac );

    zmq::socket_ref getReceiver( const MACAddress mac );

  protected:

    static bool ensureIPCFileExists( const std::filesystem::path &path );

  private:
    zmq::context_t mZmqContext;
    std::recursive_mutex mLock;
    std::map<MACAddress, ZMQEndpoint> mEndpoints;
  };
}  // namespace RF24::

#endif  /* !RF24_ZMQ_UTILITIES_HPP */
#endif  /* SIMULATOR */
