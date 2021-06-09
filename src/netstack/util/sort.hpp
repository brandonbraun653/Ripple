/********************************************************************************
 *  File Name:
 *    sort.hpp
 *
 *  Description:
 *    Sorting functions for the stack
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SORT_HPP
#define RIPPLE_SORT_HPP

/* Ripple Includes */
#include <Ripple/src/netstack/types.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  void sortFragments( MsgFrag ** headPtr );

}  // namespace Ripple

#endif  /* !RIPPLE_SORT_HPP */
