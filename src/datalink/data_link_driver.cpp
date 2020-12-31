/********************************************************************************
 *  File Name:
 *    data_link_driver.cpp
 *
 *  Description:
 *    Datalink driver implementation
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/physical>
#include <Ripple/datalink>

namespace Ripple::DATALINK
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Handle *getHandle( SessionContext session )
  {
    if ( !session )
    {
      return nullptr;
    }
    else
    {
      auto context  = reinterpret_cast<NetStackHandle *>( session );
      auto datalink = reinterpret_cast<Handle *>( context->datalink );

      return datalink;
    }
  }

}  // namespace Ripple::DATALINK