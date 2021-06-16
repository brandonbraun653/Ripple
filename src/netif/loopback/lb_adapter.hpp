/********************************************************************************
 *  File Name:
 *    lb_adapter.hpp
 *
 *  Description:
 *    Loopback adapter interface
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NETIF_LOOPBACK_HPP
#define RIPPLE_NETIF_LOOPBACK_HPP

/* ETL Includes */
#include <etl/map.h>
#include <etl/queue.h>

/* Chimera Includes */
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/src/netstack/context.hpp>
#include <Ripple/src/netif/device_intf.hpp>

namespace Ripple::NetIf::Loopback
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  #if defined( EMBEDDED )
  #define LB_QUEUE_DEPTH  ( 16 )
  #else   /* SIMULATOR */
  #define LB_QUEUE_DEPTH  ( 512 )
  #endif  /* EMBEDDED */

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   * @brief Simple loopback adapter for routing packets back into the local network
   */
  class Adapter : public INetIf, public IARP
  {
  public:
    Adapter();
    ~Adapter();

    /*-------------------------------------------------------------------------------
    Net Interface
    -------------------------------------------------------------------------------*/
    bool powerUp( void * context ) final override;
    void powerDn() final override;
    Chimera::Status_t recv( Fragment_sPtr &fragmentList ) final override;
    Chimera::Status_t send( const Fragment_sPtr head, const IPAddress &ip ) final override;
    IARP *addressResolver() final override;
    size_t maxTransferSize() const final override;
    size_t maxNumFragments() const final override;
    size_t linkSpeed() const final override;
    size_t lastActive() const final override;

    /*-------------------------------------------------------------------------------
    ARP Interface
    -------------------------------------------------------------------------------*/
    Chimera::Status_t addARPEntry( const IPAddress &ip, const void *const mac, const size_t size ) final override;
    Chimera::Status_t dropARPEntry( const IPAddress &ip ) final override;
    bool arpLookUp( const IPAddress &ip, void *const mac, const size_t size ) final override;
    IPAddress arpLookUp( const void *const mac, const size_t size ) final override;

  private:
    Context_rPtr mContext;
    etl::map<IPAddress, uint64_t, 32> mAddressCache;
    etl::queue<Fragment_sPtr, LB_QUEUE_DEPTH> mPacketQueue;
    Chimera::Thread::RecursiveMutex *mLock;
  };

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Creates a handle to a new NRF24 device
   *
   *  @param[in]  context       The current network context
   *  @return Handle *
   */
  Adapter *createNetIf( Context_rPtr context );
}  // namespace Ripple::NetIf::Loopback

#endif  /* !RIPPLE_NETIF_LOOPBACK_HPP */
