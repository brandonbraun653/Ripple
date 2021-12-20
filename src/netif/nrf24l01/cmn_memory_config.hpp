/********************************************************************************
 *  File Name:
 *    cmn_memory_config.hpp
 *
 *  Description:
 *    Memory configuration options for static buffer allocation
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SHARED_MEMORY_CONFIG_HPP
#define RIPPLE_SHARED_MEMORY_CONFIG_HPP

/* STL Includes */
#include <cstddef>

namespace Ripple::NetIf::NRF24
{
  /*-------------------------------------------------------------------------------
  Common Settings
  -------------------------------------------------------------------------------*/
  /**
   *  Sets the maximum amount of memory that is allowed to be statically
   *  allocated from RAM for the entire instance of the network stack.
   */
  static constexpr size_t GLOBAL_MAX_BUFFER_ALLOCATION = 5 * 1024;

  /*-------------------------------------------------------------------------------
  Physical Layer
  -------------------------------------------------------------------------------*/
  namespace Physical
  {
    /**
     *  Due to the prevalence of counterfeit NRF24L01(+) chips, most of the
     *  cheap radios bought online will not support dynamic payloads, thus
     *  this network driver really only supports fixed-length payloads instead.
     *  The setting below controls the on-air frame length for each transmission,
     *  (in bytes) regardless of the number of actual user data bytes.
     *
     *  Be warned that increasing this size will increase RAM allocation across
     *  nearly all of the network stack.
     */
    static constexpr size_t DFLT_STATIC_PAYLOAD_SIZE = 32;
    static_assert( DFLT_STATIC_PAYLOAD_SIZE <= 32 );
  }    // namespace Physical

  /*-------------------------------------------------------------------------------
  Data Link Layer
  -------------------------------------------------------------------------------*/
  namespace DataLink
  {
    /**
     *  Set an upper limit on the number of bytes the DataLink layer can
     *  allocate from RAM at compile time for its purposes. This is just
     *  for static analysis.
     */
    static constexpr size_t MAX_ALLOCATION_SIZE = 4096;

    /**
     *  Defines the number of physical frames that can be buffered to either
     *  send to or receive from on the hardware. The limiting factor here is
     *  the processing rate of the higher network layers. If the datalink
     *  queues are not emptied quickly enough, these may need to be larger.
     */
    static constexpr size_t TX_QUEUE_ELEMENTS = 32;
    static constexpr size_t RX_QUEUE_ELEMENTS = 32;

    /**
     *  Defines the number of IP<->MAC mapping entries can exist in the
     *  Address Resolution Protocol cache table. If a message is attempted
     *  to be sent to a MAC that doesn't exist in this table, a cache miss
     *  will occur and a message will be sent out on the network to discover
     *  the appropriate mapping. Thus, it's important to not make this table
     *  too small and cause frequent cache misses, but not too large either
     *  such that you waste memory.
     */
    static constexpr size_t ARP_CACHE_TABLE_ELEMENTS = 15;

    /*-------------------------------------------------
    Perform compile time checks on memory allocation
    -------------------------------------------------*/
    namespace _Internal
    {
      static constexpr size_t _arp_entry_size = sizeof( uint32_t ) + sizeof( uint64_t );
      static constexpr size_t _arp_alloc_size = _arp_entry_size * ARP_CACHE_TABLE_ELEMENTS;

      static constexpr size_t _tx_alloc_size = TX_QUEUE_ELEMENTS * Physical::DFLT_STATIC_PAYLOAD_SIZE;
      static constexpr size_t _rx_alloc_size = RX_QUEUE_ELEMENTS * Physical::DFLT_STATIC_PAYLOAD_SIZE;

      static constexpr size_t _alloc_size = _arp_alloc_size + _tx_alloc_size + _rx_alloc_size;
      static_assert( _alloc_size <= MAX_ALLOCATION_SIZE );
    }    // namespace _Internal
  }      // namespace DataLink

  /*-------------------------------------------------------------------------------
  Validate Common Settings
  -------------------------------------------------------------------------------*/
  namespace _Internal
  {
    static constexpr size_t DataLinkAlloc = DataLink::_Internal::_alloc_size;

    static constexpr size_t AllLayerAlloc = DataLinkAlloc;
    static_assert( AllLayerAlloc <= GLOBAL_MAX_BUFFER_ALLOCATION );
  }    // namespace _Internal

}    // namespace Ripple

#endif /* !RIPPLE_SHARED_MEMORY_CONFIG_HPP */
