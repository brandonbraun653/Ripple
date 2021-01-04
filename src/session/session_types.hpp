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

/* Ripple Includes */
#include <Ripple/src/network/net_types.hpp>
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
    NET::IPAddress address; /**< Static address of this device */

    /*-------------------------------------------------
    Advanced Properties:
    Defaults will be assigned if left blank
    -------------------------------------------------*/
    struct _Avanced
    {
      bool verifyRegisters;       /**< Optionally verify registers are set correctly at runtime */
      bool staticPayloads;        /**< Optionally enable static payloads (defaults to dynamic) */
      uint8_t staticPayloadSize;  /**< Fixed length of static payloads, if used */
      PHY::MACAddress mac;        /**< MAC Address for the device */
      PHY::AddressWidth macWidth; /**< Number of bytes used in the MAC address */
      PHY::DataRate dataRate;     /**< RF on-air data rate */
      PHY::RFPower rfPower;       /**< RF transmission power */
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
      advanced.macWidth          = PHY::AddressWidth::AW_INVALID;
      advanced.dataRate          = PHY::DataRate::DR_INVALID;
      advanced.rfPower           = PHY::RFPower::PA_INVALID;
      advanced.verifyRegisters   = false;
      advanced.staticPayloads    = false;
      advanced.staticPayloadSize = static_cast<uint8_t>( PHY::MAX_TX_PAYLOAD_SIZE );
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
}    // namespace Ripple::Session

#endif /* !RIPPLE_SESSION_TYPES_HPP */
