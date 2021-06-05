/********************************************************************************
 *  File Name:
 *    user_intf.hpp
 *
 *  Description:
 *    Declarations for the network stack user interface
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_NET_STACK_CONTEXT_HPP
#define RIPPLE_NET_STACK_CONTEXT_HPP

/* STL Includes */
#include <cstddef>
#include <cstdint>

/* ETL Includes */
#include <etl/list.h>
#include <etl/string.h>
#include <etl/queue.h>

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
  class Socket;

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Context_rPtr = Context *;
  using Socket_rPtr  = Socket *;
  using SocketId     = uint16_t;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum CallbackId : uint8_t
  {
    CB_OUT_OF_MEMORY,

    CB_NUM_OPTIONS,
    CB_INVALID
  };


  enum class SocketType : uint8_t
  {
    PUSH,
    PULL,

    INVALID
  };

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct MsgFrag
  {
    MsgFrag *nextFragment;   /**< Next message fragment in the list */
    void *fragmentData;      /**< Allocated memory for this fragment */
    uint16_t fragmentLength; /**< Length of the current fragment */
    uint16_t totalLength;    /**< Length of the entire packet */
    uint16_t fragmentNumber; /**< Which fragment number this is, zero indexed */
    uint8_t type;            /**< Message type */
    uint8_t flags;           /**< Any flags needed */
    SocketId socketId;       /**< Which socket this came from */
  };

}    // namespace Ripple

#endif /* !RIPPLE_NET_STACK_CONTEXT_HPP */
