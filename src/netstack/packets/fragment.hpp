/********************************************************************************
 *  File Name:
 *    fragment.hpp
 *
 *  Description:
 *    Data fragment types
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_FRAGMENT_TYPE_HPP
#define RIPPLE_FRAGMENT_TYPE_HPP

/* Aurora Includes  */
#include <Aurora/memory>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   * @brief Models a piece of data belonging to a packet
   *
   * Utilizes a list structure to allow non-contiguous memory and dynamically
   * building a packet at runtime. Each fragment represents a single piece of
   * a full packet. The fragment's memory is dynamically allocated inside of
   * the network context manager and its lifetime is controlled via reference
   * counting.
   */
  class Fragment
  {
  public:
    Aurora::Memory::shared_ptr<Fragment> next; /**< Next fragment */
    Aurora::Memory::shared_ptr<void *> data;   /**< Allocated memory for this fragment */
    uint16_t length;                           /**< Length of the current fragment */
    uint16_t number;                           /**< Which fragment number this is, zero indexed */
    uint16_t total;                            /**< Total number of fragments */
    uint16_t uuid;                             /**< Unique ID for the fragment */
  };

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Fragment_sPtr = Aurora::Memory::shared_ptr<Fragment>;

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Fragment_sPtr allocFragment( Aurora::Memory::IHeapAllocator *const context, const size_t payload_bytes );
  void fragmentSort( Fragment_sPtr *headPtr );
  Fragment_sPtr fragmentShallowCopy( Aurora::Memory::IHeapAllocator *const context, const Fragment_sPtr &fragment );

}  // namespace Ripple

#endif  /* !RIPPLE_FRAGMENT_TYPE_HPP */
