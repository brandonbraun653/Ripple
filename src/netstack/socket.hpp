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
  Aliases
  -------------------------------------------------------------------------------*/
  using Port = uint32_t;

  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr Port LOCAL_HOST_PORT    = 0;
  static constexpr IPAddress LOCAL_HOST_IP = 127001; /* 127.0.0.1 */

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
     *  Opens the socket for operation on the given port. The socket will
     *  automatically inherit the address of its creation context as well
     *  as the direction of the data stream.
     *
     *  @param[in]  port    Port to open as
     *  @return Chimera::Status_t
     */
    Chimera::Status_t open( const Port port );

    /**
     *  Closes the socket and places it in an idle state
     *  @return void
     */
    void close();

    /**
     *  Connects to a remote port
     *
     *  @param[in]  address   Remote address to connect to
     *  @param[in]  port      Port(s) to connect to
     *  @return Chimera::Status_t
     */
    Chimera::Status_t connect( const IPAddress address, const Port port );

    /**
     *  Disconnects from the given port
     *
     *  @param[in]  port    Port(s) to disconnect from
     *  @return Chimera::Status_t
     */
    Chimera::Status_t disconnect();

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

    Port port()
    {
      return mThisPort;
    }

    SocketType type()
    {
      return mSocketType;
    }

  protected:
    friend class Context;
    Socket( Context_rPtr ctx, const SocketType type, const size_t memory );


    size_t maxMem;      /**< Maximum memory assigned to this socket */
    size_t allocMem;    /**< Currently allocated memory */

    bool mTXReady;
    PacketQueue<5> mTXQueue;
    PacketQueue<5> mRXQueue;
    Chimera::Thread::RecursiveMutex *mLock;

    Port      mThisPort;    /**< Port of this socket */
    IPAddress mDestAddress; /**< Destination network address */
    Port      mDestPort;    /**< Destination network port */

  private:
    Context_rPtr mContext;
    SocketType mSocketType;
  };


}  // namespace Ripple

#endif  /* !RIPPLE_NETSTACK_SOCKET_HPP */
