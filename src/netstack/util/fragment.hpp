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

namespace Ripple::Fragment
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /*-------------------------------------------------
  Fragment Actions
  -------------------------------------------------*/
  void sort( MsgFrag ** headPtr );
  void setCRC32( MsgFrag *const head, const uint32_t crc32 );
  MsgFrag * copyToBuffer( const MsgFrag *const head, void *const buffer, const size_t size );
  bool copyPayloadToBuffer( const MsgFrag *const head, void *const buffer, const size_t size );

  /*-------------------------------------------------
  Fragment Information
  -------------------------------------------------*/
  bool isValid( const MsgFrag *const head );
  uint32_t calcCRC32( const MsgFrag * const head );
  uint32_t readCRC32( const MsgFrag * const head );
  size_t memoryConsumption( const MsgFrag *const head );

  /*-------------------------------------------------
  Debugging Utilities
  -------------------------------------------------*/
  void printPacketData( const MsgFrag *const head );

}  // namespace Ripple

#endif  /* !RIPPLE_SORT_HPP */
