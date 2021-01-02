/********************************************************************************
 *  File Name:
 *    data_link_arp.hpp
 *
 *  Description:
 *    Address Resolution Protocol interface
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_DATALINK_ARP_HPP
#define RIPPLE_DATALINK_ARP_HPP

/* STL Includes */
#include <array>
#include <utility>

/* ETL Includes */
#include <etl/delegate.h>
#include <etl/flat_map.h>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/shared/cmn_memory_config.hpp>

namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using ARPMap = std::pair<uint32_t, PHY::MACAddress>;
  using ARPCallback = etl::delegate<void( const uint32_t )>;


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  A simple cache for storing mappings of IP addresses to MAC addresses.
   *  The class is not thread safe, so additional protection is required if
   *  access is to be performed from multiple threads.
   */
  class ARPCache
  {
  public:
    ARPCache();
    ~ARPCache();

    /**
     *  Empties the cache of all data. Does not modify a registered callback.
     *  @return void
     */
    void clear();

    /**
     *  Looks for an entry with the given IP address and returns the
     *  associated MAC address if found.
     *
     *  @param[in]  ip        The IP address to look up
     *  @param[out] addr      The associated MAC address
     *  @return bool          Whether or not the lookup was successful
     */
    bool lookup( const uint32_t ip, PHY::MACAddress *addr );

    /**
     *  Removes the cache entry associated with the given IP address.
     *  Does nothing if the entry does not exist.
     *
     *  @return void
     */
    void remove( const uint32_t ip );

    /**
     *  Inserts a new entry into the cache table
     *
     *  @param[in]  ip        IP address to use as the key
     *  @param[in]  addr      MAC address used as the value
     *  @return bool          Whether the insertion was successful
     */
    bool insert( const uint32_t ip, const PHY::MACAddress &addr );

    /**
     *  Register a callback to execute when a lookup fails
     *
     *  @param[in]  func      Function to execute on failure
     *  @return void
     */
    void onCacheMiss( ARPCallback &func );

  private:
    ARPCallback mCacheMissCallback;
    etl::flat_map<uint32_t, PHY::MACAddress, ARP_CACHE_TABLE_ELEMENTS> mCache;
  };

}  // namespace Ripple::DATALINK::ARP

#endif  /* !RIPPLE_DATALINK_ARP_HPP */
