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
#include <Ripple/src/netstack/packets/types.hpp>

/*
need some way of dynamically assigning packet lists for filtering and holding handlers.
It is like I need a structure that contains socket specific items. Factory functions
for generating these structures? Each understands the size associate and builds up the
structure from scratch using the context manager. The handlers could be expanded at
runtime with a copy and replace mechanism. I'm going to need a lot of shared_ptrs...
*/

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
  Structures
  -------------------------------------------------------------------------------*/
  /**
   * @brief Runtime configuration options for a socket
   */
  struct SocketConfig
  {
    Port devicePort;       /**< Port the socket will listen on */
    PacketFilter txFilter; /**< Packets the socket is allowed to TX */
    PacketFilter rxFilter; /**< Packets the socket will allow to be RX'd */
  };

  /**
   * @brief A gathering of stats for a socket
   */
  struct SocketStats
  {
    size_t txPackets;
    size_t rxPackets;

    size_t allocatedMem;
  };

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   * @brief Checks if a packet is in the given filter
   *
   * @param pkt       Packet to look for
   * @param filter    Which filter to look in
   * @return true     The packet is in the filter
   * @return false    The packet is not in the filter
   */
  bool packetInFilter( const PacketId pkt, const PacketFilter &filter );

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
     * @brief Opens the socket for operation
     *
     * @param cfg   Socket configuration parameters
     * @return Chimera::Status_t
     */
    Chimera::Status_t open( const SocketConfig &cfg );

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
     * @brief Gather statistics for the socket
     *
     * @param stats   Output object for the gathered data
     */
    void getStatistics( SocketStats &stats );

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
    friend bool transmit( const PacketId, Socket &, const void *const, const size_t );
    friend bool onReceive( const PacketId, Socket &, PacketCallback );
    friend bool onReceive( Socket &, PacketCallback );

    /**
     * @brief Construct a new Socket object
     *
     * Used by the context manager for creating a new object with
     * dynamically allocated memory.
     *
     * @param ctx     Context manager to use
     * @param type    What type of socket this is
     * @param memory  How much memory should be allocated for socket use
     */
    Socket( Context_rPtr ctx, const SocketType type, const size_t memory );

    /**
     *  @brief Low level function to write a number of bytes into a connection stream
     *
     *  @param[in]  data      Data to write
     *  @param[in]  bytes     Number of bytes to write
     *  @return Chimera::Status_t
     */
    Chimera::Status_t write( const void *const data, const size_t bytes );

    /**
     *  @brief Low level function to read a number of bytes out from the connection stream
     *
     *  @param[out] data      Data to read into
     *  @param[in]  bytes     Number of bytes to read
     *  @return Chimera::Status_t
     */
    Chimera::Status_t read( void *const data, const size_t bytes );

    /**
     *  @brief Queries the number of bytes available to read from the the connection stream
     *  @return size_t
     */
    size_t available();

    /**
     * @brief Periodic processing for the socket
     *
     * Handles reading data out of the RX queue and translating that into
     * packets. Upon reception of a good packet, calls reception handler.
     */
    void processData();


    size_t maxMem;   /**< Maximum memory assigned to this socket */
    size_t allocMem; /**< Currently allocated memory */

    PacketQueue<5> mTXQueue;
    PacketQueue<5> mRXQueue;

    Port mThisPort;         /**< Port of this socket */
    IPAddress mDestAddress; /**< Destination network address */
    Port mDestPort;         /**< Destination network port */
    SocketConfig mConfig;

    PacketCallback mCommonPktCallback;
    etl::map<PacketId, PacketCallback, 10> mPktCallbacks;

  private:
    Context_rPtr mContext;
    SocketType mSocketType;
  };


}    // namespace Ripple

#endif /* !RIPPLE_NETSTACK_SOCKET_HPP */
