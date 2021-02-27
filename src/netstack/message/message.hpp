/********************************************************************************
 *  File Name:
 *    message.hpp
 *
 *  Description:
 *    Message storage class type
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_MESSAGE_HPP
#define RIPPLE_MESSAGE_HPP

/* STL Includes */
#include <cstddef>

/* Ripple Includes */
#include <Ripple/src/netstack/context.hpp>

namespace Ripple
{
  struct MsgFrag
  {
    MsgFrag *nextFragment;   /**< Next message fragment in the list */
    void *fragmentData;      /**< Allocated memory for this fragment */
    uint16_t fragmentLength; /**< Length of the current fragment */
    uint16_t totalLength;    /**< Length of the entire packet */
    uint16_t fragmentNumber; /**< Which fragment number this is, zero indexed */
    uint8_t type;            /**< Message type */
    uint8_t flags;           /**< Any flags needed */
  };


  MsgFrag allocMessage( Context_rPtr context, const void *const data, const size_t size );

}    // namespace Ripple

#endif /* !RIPPLE_MESSAGE_HPP */
