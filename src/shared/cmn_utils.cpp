/********************************************************************************
 *  File Name:
 *    cmn_utils.cpp
 *
 *  Description:
 *    Implementation of the Ripple shared utility functions
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/shared/cmn_utils.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  bool validateContext( SessionContext session )
  {
    if( !session )
    {
      return false;
    }
    auto context = reinterpret_cast<NetStackHandle *>( session );

    /* clang-format off */
    return (
      context->datalink &&
      context->network &&
      context->physical &&
      context->session &&
      context->transport
    );
    /* clang-format on */
  }
}  // namespace Ripple