/********************************************************************************
 *  File Name:
 *    context.hpp
 *
 *  Description:
 *    Declaration of the network context
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NETSTACK_CONTEXT_HPP
#define RIPPLE_NETSTACK_CONTEXT_HPP

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
   *  Network context manager that handles high level operations, typically
   *  revolving around memory management.
   */
  class Context : public Chimera::Callback::DelegateService<Context, CallbackId>, public Chimera::Thread::Lockable<Context>
  {
  public:
    Context();
    ~Context();

    /**
     *  Creates a new socket. Returns nullptr if out of memory.
     *
     *  @note The socket object will be constructed in-place with the
     *        cache memory. The cache must be word aligned.
     *
     *  @param[in]  type      Socket type to create
     *  @param[in]  cacheSize How many bytes to allocate for packet memory
     *  @return Socket_rPtr
     */
    Socket_rPtr socket( const SocketType type, const size_t cacheSize );

    /**
     *  Attaches a network interface instance to use as the transport layer
     *
     *  @param[in]  netif     Network interface
     *  @return void
     */
    void attachNetif( NetIf::INetIf *const netif );

    /**
     *  Allocates memory from the internally managed heap
     *
     *  @param[in]  size      Bytes to allocate
     *  @return void *
     */
    void *malloc( const size_t size );

    /**
     *  Free memory allocated on the internally managed heap
     *
     *  @param[in]  pv        The memory to be freed
     *  @return void
     */
    void free( void *pv );

    /**
     *  Remaining free bytes on the heap
     *  @return size_t
     */
    size_t availableMemory() const;


  protected:
    friend class Socket;
    friend Context_rPtr create( void *, const size_t );

    /**
     *  Constructor for creating the context from a pre-allocated memory pool
     *
     *  @param[in]  heap    Memory allocator used for construction
     */
    explicit Context( Aurora::Memory::Heap &&heap );

    /**
     *  Class manager thread that handles all the runtime operations
     *  needed for the network to stay alive and have messages flowing.
     *
     *  @param[in]  arg     Unused
     *  @return void
     */
    void ManagerThread( void *arg );

    /**
     *  Processes RX data and routes to the proper socket
     *  @return void
     */
    void processRX();

    /**
     *  Processes TX data and queues for transmission
     *  @return void
     */
    void processTX();

  private:
    NetIf::INetIf *mNetIf;               /**< Network interface driver */
    Aurora::Memory::Heap mHeap;          /**< Managed memory pool for the whole network */
    etl::list<Socket *, 16> mSocketList; /**< Socket control structures */
  };


}    // namespace Ripple

#endif /* !RIPPLE_NETSTACK_CONTEXT_HPP */
