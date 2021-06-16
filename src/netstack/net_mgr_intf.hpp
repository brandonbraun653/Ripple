/********************************************************************************
 *  File Name:
 *    net_mgr_intf.hpp
 *
 *  Description:
 *    Interface class for a network resource manager
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NET_MANAGER_HPP
#define RIPPLE_NET_MANAGER_HPP

/* STL Includes */
#include <cstddef>

/* Chimera Includes */
#include <Chimera/thread>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   * @brief Common interface to a network resource manager
   */
  class INetMgr : public Chimera::Thread::Lockable<INetMgr>
  {
  public:
    /**
     *  Allocates memory from the internally managed heap
     *
     *  @param[in]  size      Bytes to allocate
     *  @return void *
     */
    virtual void *malloc( const size_t size ) = 0;

    /**
     *  Free memory allocated on the internally managed heap
     *
     *  @param[in]  pv        The memory to be freed
     *  @return void
     */
    virtual void free( void *pv ) = 0;

    /**
     *  Remaining free bytes on the heap
     *  @return size_t
     */
    virtual size_t availableMemory() const = 0;

  private:
    friend Chimera::Thread::Lockable<INetMgr>;
  };

}  // namespace Ripple

#endif  /* !RIPPLE_NET_MANAGER_HPP */
