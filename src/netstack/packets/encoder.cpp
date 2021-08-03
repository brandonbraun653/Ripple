/********************************************************************************
 *  File Name:
 *    encoder.cpp
 *
 *  Description:
 *    Encoder implementation details
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/common>

/* ETL Includes */
#include <etl/crc32.h>

/* Project Includes */
#include <Ripple/netstack>


namespace Ripple::Transport
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Packet_sPtr constructPacket( Aurora::Memory::IHeapAllocator *const context, const TransportHeader &header, const void *const data, const size_t bytes )
  {
    /*-----------------------------------------------------------------
    Input protection
    -----------------------------------------------------------------*/
    if ( !context || !data || !bytes )
    {
      return Packet_sPtr();
    }

    /*-----------------------------------------------------------------
    Ensure exclusive access to the context manager
    -----------------------------------------------------------------*/
    Chimera::Thread::LockGuard lck( *context );

    /*-----------------------------------------------------------------
    Build the full packet in some scratch memory
    -----------------------------------------------------------------*/
    const size_t allocationSize = bytes + sizeof( TransportHeader );
    uint8_t *scratch            = reinterpret_cast<uint8_t *>( context->malloc( allocationSize ) );

    memset( scratch, 0, allocationSize );
    memcpy( scratch, &header, sizeof( TransportHeader ) );
    memcpy( scratch + sizeof( TransportHeader ), data, bytes );

    /*-----------------------------------------------------------------
    Add the CRC to the packet header
    -----------------------------------------------------------------*/
    etl::crc32 crc_gen;
    crc_gen.reset();
    crc_gen.add( scratch + offsetof( TransportHeader, dstPort ), scratch + allocationSize );

    auto hdr_ptr = reinterpret_cast<TransportHeader *>( scratch );
    hdr_ptr->crc = crc_gen.value();

    /*-----------------------------------------------------------------
    Pack the scratch memory into a newly created packet
    -----------------------------------------------------------------*/
    Packet_sPtr pkt = allocPacket( context );
    pkt->pack( scratch, allocationSize );

    /*-----------------------------------------------------------------
    Free the scratch memory now that we're done with it
    -----------------------------------------------------------------*/
    context->free( scratch );

    return pkt;
  }
}    // namespace Ripple::Transport
