/********************************************************************************
 *  File Name:
 *    transport_types.hpp
 *
 *  Description:
 *    Types and definitions associated with the Transport layer
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_TRANSPORT_TYPES_HPP
#define RIPPLE_TRANSPORT_TYPES_HPP

/* STL Includes */
#include <cstdint>

namespace Ripple::Transport
{
  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum CallbackId : uint8_t
  {
    CB_UNHANDLED,
    CB_SERVICE_OVERRUN,

    CB_NUM_OPTIONS
  };


  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct Handle
  {
    uint8_t dummy;
  };

}  // namespace Ripple::Transport

#endif  /* !RIPPLE_TRANSPORT_TYPES_HPP */
