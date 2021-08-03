/********************************************************************************
 *  File Name:
 *    definitions_contract.hpp
 *
 *  Description:
 *    Contract file for integrating projects that provide packet defintions to
 *    Ripple for TX-ing/RX-ing data.
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PACKET_DEFINTIONS_CONTRACT_HPP
#define RIPPLE_PACKET_DEFINTIONS_CONTRACT_HPP

/* ETL Includes */
#include <etl/map.h>

/* Ripple Includes */
#include <Ripple/src/netstack/packets/types.hpp>
#include "ripple_packet_contract_prj.hpp"

namespace Ripple
{
  extern etl::map<PacketId, PacketDef, RIPPLE_NUM_PACKETS> PacketDefinitions;
}  // namespace Ripple

#endif  /* !RIPPLE_PACKET_DEFINTIONS_CONTRACT_HPP */
