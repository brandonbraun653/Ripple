/********************************************************************************
 *  File Name:
 *    fragment_util.cpp
 *
 *  Description:
 *    Utilities for acting on fragments
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* ETL Includes */
#include <etl/crc32.h>

/* Aurora Logging */
#include <Aurora/logging>
#include <Aurora/utility>

/* Ripple Includes */
#include <Ripple/netstack>

namespace Ripple::TMPFragment
{
  /*-------------------------------------------------------------------------------
  Fragment Actions
  -------------------------------------------------------------------------------*/
  /**
   * @brief Assigns the CRC32 field of a fragment list
   *
   * @param head        Start of the packet
   * @param crc32       CRC to assign
   */
  void setCRC32( MsgFrag *const head, const uint32_t crc32 )
  {
    // /*-------------------------------------------------
    // Input Protection
    // -------------------------------------------------*/
    // if ( !head || ( head->fragmentNumber != 0 ) || ( head->fragmentLength != sizeof( TransportHeader ) ) )
    // {
    //   return;
    // }

    // /*-------------------------------------------------
    // Assign the CRC
    // -------------------------------------------------*/
    // auto hdr = reinterpret_cast<TransportHeader *>( head->fragmentData );
    // hdr->crc = crc32;
  }


  /**
   * @brief Copies a fragment list into a buffer
   *
   * @param head        List being copied
   * @param buffer      Raw storage buffer
   * @param size        Size of the buffer in bytes
   * @return MsgFrag*   Pointer to the head of the copied list
   */
  MsgFrag *copyToBuffer( const MsgFrag *const head, void *const buffer, const size_t size )
  {
    // /*-------------------------------------------------
    // Input Protection
    // -------------------------------------------------*/
    // if ( !head || !buffer || !size )
    // {
    //   return nullptr;
    // }

    // const size_t allocationSize = TMPFragment::memoryConsumption( head );
    // if ( allocationSize > size )
    // {
    //   return nullptr;
    // }

    // /*-------------------------------------------------
    // Move the data over into the buffer
    // -------------------------------------------------*/
    // const MsgFrag *fragPtr = head;
    // uint8_t *pool          = reinterpret_cast<uint8_t *>( buffer );
    // const auto endAddr     = reinterpret_cast<std::uintptr_t>( pool + allocationSize );

    // size_t offset   = 0;
    // MsgFrag *newPtr = new ( pool ) MsgFrag();
    // pool += sizeof( MsgFrag );

    // MsgFrag *newRootPtr = newPtr;

    // while ( fragPtr && newPtr )
    // {
    //   /*-------------------------------------------------
    //   Copy over the fragment meta-data
    //   -------------------------------------------------*/
    //   *newPtr = *fragPtr;

    //   /*-------------------------------------------------
    //   Copy over the fragment data
    //   -------------------------------------------------*/
    //   newPtr->fragmentData = pool;
    //   memcpy( newPtr->fragmentData, fragPtr->fragmentData, fragPtr->fragmentLength );
    //   pool += fragPtr->fragmentLength;

    //   /*-------------------------------------------------
    //   Allocate a new fragment at the end of the list
    //   -------------------------------------------------*/
    //   if ( fragPtr->next )
    //   {
    //     MsgFrag *tmpPtr = new ( pool ) MsgFrag();
    //     pool += sizeof( MsgFrag );

    //     newPtr->next = tmpPtr;
    //     newPtr       = newPtr->next;
    //   }

    //   fragPtr = fragPtr->next;
    // }

    // RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( pool ) == endAddr );
    // return newRootPtr;
  }


  /*-------------------------------------------------------------------------------
  Fragment Information
  -------------------------------------------------------------------------------*/
  /**
   * @brief Calculates the CRC32 of a given packet
   * @note Assumes the packet is valid and sorted with the header
   *       as the first fragment.
   *
   * @param fragment    Fragment to calculate CRC on
   * @return uint32_t   Calculated CRC value
   */
  uint32_t calcCRC32( const MsgFrag *const fragment )
  {
    // /*-------------------------------------------------
    // Input Protections
    // -------------------------------------------------*/
    // if ( !fragment || !TMPFragment::isValid( fragment ) )
    // {
    //   return 0;
    // }

    // /*-------------------------------------------------
    // Initialize the CRC generator
    // -------------------------------------------------*/
    // etl::crc32 crc_gen;
    // crc_gen.reset();

    // /*-------------------------------------------------
    // CRC of the header itself, minus the CRC field
    // -------------------------------------------------*/
    // // auto header = reinterpret_cast<TransportHeader *>( fragment->fragmentData );

    // // crc_gen.add( reinterpret_cast<uint8_t *>( &header + sizeof( TransportHeader::crc ) ),
    // //              reinterpret_cast<uint8_t *>( &header + sizeof( header ) ) );

    // /*-------------------------------------------------
    // CRC of all the data payload fields
    // -------------------------------------------------*/
    // auto fragPtr = fragment->next;
    // while ( fragPtr->next )
    // {
    //   crc_gen.add( reinterpret_cast<uint8_t *>( fragPtr->fragmentData ),
    //                reinterpret_cast<uint8_t *>( fragPtr->fragmentData ) + fragPtr->fragmentLength );
    //   fragPtr = fragPtr->next;
    // }

    // return crc_gen.value();
  }


  /**
   * @brief Reads the stored CRC32 value in a fragment list
   *
   * @param head        Start of the packet
   * @return uint32_t   Stored CRC value
   */
  uint32_t readCRC32( const MsgFrag *const head )
  {
    // /*-------------------------------------------------
    // Input Protection
    // -------------------------------------------------*/
    // if ( !head || ( head->fragmentNumber != 0 ) || ( head->fragmentLength != sizeof( TransportHeader ) ) )
    // {
    //   return 0;
    // }

    // /*-------------------------------------------------
    // Read back the CRC data
    // -------------------------------------------------*/
    // auto hdr = reinterpret_cast<TransportHeader *>( head->fragmentData );
    // return hdr->crc;
  }


}    // namespace Ripple::Fragment
