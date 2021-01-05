/********************************************************************************
 *  File Name:
 *    session_user.hpp
 *
 *  Description:
 *    User interface for the Session layer. As far as the Ripple networking stack
 *    is concerned, this is as high as it goes. This interface assumes all the
 *    initialization steps for the network have been performed and the network is
 *    sitting in an idle or running state.
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_SESSION_USER_HPP
#define RIPPLE_SESSION_USER_HPP

/* Chimera Includes */
#include <Chimera/common>

/* Ripple Includes */
#include <Ripple/src/session/session_driver.hpp>
#include <Ripple/src/network/net_types.hpp>


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Opens a socket on the host device using statically allocated memory.
   *
   *  @param[in]  mgr       The session instance managing all connections
   *  @param[in]  sock      Statically allocated socket resources. Must exist while the socket is open.
   *  @param[in]  port      Which port to terminate
   *  @return Chimera::Status_t
   */
  Chimera::Status_t openSocket( Session::Manager &mgr, Session::Socket &sock, const NET::Port port );

  /**
   *  Closes a previously opened socket on the host device
   *
   *  @param[in]  mgr       The session instance managing all connections
   *  @param[in]  port      Which port to terminate
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeSocket( Session::Manager &mgr, const NET::Port port );

  /**
   *  Opens a connection to a remote device on the network
   *
   *  @param[in]  mgr       The session instance managing all connections
   *  @param[out] conn      Handle to the connection info
   *  @param[in]  ip        IP address of the remote device to connect to
   *  @param[in]  port      Port of the remote device to connect to
   *  @return Chimera::Status_t
   */
  Chimera::Status_t openConnection( Session::Manager &mgr, Session::Connection &conn, const NET::IPAddress ip, const NET::Port port );

  /**
   *  Closes a connection to a remote device on the network
   *
   *  @param[in]  mgr       The session instance managing all connections
   *  @param[in]  conn      Connection being disconnected from
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeConnection( Session::Manager &mgr, Session::Connection &conn );

  /**
   *  Writes a number of bytes into a connection stream
   *
   *  @param[in]  mgr       The session instance managing all connections
   *  @param[in]  conn      Connection handle describing where to write data to
   *  @param[in]  data      Data to write
   *  @param[in]  bytes     Number of bytes to write
   *  @return Chimera::Status_t
   */
  Chimera::Status_t write( Session::Manager &mgr, const Session::Connection &conn, const void *const data, const size_t bytes );

  /**
   *  Reads a number of bytes out from the connection stream
   *
   *  @param[in]  mgr       The session instance managing all connections
   *  @param[in]  conn      Connection handle describing where to read data from
   *  @param[out] data      Data to read into
   *  @param[in]  bytes     Number of bytes to read
   *  @return Chimera::Status_t
   */
  Chimera::Status_t read( Session::Manager &mgr, const Session::Connection &conn, void *const data, const size_t bytes );

  /**
   *  Queries the number of bytes available to read from the the connection
   *
   *  @param[in]  mgr       The session instance managing all connections
   *  @param[in]  conn      Connection handle describing where to read data from
   *  @return size_t
   */
  size_t available( Session::Manager &mgr, const Session::Connection &conn );

}  // namespace Ripple

#endif  /* !RIPPLE_SESSION_USER_HPP */
