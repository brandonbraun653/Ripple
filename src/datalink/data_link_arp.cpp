/********************************************************************************
 *  File Name:
 *    data_link_arp.cpp
 *
 *  Description:
 *    ARP implementation details
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/datalink>
#include <Ripple/physical>
#include <Ripple/session>

namespace Ripple::DATALINK
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


  bool ARPCache::lookup( const uint32_t ip, PHY::MACAddress *addr )
  {
    /*-------------------------------------------------
    Find the registered key. Copy out the MAC if found.
    -------------------------------------------------*/
    bool found = false;
    PHY::MACAddress tmp = 0;
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


  void ARPCache::remove( const uint32_t ip )
  {
    if( auto iter = mCache.find( ip ); iter != mCache.end() )
    {
      mCache.erase( iter );
    }
  }


  bool ARPCache::insert( const uint32_t ip, const PHY::MACAddress &addr )
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

}  // namespace Ripple::DATALINK
