/********************************************************************************
 *  File Name:
 *    data_link_arp.cpp
 *
 *  Description:
 *    ARP implementation details
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>


namespace Ripple::NetIf::NRF24::DataLink
{
  /*-------------------------------------------------------------------------------
  ARPCache Implementation
  -------------------------------------------------------------------------------*/
  ARPCache::ARPCache()
  {

  }


  ARPCache::~ARPCache()
  {

  }


  void ARPCache::clear()
  {
    mCache.clear();
  }


  bool ARPCache::lookup( const std::string_view ip, Physical::MACAddress *addr )
  {
    /*-------------------------------------------------
    Find the registered key. Copy out the MAC if found.
    -------------------------------------------------*/
    bool found = false;
    Physical::MACAddress tmp = 0;
    if ( auto iter = mCache.find( ip ); iter != mCache.end() )
    {
      tmp = iter->second;
      found = true;
    }

    /*-------------------------------------------------
    Invoke the cache miss callback if the IP wasn't
    found and the caller was expecting it to exist.
    -------------------------------------------------*/
    if( !found )
    {
      if( addr && mCacheMissCallback )
      {
        mCacheMissCallback( ip );
      }

      return false;
    }

    /*-------------------------------------------------
    Copy out the data if the caller wants it
    -------------------------------------------------*/
    if( addr )
    {
      *addr = tmp;
    }

    return true;
  }


  void ARPCache::remove( const std::string_view ip )
  {
    if( auto iter = mCache.find( ip ); iter != mCache.end() )
    {
      mCache.erase( iter );
    }
  }


  bool ARPCache::insert( const std::string_view ip, const Physical::MACAddress &addr )
  {
    /*-------------------------------------------------
    Can't insert if the cache is full or the key exists
    -------------------------------------------------*/
    if ( mCache.full() || lookup( ip, nullptr ) )
    {
      return false;
    }

    mCache.insert( { ip, addr } );
    return true;
  }


  void ARPCache::onCacheMiss( ARPCallback &func )
  {
    mCacheMissCallback = func;
  }

}  // namespace Ripple::DataLink
