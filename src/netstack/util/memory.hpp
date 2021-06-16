/********************************************************************************
 *  File Name:
 *    memory.hpp
 *
 *  Description:
 *    Memory utilities for the network stack
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_UTIL_MEMORY_HPP
#define RIPPLE_UTIL_MEMORY_HPP

/* Chimera Includes */
#include <Chimera/assert>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/src/netstack/net_mgr_intf.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   * @brief Reference counted pointer using network context memory
   *
   * @tparam T  Object type being managed
   */
  template<typename T>
  class RefPtr
  {
  public:
    /**
     * @brief Default construct a new Ref Ptr object
     */
    explicit RefPtr() : mContext( nullptr ), mRefCount( nullptr ), mPtr( nullptr )
    {
    }

    /**
     * @brief Construct a new Ref Ptr object
     * Automatically allocates memory for the required type
     *
     * @param context   Network memory context to allocate from
     */
    explicit RefPtr( INetMgr * context ) : mContext( context )
    {
      Chimera::Thread::LockGuard lck( *context );

      /*-------------------------------------------------
      Check to ensure memory requirements are met
      -------------------------------------------------*/
      if ( context->availableMemory() < this->size() )
      {
        mPtr      = nullptr;
        mRefCount = nullptr;
        return;
      }

      /*-------------------------------------------------
      Allocate memory from the network context
      -------------------------------------------------*/
      uint8_t *pool                = reinterpret_cast<uint8_t *>( context->malloc( this->size() ) );
      const auto expected_end_addr = reinterpret_cast<std::uintptr_t>( this->size() );

      /*-------------------------------------------------
      Construct the reference counter
      -------------------------------------------------*/
      mRefCount = new ( pool ) CountType( 1 );
      pool += sizeof( CountType );

      /*-------------------------------------------------
      Construct the object
      -------------------------------------------------*/
      mPtr = new ( pool ) T();
      pool += sizeof( T );

      RT_HARD_ASSERT( expected_end_addr == reinterpret_cast<std::uintptr_t>( pool ) );
    }

    /**
     * @brief Copy construct a new Ref Ptr object
     *
     * @param obj     Object being copied
     */
    RefPtr( const RefPtr<T> &obj )
    {
      this->mPtr      = obj.mPtr;
      this->mRefCount = obj.mRefCount;

      if ( obj.mPtr && obj.mRefCount )
      {
        ( *this->mRefCount )++;
      }
    }

    /**
     * @brief Move construct a new Ref Ptr object
     *
     * @param obj     Dying object being moved
     */
    explicit RefPtr( RefPtr<T> &&obj )
    {
      this->mPtr      = obj.mPtr;
      this->mRefCount = obj.mRefCount;

      obj.mPtr      = nullptr;
      obj.mRefCount = nullptr;
    }

    /**
     * @brief Destroy the Ref Ptr object
     */
    ~RefPtr()
    {
      this->do_cleanup();
    }

    /**
     * @brief Copy assignment operator
     *
     * @param obj   Object being copied
     * @return T&
     */
    RefPtr<T> &operator=( const RefPtr<T> &obj )
    {
      /*-------------------------------------------------
      Clean up existing data
      -------------------------------------------------*/
      do_cleanup();

      /*-------------------------------------------------
      Copy in the new data
      -------------------------------------------------*/
      this->mPtr      = obj.mPtr;
      this->mRefCount = obj.mRefCount;

      if ( obj.mPtr && obj.mRefCount )
      {
        ( *this->mRefCount )++;
      }

      return *this;
    }

    /**
     * @brief Move assignment operator
     *
     * @param obj     Object being moved
     * @return T&
     */
    RefPtr<T> &operator=( RefPtr<T> &&obj )
    {
      /*-------------------------------------------------
      Clean up existing data
      -------------------------------------------------*/
      do_cleanup();

      /*-------------------------------------------------
      Copy in the new data
      -------------------------------------------------*/
      this->mPtr      = obj.mPtr;
      this->mRefCount = obj.mRefCount;

      obj.mPtr      = nullptr;
      obj.mRefCount = nullptr;

      return *this;
    }

    /**
     * @brief Pointer access overload
     *
     * @return T*
     */
    T *operator->() const
    {
      return this->mPtr;
    }

    /**
     * @brief Reference access overload
     *
     * @return T&
     */
    T &operator*() const
    {
      return this->mPtr;
    }

    /**
     * @brief Evaluates if this object is valid
     *
     * @return true
     * @return false
     */
    explicit operator bool() const
    {
      return mContext && mRefCount && mPtr;
    }

    T * get() const
    {
      return this->mPtr;
    }

  protected:
    using CountType = size_t;

    size_t size() const
    {
      return sizeof( T ) + sizeof( CountType );
    }

  private:
    INetMgr *mContext;
    CountType *mRefCount;
    T *mPtr;

    /*-------------------------------------------------
    Double check the data is trivial. This class is
    really only meant for the most basic structures.
    -------------------------------------------------*/
    // static_assert( std::is_trivial<T>::value );

    /**
     * @brief Deallocate
     *
     */
    void do_cleanup()
    {
      /*-------------------------------------------------
      No references?
      -------------------------------------------------*/
      if ( !mRefCount )
      {
        return;
      }

      ( *mRefCount )--;
      if ( *mRefCount != 0 )
      {
        return;
      }

      /*-------------------------------------------------
      Deallocate the memory. Must explicitly call the ptr
      destructor due to creation with placement new.
      -------------------------------------------------*/
      RT_HARD_ASSERT( mRefCount && mPtr && mContext );

      mPtr->~T();
      mContext->free( mRefCount );
      mContext->free( mPtr );
    }
  };

}    // namespace Ripple

#endif /* !RIPPLE_UTIL_MEMORY_HPP */
