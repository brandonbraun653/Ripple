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
    this->lock();
    void *mem = mHeap.malloc( size );
    this->unlock();

    if( !mem )
    {
      mCBService_registry.call<CallbackId::CB_OUT_OF_MEMORY>();
    }

    return mem;
  }


  void Context::free( void *pv )
  {
    this->lock();
    mHeap.free( pv );
    this->unlock();
  }


  size_t Context::availableMemory() const
  {
    return mHeap.available();
  }

}    // namespace Ripple
