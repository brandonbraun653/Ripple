/********************************************************************************
 *  File Name:
 *    data_link_driver.cpp
 *
 *  Description:
 *    DataLink driver implementation
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/physical>
#include <Ripple/datalink>

namespace Ripple::DataLink
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Handle *getHandle( Session::Context session )
  {
    if ( !session )
    {
      return nullptr;
    }
    else
    {
      auto context  = reinterpret_cast<Session::Handle *>( session );
      auto DataLink = reinterpret_cast<Handle *>( context->datalink );

      return DataLink;
    }
  }

}  // namespace Ripple::DataLink
