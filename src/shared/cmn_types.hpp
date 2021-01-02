/********************************************************************************
 *  File Name:
 *    cmn_types.hpp
 *
 *  Description:
 *    Common type definitions and declarations shared across Ripple
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_COMMON_TYPES_HPP
#define RIPPLE_COMMON_TYPES_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using MACAddress = uint64_t; /**< Hardware address uniquely identifying a pipe in the network */
  using IPAddress  = uint32_t; /**< Use IPV4 style encoding, but quartet instead of octet */

  using SessionContext = void *;  /**< Opaque pointer to the NetStackHandle */

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct NetStackHandle
  {
    void *session;
    void *transport;
    void *network;
    void *datalink;
    void *physical;
  };

}  // namespace Ripple

#endif  /* !RIPPLE_COMMON_TYPES_HPP */
