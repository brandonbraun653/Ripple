/********************************************************************************
 *  File Name:
 *    user_init.hpp
 *
 *  Description:
 *    Initialization methods for the Ripple network stack
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_USER_INIT_HPP
#define RIPPLE_USER_INIT_HPP

/* STL Includes */
#include <cstddef>

/* Ripple Includes */
#include <Ripple/src/netstack/context.hpp>
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/netif/device_intf.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Creates a new context that represents a network processing stack. Ripple
   *  does not allow dynamic memory allocation, so a pre-allocated buffer is
   *  needed for all network operations.
   *
   *  @note This does not include any memory needed by the netif driver.
   *
   *  @param[in]  mem_pool    Raw memory buffer
   *  @param[in]  mem_size    Size of the buffer in bytes
   *  @return Context         Created context object
   */
  Context_rPtr create( void *mem_pool, const size_t mem_size );

  /**
   *  Powers up the modules used in processing the network stack. Upon exit, the
   *  network stack is ready for operations.
   *
   *  @param[in]  ctx         Previously created network context
   *  @param[in]  intf        Network interface driver
   *  @return bool            Whether or not the boot was successful
   */
  bool boot( Context &ctx, NetIf::INetIf &intf );

  /**
   *  Stops all processing and places the network stack in an idle state. May
   *  be rebooted by calling boot(), but the system state will be lost and
   *  initialization steps re-performed.
   *
   *  @param[in]  ctx         Previously created network context
   *  @return void
   */
  void shutdown( Context &ctx );

  /**
   *  Completely obliterates all memory associated with the network stack
   *
   *  @param[in]  ctx         Previously created network context
   *  @return void
   */
  void destroy( Context &ctx );
}  // namespace Ripple

#endif  /* !RIPPLE_USER_INIT_HPP */
