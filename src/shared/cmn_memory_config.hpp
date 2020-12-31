/********************************************************************************
 *  File Name:
 *    cmn_memory_config.hpp
 *
 *  Description:
 *    Memory configuration options for static buffer allocation
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SHARED_MEMORY_CONFIG_HPP
#define RIPPLE_SHARED_MEMORY_CONFIG_HPP

/* STL Includes */
#include <cstddef>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Data Link Layer
  -------------------------------------------------------------------------------*/
  namespace DATALINK
  {
    static constexpr size_t TX_QUEUE_ELEMENTS = 15;
    static constexpr size_t RX_QUEUE_ELEMENTS = 15;
    static constexpr size_t ARP_CACHE_TABLE_ELEMENTS = 15;
  }
}  // namespace Ripple

#endif  /* !RIPPLE_SHARED_MEMORY_CONFIG_HPP */
