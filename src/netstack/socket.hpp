/********************************************************************************
 *  File Name:
 *    socket.hpp
 *
 *  Description:
 *    Socket class
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NETSTACK_SOCKET_HPP
#define RIPPLE_NETSTACK_SOCKET_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

/* ETL Includes */
#include <etl/list.h>
#include <etl/queue.h>

/* Aurora Includes */
#include <Aurora/memory>

/* Chimera Includes */
#include <Chimera/callback>

/* Ripple Includes */
#include <Ripple/src/netif/device_intf.hpp>
#include <Ripple/src/netstack/types.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Defines a single interface to transmit/receive data on the network.
   *  Must be created by a network context manager.
   */
  class Socket : public Chimera::Thread::Lockable<Socket>
  {
  public:
    ~Socket();

    /**
     *  Opens the socket for operation on the given port
     *
     *  @param[in]  port    Port to open as
     *  @return Chimera::Status_t
     */
    Chimera::Status_t open( const std::string_view &port );

    /**
     *  Closes the socket and places it in an idle state
     *  @return void
     */
    void close();

    /**
     *  Connects to remote port(s). This allows for a MIMO connection
     *  style, so that more data may be broadcasted/received.
     *
     *  @param[in]  port    Port(s) to connect to
     *  @return Chimera::Status_t
     */
    Chimera::Status_t connect( const std::string_view &port );

    /**
     *  Disconnects from the given port
     *
     *  @param[in]  port    Port(s) to disconnect from
     *  @return Chimera::Status_t
     */
    Chimera::Status_t disconnect( const std::string_view &port );

    /**
     *  Writes a number of bytes into a connection stream
     *
     *  @param[in]  data      Data to write
     *  @param[in]  bytes     Number of bytes to write
     *  @return Chimera::Status_t
     */
    Chimera::Status_t write( const void *const data, const size_t bytes );

    /**
     *  Reads a number of bytes out from the connection stream
     *
     *  @param[out] data      Data to read into
     *  @param[in]  bytes     Number of bytes to read
     *  @return Chimera::Status_t
     */
    Chimera::Status_t read( void *const data, const size_t bytes );

    /**
     *  Queries the number of bytes available to read from the the connection.
     *  @return size_t
     */
    size_t available();

    /**
     *  Comparison function for list sorting
     */
    static bool compare( const Socket *lhs, const Socket *rhs )
    {
      auto lhsAddr = reinterpret_cast<std::uintptr_t>( lhs->maxMem );
      auto rhsAddr = reinterpret_cast<std::uintptr_t>( rhs->maxMem );
      return lhsAddr < rhsAddr;
    }

  protected:
    friend class Context;
    Socket( Context_rPtr ctx, const SocketType type, const size_t memory );


    size_t maxMem;      /**< Maximum memory assigned to this socket */
    size_t allocMem;    /**< Currently allocated memory */

    etl::queue<MsgFrag*, 5> mTXQueue;
    etl::queue<MsgFrag*, 5> mRXQueue;
    Chimera::Thread::RecursiveMutex *mLock;


  private:
    Context_rPtr mContext;
    SocketType mSocketType;
  };


}  // namespace Ripple

#endif  /* !RIPPLE_NETSTACK_SOCKET_HPP */
