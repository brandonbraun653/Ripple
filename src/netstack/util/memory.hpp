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

/* STL Includes */
#include <type_traits>

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
    explicit RefPtr( INetMgr *const context ) : RefPtr( context, 0 )
    {
    }

    /**
     * @brief Construct a new Ref Ptr object
     * Allocates enough memory for this object type + whatever data is being
     * held. Typically used for arrays, but can hold more complex types too.
     *
     * @param context   Network memory context to allocate from
     * @param size      Additional bytes to allocate
     */
    explicit RefPtr( INetMgr * context, const size_t size ) : mContext( context )
    {
      Chimera::Thread::LockGuard lck( *context );

      const auto total_size = this->size() + size;

      /*-------------------------------------------------
      Check to ensure memory requirements are met
      -------------------------------------------------*/
      if ( context->availableMemory() < total_size )
      {
        mPtr      = nullptr;
        mRefCount = nullptr;
        return;
      }

      /*-------------------------------------------------
      Allocate memory from the network context
      -------------------------------------------------*/
      uint8_t *pool                = reinterpret_cast<uint8_t *>( context->malloc( total_size ) );
      const auto expected_end_addr = reinterpret_cast<std::uintptr_t>( pool ) + total_size;

      /*-------------------------------------------------
      Construct the reference counter. Order important.
      See do_cleanup() for details.
      -------------------------------------------------*/
      mRefCount = new ( pool ) CountType( 1 );
      pool += sizeof( CountType );

      /*-------------------------------------------------
      Construct the underlying object
      -------------------------------------------------*/
      mPtr = new ( pool ) T();
      pool += sizeof( T );

      /*-------------------------------------------------
      If additional size was specified and the underlying
      type is a pointer, assume that pointer will hold
      the remaining bytes.
      -------------------------------------------------*/
      if constexpr( std::is_pointer<T>::value )
      {
        if ( size )
        {
          *mPtr = pool;
          memset( *mPtr, 0xCC, size );

          pool += size;
        }
      }

      /*-------------------------------------------------
      Make it abundantly clear at runtime if some kind of
      allocation error occurred.
      -------------------------------------------------*/
      RT_HARD_ASSERT( expected_end_addr == reinterpret_cast<std::uintptr_t>( pool ) );
    }

    /**
     * @brief Copy construct a new Ref Ptr object
     *
     * @param obj     Object being copied
     */
    RefPtr( const RefPtr<T> &obj )
    {
      this->mContext  = obj.mContext;
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
      this->mContext  = obj.mContext;
      this->mPtr      = obj.mPtr;
      this->mRefCount = obj.mRefCount;

      obj.mPtr      = nullptr;
      obj.mRefCount = nullptr;
      obj.mContext  = nullptr;
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
      Copy in the new data
      -------------------------------------------------*/
      this->mContext  = obj.mContext;
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
      this->mContext  = obj.mContext;
      this->mPtr      = obj.mPtr;
      this->mRefCount = obj.mRefCount;

      obj.mPtr      = nullptr;
      obj.mRefCount = nullptr;
      obj.mContext  = nullptr;

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

    /**
     * @brief Returns pointer to underlying data
     *
     * @return T*
     */
    T * get() const
    {
      return this->mPtr;
    }

    /**
     * @brief Returns the total size of the object and managed data
     *
     * @return constexpr size_t
     */
    static constexpr size_t size()
    {
      return sizeof( T ) + sizeof( CountType );
    }

  protected:
    using CountType = size_t;

  private:
    INetMgr *mContext;
    CountType *mRefCount;
    T *mPtr;

    /**
     * @brief Handle reference counting on destruction
     * Can optionally release memory.
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

      /*-------------------------------------------------
      Decrement the reference count
      -------------------------------------------------*/
      ( *mRefCount )--;
      if ( *mRefCount != 0 )
      {
        return;
      }

      /* By this point, all references need to be valid */
      RT_HARD_ASSERT( mRefCount && mPtr && mContext );

      /*-------------------------------------------------
      If the type isn't a pointer, there is a non-trivial
      destructor that needs calling. Type was constructed
      with placement new, which won't automatically call
      the destructor on free.
      -------------------------------------------------*/
      if constexpr( !std::is_pointer<T>::value )
      {
        mPtr->~T();
      }

      /*-------------------------------------------------
      The entire packet was allocated in a single block,
      with mRefCount being the first item. This points to
      the start of the block and thus is the only item
      that needs freeing. All other pointers from this
      class are assigned or allocated with placement new.
      -------------------------------------------------*/
      mContext->free( mRefCount );
    }
  };

}    // namespace Ripple

#endif /* !RIPPLE_UTIL_MEMORY_HPP */
