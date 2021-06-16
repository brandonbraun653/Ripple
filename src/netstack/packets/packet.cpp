/********************************************************************************
 *  File Name:
 *    packet.cpp
 *
 *  Description:
 *    Implementation of a packet
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/


/* Ripple Includes */
#include <Ripple/netstack>


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Packet_sPtr allocPacket( INetMgr *const context )
  {
    return Packet_sPtr();
  }


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  Packet::Packet()
  {
  }


  Packet::Packet( INetMgr *const context ) : mContext( context )
  {
  }


  Packet::~Packet()
  {

  }


  bool Packet::isValid()
  {
    return false;
  }


  void Packet::sort()
  {
  }


  void Packet::updateCRC()
  {
  }


  bool Packet::pack( const void *const buffer, const size_t size )
  {
    // /*-------------------------------------------------------------------------------
    // Determine transfer fragmentation sizing/boundaries
    // -------------------------------------------------------------------------------*/
    // /*-------------------------------------------------
    // Figure out the fragment transfer size that can be
    // handled by the network layer.
    // -------------------------------------------------*/
    // size_t maxPayload = mContext->mNetIf->maxTransferSize();
    // if ( !maxPayload )
    // {
    //   return Chimera::Status::FAIL;
    // }

    // /*-------------------------------------------------
    // Figure out the fragment boundaries
    // -------------------------------------------------*/
    // size_t numFragments     = 0; /**< Total fragments to be sent */
    // size_t lastFragmentSize = 0; /**< Track remainder payload in last non-full fragment */

    // if ( bytes <= maxPayload )
    // {
    //   /*-------------------------------------------------
    //   Single "fragment" lte to the reported fragment size
    //   -------------------------------------------------*/
    //   numFragments     = 1;
    //   lastFragmentSize = bytes;
    // }
    // else
    // {
    //   /*-------------------------------------------------
    //   Multiple fragments, possibly not aligned
    //   -------------------------------------------------*/
    //   numFragments     = bytes / maxPayload;
    //   lastFragmentSize = bytes % maxPayload;

    //   if ( lastFragmentSize != 0 )
    //   {
    //     numFragments += 1u;
    //   }
    // }

    // /*-------------------------------------------------
    // Increment the fragment number to account for the
    // first fragment always being a packet header.
    // -------------------------------------------------*/
    // numFragments++;

    // /*-------------------------------------------------
    // Check that the number of fragments are supported by
    // the underlying network interface.
    // -------------------------------------------------*/
    // if( numFragments > mContext->mNetIf->maxNumFragments() )
    // {
    //   LOG_ERROR( "Packet too large. NetIf only supports %d fragments, but %d are needed.\r\n",
    //              mContext->mNetIf->maxNumFragments(), numFragments );
    //   return Chimera::Status::NOT_SUPPORTED;
    // }

    // /*-------------------------------------------------------------------------------
    // Validate memory requirements
    // -------------------------------------------------------------------------------*/
    // /*-------------------------------------------------
    // Make sure the TX queue can handle another message
    // -------------------------------------------------*/
    // Chimera::Thread::LockGuard lck( *this );
    // if ( mTXQueue.full() )
    // {
    //   return Chimera::Status::FULL;
    // }

    // /*-------------------------------------------------
    // Check if enough memory is available to handle the
    // raw data + the fragment control structure overhead.
    // -------------------------------------------------*/
    // Chimera::Thread::LockGuard<Context> lock( *mContext );
    // const size_t freeMem = mContext->availableMemory();

    // /*-------------------------------------------------
    // Calculate the expected memory consumption:

    //     User payload data
    //   + Payload for first fragment
    //   + Total num fragment structures
    //   --------------------------------------
    //   = allocation Size
    // -------------------------------------------------*/
    // const size_t allocationSize = bytes + sizeof( TransportHeader ) + ( sizeof( MsgFrag ) * numFragments );
    // if ( freeMem <= allocationSize )
    // {
    //   LOG_DEBUG( "Socket out of memory. Tried to allocate %d bytes from remaining %d\r\n", allocationSize, freeMem );
    //   return Chimera::Status::MEMORY;
    // }

    // /*-------------------------------------------------
    // Allocate the packet memory and initialize it
    // -------------------------------------------------*/
    // uint8_t *bytePool = reinterpret_cast<uint8_t *>( mContext->malloc( allocationSize ) );
    // if ( !bytePool )
    // {
    //   return Chimera::Status::FAIL;
    // }

    // /* If we see this "BaD" value in memory, something may have gone wrong... */
    // memset( bytePool, 0xBD, allocationSize );

    // /*-------------------------------------------------
    // Helper value to double check later that memory ops
    // didn't over/under run their expected behavior.
    // -------------------------------------------------*/
    // const std::uintptr_t finalMemAddr = reinterpret_cast<std::uintptr_t>( bytePool ) + allocationSize;

    // /*-------------------------------------------------------------------------------
    // Construct the header
    // -------------------------------------------------------------------------------*/
    // TransportHeader header;
    // header.crc        = 0;
    // header.dataLength = sizeof( TransportHeader ) + bytes;
    // header.dstPort    = mDestPort;
    // header.srcPort    = mThisPort;
    // header.srcAddress = mContext->getIPAddress();
    // header._pad       = 0;

    // /*-------------------------------------------------------------------------------
    // Initialize first message fragment (Header)
    // -------------------------------------------------------------------------------*/
    // const uint16_t randomUUID     = s_rng() % std::numeric_limits<uint16_t>::max();
    // const uint32_t pktDataLength  = sizeof( TransportHeader ) + bytes;

    // /*-------------------------------------------------
    // Construct fragment from the recent allocation
    // -------------------------------------------------*/
    // MsgFrag *msg = new ( bytePool ) MsgFrag();
    // bytePool += sizeof( MsgFrag );

    // /*-------------------------------------------------
    // Fill the fragment with packet header data
    // -------------------------------------------------*/
    // msg->fragmentData   = bytePool;
    // msg->fragmentLength = sizeof( TransportHeader );
    // msg->totalLength    = pktDataLength;
    // msg->totalFragments = numFragments;
    // msg->uuid           = randomUUID;

    // memcpy( msg->fragmentData, &header, sizeof( TransportHeader ) );
    // bytePool += sizeof( TransportHeader );

    // /*-------------------------------------------------
    // Initialize the linked list
    // -------------------------------------------------*/
    // MsgFrag *rootMsg = msg;

    // /*-------------------------------------------------------------------------------
    // Construct the fragment list from user data
    // -------------------------------------------------------------------------------*/
    // size_t bytesLeft          = bytes;   /**< Bytes to pack into fragments */
    // MsgFrag *lastFragPtr      = msg;     /**< Ptr to last valid fragment */
    // size_t dataOffset         = 0;       /**< Byte offset into the user data */

    // /*-------------------------------------------------
    // Fill in the remaining fragments. Start at 1 because
    // the packet header has already been created.
    // -------------------------------------------------*/
    // for ( size_t fragCnt = 1; fragCnt < numFragments; fragCnt++ )
    // {
    //   /*-------------------------------------------------
    //   Construct the structure holding the message
    //   -------------------------------------------------*/
    //   msg = new ( bytePool ) MsgFrag();
    //   bytePool += sizeof( MsgFrag );
    //   RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( bytePool ) <= finalMemAddr );

    //   /*-------------------------------------------------
    //   Link the fragment list, unless this is the last one
    //   -------------------------------------------------*/
    //   msg->next = nullptr;
    //   if ( lastFragPtr && ( fragCnt != numFragments ) )
    //   {
    //     lastFragPtr->next = msg;
    //   }
    //   lastFragPtr = msg;

    //   /*-------------------------------------------------
    //   Fill out a few control fields
    //   -------------------------------------------------*/
    //   msg->fragmentNumber = fragCnt;
    //   msg->totalLength    = pktDataLength;
    //   msg->totalFragments = numFragments;
    //   msg->uuid           = randomUUID;

    //   /*-------------------------------------------------
    //   Decide the number of bytes going into this transfer
    //   -------------------------------------------------*/
    //   const size_t payloadSize = std::min( bytesLeft, maxPayload );

    //   /*-------------------------------------------------
    //   Copy in the user data
    //   -------------------------------------------------*/
    //   msg->fragmentData   = bytePool;
    //   msg->fragmentLength = payloadSize;
    //   memcpy( msg->fragmentData, ( reinterpret_cast<const uint8_t *const>( data ) + dataOffset ), payloadSize );

    //   /*-------------------------------------------------
    //   Update tracking data
    //   -------------------------------------------------*/
    //   bytePool += payloadSize;
    //   bytesLeft -= payloadSize;
    //   dataOffset += payloadSize;

    //   RT_HARD_ASSERT( reinterpret_cast<std::uintptr_t>( bytePool ) <= finalMemAddr );
    // }

    // /*-------------------------------------------------
    // Check exit conditions. Either of these failing
    // means a bug is present above.
    // -------------------------------------------------*/
    // RT_HARD_ASSERT( bytesLeft == 0 );
    // RT_HARD_ASSERT( finalMemAddr == reinterpret_cast<std::uintptr_t>( bytePool ) );

    // /*-------------------------------------------------------------------------------
    // Calculate the CRC over the entire message
    // -------------------------------------------------------------------------------*/
    // auto crc = TMPFragment::calcCRC32( rootMsg );
    // TMPFragment::setCRC32( rootMsg, crc );

    return false;
  }


  bool Packet::unpack( void *const buffer, const size_t size )
  {
    //   /*-------------------------------------------------
    //   Input Protection
    //   -------------------------------------------------*/
    //   if ( !head || !buffer || !size )
    //   {
    //     return false;
    //   }

    //   /*-------------------------------------------------
    //   Copy the data over
    //   -------------------------------------------------*/
    //   const MsgFrag * fragPtr = head;
    //   size_t offset = 0;

    //   while( fragPtr && ( offset < size ) )
    //   {
    //     memcpy( reinterpret_cast<uint8_t *const>( buffer ) + offset, fragPtr->fragmentData, fragPtr->fragmentLength );
    //     offset += fragPtr->fragmentLength;
    //     fragPtr = fragPtr->next;
    //   }

    //   return ( !fragPtr && ( offset <= size ) );
    return false;
  }


  size_t Packet::numFragments()
  {
    return 0;
  }


  size_t Packet::size()
  {
    return 0;
  }


  size_t Packet::maxSize()
  {
    return 0;
  }


  uint32_t Packet::readCRC()
  {
    return 0;
  }


  uint32_t Packet::calcCRC()
  {
    return 0;
  }


  uint16_t Packet::getUUID()
  {
    return 0;
  }

}    // namespace Ripple
