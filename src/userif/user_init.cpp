/********************************************************************************
 *  File Name:
 *    user_init.cpp
 *
 *  Description:
 *    Initialization methods for the Ripple network stack
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Aurora Includes */
#include <Aurora/memory>

/* Ripple Includes  */
#include <Ripple/src/netstack/context.hpp>
#include <Ripple/src/userif/user_init.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Context_rPtr create( void *mem_pool, const size_t mem_size )
  {
    using namespace Aurora::Memory;

    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if( !mem_pool || !mem_size )
    {
      return nullptr;
    }

    /*-------------------------------------------------
    Temporarily create a heap allocator on the stack to
    initialize the context object.
    -------------------------------------------------*/
    Heap tmpHeap;
    if( !tmpHeap.assignPool( mem_pool, mem_size ) )
    {
      return nullptr;
    }
    else
    {
      tmpHeap.staticReset();
    }

    /*-------------------------------------------------
    Construct inside the memory pool with placement new
    -------------------------------------------------*/
    void *rawContext = tmpHeap.malloc( sizeof( Context ) );
    Context_rPtr ctx = new( rawContext ) Context( std::move( tmpHeap ) );

    return ctx;
  }


  bool boot( Context &ctx, NetIf::INetIf &intf )
  {
    return false;
  }


  void shutdown( Context_rPtr ctx )
  {

  }


  void destroy( Context_rPtr ctx )
  {

  }

}  // namespace Ripple
