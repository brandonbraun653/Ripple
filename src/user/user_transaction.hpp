/********************************************************************************
 *  File Name:
 *    user_transaction.hpp
 *
 *  Description:
 *    API for network transactions on Ripple
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_USER_TRANSACTIONS_HPP
#define RIPPLE_USER_TRANSACTIONS_HPP

/* Ripple Includes */
#include <Ripple/src/netstack/packets/types.hpp>
#include <Ripple/src/netstack/socket.hpp>


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using PacketCallback = void( * )( const PacketId );

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   * @brief Transmit a packet on the network
   *
   * @param pkt       Which packet to transmit
   * @param socket    Socket connection the packet will be transmitted through
   * @param data      Packet's data
   * @param size      Packet's size (must match size in packet definitions)
   * @return true     The packet was queued for transmission
   * @return false    The packet couldn't be queued for some reason
   */
  bool transmit( const PacketId pkt, Socket &socket, const void *const data, const size_t size );

  /**
   * @brief Register a callback to execute when a particular packet is received
   * @note This callback will supercede any generic callback that has been registered
   *
   * @param pkt       Which packet to register the callback against
   * @param socket    Socket connection that should listen for the packet and execute the callback
   * @param callback  Callback to execute
   * @return true     The callback was registered
   * @return false    The callback failed to be registered
   */
  bool onReceive( const PacketId pkt, Socket &socket, PacketCallback callback );

  /**
   * @brief Register a generic callback to execute on the reception of any packet
   * @warning Only one callback may be registered with this method. Subsequent calls
   *          will override any previous registrations.
   *
   * @param socket    Socket connection to listen for packets on
   * @param callback  Callback to execute
   * @return true     The callback was registered
   * @return false    The callback failed to be registered
   */
  bool onReceive( Socket &socket, PacketCallback callback );

}  // namespace Ripple

#endif  /* !RIPPLE_USER_TRANSACTIONS_HPP */
