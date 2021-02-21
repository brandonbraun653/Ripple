/********************************************************************************
 *  File Name:
 *    context.hpp
 *
 *  Description:
 *    Declarations for the network stack context manager
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NET_STACK_CONTEXT_HPP
#define RIPPLE_NET_STACK_CONTEXT_HPP

/* STL Includes */
#include <cstddef>

/* Aurora Includes */
#include <Aurora/memory>

/* Chimera Includes */
#include <Chimera/callback>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Forward Declarations
  -------------------------------------------------------------------------------*/
  class Context;

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Context_rPtr = Context *;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum CallbackId : uint8_t
  {
    CB_OUT_OF_MEMORY,

    CB_NUM_OPTIONS,
    CB_INVALID
  };

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  class Context : public Chimera::Callback::DelegateService<Context, CallbackId>
  {
  public:
    Context();
    ~Context();

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



  protected:
    friend Context_rPtr create( void *, const size_t );

    /**
     *  Constructor for creating the context from a pre-allocated memory pool
     *
     *  @param[in]  heap    Memory allocator used for construction
     */
    explicit Context( Aurora::Memory::Heap &&heap );

  private:
    Aurora::Memory::Heap mHeap;
  };
}  // namespace Ripple

#endif  /* !RIPPLE_NET_STACK_CONTEXT_HPP */
