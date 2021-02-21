/********************************************************************************
 *  File Name:
 *    context.cpp
 *
 *  Description:
 *    Net stack context implementation
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/src/netstack/context.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  Context::Context()
  {
  }


  Context::Context( Aurora::Memory::Heap &&heap ) : mHeap( std::move( heap ) )
  {
  }


  Context::~Context()
  {
  }


  void * Context::malloc( const size_t size )
  {
    void * mem =  mHeap.malloc( size );

    if( !mem )
    {
      mDelegateRegistry.call<CallbackId::CB_OUT_OF_MEMORY>();
    }

    return mem;
  }


  void Context::free( void *pv )
  {
    mHeap.free( pv );
  }

}    // namespace Ripple
