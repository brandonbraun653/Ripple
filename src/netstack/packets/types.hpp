/********************************************************************************
 *  File Name:
 *    types.hpp
 *
 *  Description:
 *    Packet types declarations
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PACKET_TYPES_HPP
#define RIPPLE_PACKET_TYPES_HPP

/* STL Includes */
#include <cstdint>

/* NanoPB Includes */
#include "pb.h"


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using PacketId = uint32_t;

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  /**
   * @brief Provides a set of attributes to describe a packet
   *
   * All packets are based on ProtoBuf structures to ensure compatibility
   * with external systems.
   */
  struct PacketDef
  {
    PacketId id;          /**< System identifier for the packet */
    size_t size;          /**< Size of the packet on the wire (encoded) */
    pb_msgdesc_t *fields; /**< NanoPB fields descriptor */
  };

}  // namespace Ripple

#endif  /* !RIPPLE_PACKET_TYPES_HPP */
