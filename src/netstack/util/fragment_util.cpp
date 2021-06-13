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

namespace Ripple::Fragment
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
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !head || ( head->fragmentNumber != 0 ) || ( head->fragmentLength != sizeof( TransportHeader ) ) )
    {
      return;
    }

    /*-------------------------------------------------
    Assign the CRC
    -------------------------------------------------*/
    auto hdr = reinterpret_cast<TransportHeader *>( head->fragmentData );
    hdr->crc = crc32;
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
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !head || !buffer || !size )
    {
      return nullptr;
    }

    const size_t allocationSize = Fragment::memoryConsumption( head );
    if ( allocationSize > size )
    {
      return nullptr;
    }

    /*-------------------------------------------------
    Move the data over into the buffer
    -------------------------------------------------*/
    const MsgFrag *fragPtr = head;
    uint8_t *pool          = reinterpret_cast<uint8_t *>( buffer );
    const auto endAddr     = reinterpret_cast<std::uintptr_t>( pool + allocationSize );

    size_t offset   = 0;
    MsgFrag *newPtr = new ( pool ) MsgFrag();
    pool += sizeof( MsgFrag );

    MsgFrag *newRootPtr = newPtr;

    while ( fragPtr && newPtr )
    {
      /*-------------------------------------------------
      Copy over the fragment meta-data
      -------------------------------------------------*/
      *newPtr = *fragPtr;

      /*-------------------------------------------------
      Copy over the fragment data
      -------------------------------------------------*/
      newPtr->fragmentData = pool;
      memcpy( newPtr->fragmentData, fragPtr->fragmentData, fragPtr->fragmentLength );
      pool += fragPtr->fragmentLength;

      /*-------------------------------------------------
      Allocate a new fragment at the end of the list
      -------------------------------------------------*/
      if ( fragPtr->next )
      {
        MsgFrag *tmpPtr = new ( pool ) MsgFrag();
        pool += sizeof( MsgFrag );

        newPtr->next = tmpPtr;
        newPtr       = newPtr->next;
      }

      fragPtr = fragPtr->next;
    }

    RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( pool ) == endAddr );
    return newRootPtr;
  }

  bool copyPayloadToBuffer( const MsgFrag *const head, void *const buffer, const size_t size )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !head || !buffer || !size )
    {
      return false;
    }

    /*-------------------------------------------------
    Copy the data over
    -------------------------------------------------*/
    const MsgFrag * fragPtr = head;
    size_t offset = 0;

    while( fragPtr && ( offset < size ) )
    {
      memcpy( reinterpret_cast<uint8_t *const>( buffer ) + offset, fragPtr->fragmentData, fragPtr->fragmentLength );
      offset += fragPtr->fragmentLength;
      fragPtr = fragPtr->next;
    }

    return ( !fragPtr && ( offset <= size ) );
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
    /*-------------------------------------------------
    Input Protections
    -------------------------------------------------*/
    if ( !fragment || !Fragment::isValid( fragment ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Initialize the CRC generator
    -------------------------------------------------*/
    etl::crc32 crc_gen;
    crc_gen.reset();

    /*-------------------------------------------------
    CRC of the header itself, minus the CRC field
    -------------------------------------------------*/
    // auto header = reinterpret_cast<TransportHeader *>( fragment->fragmentData );

    // crc_gen.add( reinterpret_cast<uint8_t *>( &header + sizeof( TransportHeader::crc ) ),
    //              reinterpret_cast<uint8_t *>( &header + sizeof( header ) ) );

    /*-------------------------------------------------
    CRC of all the data payload fields
    -------------------------------------------------*/
    auto fragPtr = fragment->next;
    while ( fragPtr->next )
    {
      crc_gen.add( reinterpret_cast<uint8_t *>( fragPtr->fragmentData ),
                   reinterpret_cast<uint8_t *>( fragPtr->fragmentData ) + fragPtr->fragmentLength );
      fragPtr = fragPtr->next;
    }

    return crc_gen.value();
  }


  /**
   * @brief Reads the stored CRC32 value in a fragment list
   *
   * @param head        Start of the packet
   * @return uint32_t   Stored CRC value
   */
  uint32_t readCRC32( const MsgFrag *const head )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !head || ( head->fragmentNumber != 0 ) || ( head->fragmentLength != sizeof( TransportHeader ) ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Read back the CRC data
    -------------------------------------------------*/
    auto hdr = reinterpret_cast<TransportHeader *>( head->fragmentData );
    return hdr->crc;
  }


  /**
   * @brief Validates a fragmented packet has the expected structure
   *
   * @param head    Start of the packet
   * @return true   Packet is valid
   * @return false  Packet is not valid
   */
  bool isValid( const MsgFrag *const head )
  {
    /*-------------------------------------------------
    Input Protections
    -------------------------------------------------*/
    if ( !head )
    {
      return false;
    }

    /*-------------------------------------------------
    Walk each node in the list and verify parameters
    -------------------------------------------------*/
    const size_t expectedBytes = head->totalLength;
    const size_t expectedUUID  = head->uuid;

    size_t currentBytes   = 0;
    size_t currentIdx     = 0;
    const MsgFrag *packet = head;

    while ( packet )
    {
      /*-------------------------------------------------
      Packet number is in order?
      -------------------------------------------------*/
      if ( packet->fragmentNumber != currentIdx )
      {
        return false;
      }
      currentIdx++;

      /*-------------------------------------------------
      Data field is valid?
      -------------------------------------------------*/
      if ( !packet->fragmentData )
      {
        return false;
      }

      /*-------------------------------------------------
      Packet UUID is marked as expected?
      -------------------------------------------------*/
      if ( packet->uuid != expectedUUID )
      {
        return false;
      }

      /*-------------------------------------------------
      Accumulate total number of bytes
      -------------------------------------------------*/
      currentBytes += packet->fragmentLength;

      /*-------------------------------------------------
      Move on to the next packet in the list
      -------------------------------------------------*/
      packet = packet->next;
    }

    /*-------------------------------------------------
    Total number of accumulated bytes match?
    -------------------------------------------------*/
    return ( currentBytes == expectedBytes );
  }


  /**
   * @brief Deduces the total memory consumption of the packet in bytes
   *
   * @param head      Start of the packet
   * @return size_t
   */
  size_t memoryConsumption( const MsgFrag *const head )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !head )
    {
      return 0;
    }

    /*-------------------------------------------------
    Walk the list and accumulate bytes
    -------------------------------------------------*/
    const MsgFrag *list = head;
    size_t byteSize     = 0;

    while ( list )
    {
      byteSize += sizeof( MsgFrag ) + list->fragmentLength;
      list = list->next;
    }

    return byteSize;
  }


  /*-------------------------------------------------------------------------------
  Debugging Utilities
  -------------------------------------------------------------------------------*/
  /**
   * @brief Prints the data fields of a complete fragmented packet
   *
   * @param head      Packet to print
   */
  void printPacketData( const MsgFrag *const head )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !Fragment::isValid( head ) )
    {
      LOG_ERROR( "Cannot print invalid packet\r\n" );
      return;
    }

    /*-------------------------------------------------
    Print out the data
    -------------------------------------------------*/
    const MsgFrag *data = head->next;

    char msgBuffer[ 512 ];

    auto bytes = scnprintf( msgBuffer, sizeof( msgBuffer ), "Fragment Data: " );
    while ( data )
    {
      for ( size_t byte = 0; byte < data->fragmentLength; byte++ )
      {
        bytes += scnprintf( msgBuffer + bytes, sizeof( msgBuffer ), "0x%02x ",
                            reinterpret_cast<const uint8_t *>( data->fragmentData )[ byte ] );
      }
      data = data->next;
    }

    LOG_INFO( "%s\r\n", msgBuffer );
  }

}    // namespace Ripple::Fragment
