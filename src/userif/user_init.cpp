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

/* Chimera Includes */
#include <Chimera/thread>

/* Ripple Includes  */
#include <Ripple/src/netstack/context.hpp>
#include <Ripple/src/userif/user_init.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t THREAD_STACK_BYTES    = 2048;
  static constexpr size_t THREAD_STACK_WORDS    = STACK_BYTES( THREAD_STACK_BYTES );
  static constexpr std::string_view THREAD_NAME = "NetMgr";

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Context_rPtr create( void *mem_pool, const size_t mem_size )
  {
    using namespace Aurora::Memory;
    using namespace Chimera::Thread;

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
    tmpHeap.assignMemoryPool( mem_pool, mem_size );

    /*-------------------------------------------------
    Construct the network context with the memory pool
    -------------------------------------------------*/
    void *rawContext = tmpHeap.malloc( sizeof( Context ) );
    Context_rPtr ctx = new( rawContext ) Context( std::move( tmpHeap ) );

    /*-------------------------------------------------
    Boot the network manager thread
    -------------------------------------------------*/
    TaskDelegate dlFunc = TaskDelegate::create<Context, &Context::ManagerThread>( *ctx );

    Task netManager;
    TaskId threadId;
    TaskConfig cfg;

    cfg.arg                                   = nullptr;
    cfg.function                              = dlFunc;
    cfg.priority                              = Priority::LEVEL_4;
    cfg.stackWords                            = THREAD_STACK_WORDS;
    cfg.type                                  = TaskInitType::STATIC;
    cfg.name                                  = THREAD_NAME.data();
    cfg.specialization.staticTask.stackBuffer = ctx->malloc( THREAD_STACK_BYTES );
    cfg.specialization.staticTask.stackSize   = THREAD_STACK_BYTES;

    netManager.create( cfg );
    threadId = netManager.start();
    sendTaskMsg( threadId, ITCMsg::TSK_MSG_WAKEUP, TIMEOUT_DONT_WAIT );

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
