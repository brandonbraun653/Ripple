/********************************************************************************
 *  File Name:
 *    session_types.hpp
 *
 *  Description:
 *    Types and definitions associated with the Session layer
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SESSION_TYPES_HPP
#define RIPPLE_SESSION_TYPES_HPP

/* ETL Includes */
#include <etl/callback_service.h>
#include <etl/delegate.h>

/* Ripple Includes */
#include <Ripple/src/network/network_types.hpp>
#include <Ripple/src/physical/phy_device_types.hpp>
#include <Ripple/src/physical/phy_device_constants.hpp>
#include <Ripple/src/shared/cmn_types.hpp>

namespace Ripple::Session
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Context = void *; /**< Opaque pointer to the Handle */

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  /**
   *  Supported callback identifiers that can be used to register a
   *  function for invocation on an event. Intended to be used by the
   *  Session layer.
   */
  enum CallbackId : uint8_t
  {
    CB_UNHANDLED,         /**< Handler for an unregistered callback event */
    CB_SERVICE_OVERRUN,   /**< The service thread is using nearly all its time allocation */

    CB_NUM_OPTIONS
  };

  enum class SocketType
  {
    STREAM, /**< Views data as a stream of bytes */
    PACKET, /**< Views data as a collection of packets */
  };

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   *  Configuration structure describing how to initialize the radio
   *  and what runtime behaviors it should perform.
   */
  struct RadioConfig
  {
    /*-------------------------------------------------
    Required Properties
    -------------------------------------------------*/
    uint8_t channel;        /**< Radio channel number in range [1, 255] */
    size_t networkBaud;     /**< Desired effective link speed in kbps */
    Network::IPAddress address; /**< Static address of this device */

    /*-------------------------------------------------
    Advanced Properties:
    Defaults will be assigned if left blank
    -------------------------------------------------*/
    struct _Avanced
    {
      bool verifyRegisters;       /**< Optionally verify registers are set correctly at runtime */
      bool staticPayloads;        /**< Optionally enable static payloads (defaults to dynamic) */
      uint8_t staticPayloadSize;  /**< Fixed length of static payloads, if used */
      Physical::MACAddress mac;        /**< MAC Address for the device */
      Physical::AddressWidth macWidth; /**< Number of bytes used in the MAC address */
      Physical::DataRate dataRate;     /**< RF on-air data rate */
      Physical::RFPower rfPower;       /**< RF transmission power */
    } advanced;

    /*-------------------------------------------------
    Public Methods
    -------------------------------------------------*/
    void clear()
    {
      channel     = 0;
      networkBaud = 0;
      address     = 0;

      advanced.mac               = 0;
      advanced.macWidth          = Physical::AddressWidth::AW_INVALID;
      advanced.dataRate          = Physical::DataRate::DR_INVALID;
      advanced.rfPower           = Physical::RFPower::PA_INVALID;
      advanced.verifyRegisters   = false;
      advanced.staticPayloads    = false;
      advanced.staticPayloadSize = static_cast<uint8_t>( Physical::MAX_TX_PAYLOAD_SIZE );
    }
  };


  /**
   *  Session layer handle describing an entire network stack
   */
  struct Handle
  {
    /*-------------------------------------------------
    Opaque handles to the different layer objects
    -------------------------------------------------*/
    void *session;
    void *transport;
    void *network;
    void *datalink;
    void *physical;

    /*-------------------------------------------------
    Configuration Data
    -------------------------------------------------*/
    RadioConfig radioConfig;
  };

  /**
   *  Holds all resources necessary to receive and transmit
   *  data on a socket endpoint.
   */
  struct SocketBuffer
  {
    /**
     *  Type of socket, indicating how the data buffers are managed
     *  internally. Stream sockets are very different from packet sockets.
     */
    SocketType type;
    void * rxBuffer;  /**< Buffer for RX data */
    size_t rxLength;  /**< Number of bytes in the buffer */
    void *txBuffer;   /**< Buffer for TX data */
    size_t txLength;  /**< Number of bytes in the buffer */
  };

  /**
   *  Represents a connection to a remote endpoint
   */
  struct Connection
  {
    bool established;           /**< Whether or not the connection is live */
    SocketBuffer *socketBuffer; /**< Host socket associated with the connection */
    Network::IPAddress destIP;      /**< Destination device IP address */
    Network::Port destPort;         /**< Destination device port */
    Network::Port hostPort;         /**< Port associated with the host */
  };
}    // namespace Ripple::Session

#endif /* !RIPPLE_SESSION_TYPES_HPP */
