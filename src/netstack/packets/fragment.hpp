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

/* Ripple Includes */
#include <Ripple/src/netstack/util/memory.hpp>

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
    RefPtr<Fragment> next; /**< Next fragment */
    RefPtr<Fragment> prev; /**< Previous fragment */
    RefPtr<void *> data;   /**< Allocated memory for this fragment */
    uint16_t length;       /**< Length of the current fragment */
    uint16_t number;       /**< Which fragment number this is, zero indexed */
    uint16_t uuid;
  };

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Fragment_sPtr = RefPtr<Fragment>;

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Fragment_sPtr allocFragment( INetMgr *const context, const size_t payload_bytes );
  void fragmentSort( Fragment_sPtr *headPtr );

}  // namespace Ripple

#endif  /* !RIPPLE_FRAGMENT_TYPE_HPP */
