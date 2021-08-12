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

/* ETL Includes */
#include <etl/list.h>

/* NanoPB Includes */
#include "pb.h"


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using PacketId       = uint32_t;
  using PacketCallback = void ( * )( const PacketId, const void *const, const size_t );
  using PacketFilter   = std::array<PacketId, 32>;

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
    PacketId id;                /**< System identifier for the packet */
    uint8_t size;               /**< Size of the packet on the wire (encoded) */
    const pb_msgdesc_t *fields; /**< NanoPB fields descriptor */
  };

  struct PacketHdr
  {
    PacketId id;
    uint8_t size;
    uint8_t pad0[ 3 ];
  };

}    // namespace Ripple

#endif /* !RIPPLE_PACKET_TYPES_HPP */
