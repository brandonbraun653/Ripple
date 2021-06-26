/********************************************************************************
 *  File Name:
 *    packet.cpp
 *
 *  Description:
 *    Implementation of a packet
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* STL Includes */
#include <utility>

/* ETL Includes */
#include <etl/random.h>

/* Aurora Includes */
#include <Aurora/logging>

/* Ripple Includes */
#include <Ripple/netstack>


namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr size_t DFLT_FRAG_SIZE = 24;

  /*-------------------------------------------------------------------------------
  Static Data
  -------------------------------------------------------------------------------*/
  static etl::random_xorshift s_rng;


  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Packet_sPtr allocPacket( INetMgr *const context )
  {
    s_rng.initialise( Chimera::millis() );

    Packet_sPtr local = Packet_sPtr( context );
    local->mContext   = context;

    return local;
  }


  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  Packet::Packet() : mContext( nullptr ), mFragmentationSize( DFLT_FRAG_SIZE )
  {
  }


  Packet::Packet( INetMgr *const context ) : mContext( context ), mFragmentationSize( DFLT_FRAG_SIZE )
  {
  }


  Packet::~Packet()
  {
  }


  void Packet::sort()
  {
    fragmentSort( &head );
  }


  bool Packet::pack( const void *const buffer, const size_t size )
  {
    /*-------------------------------------------------------------------------------
    Determine transfer fragmentation sizing/boundaries
    -------------------------------------------------------------------------------*/
    mTotalFragments         = 0; /**< Total fragments to be sent */
    size_t lastFragmentSize = 0; /**< Track remainder payload in last non-full fragment */

    if ( size <= mFragmentationSize )
    {
      /*-------------------------------------------------
      Single "fragment" lte to the reported fragment size
      -------------------------------------------------*/
      mTotalFragments  = 1;
      lastFragmentSize = size;
    }
    else
    {
      /*-------------------------------------------------
      Multiple fragments, possibly not aligned
      -------------------------------------------------*/
      mTotalFragments  = size / mFragmentationSize;
      lastFragmentSize = size % mFragmentationSize;

      if ( lastFragmentSize != 0 )
      {
        mTotalFragments += 1u;
      }
    }

    /*-------------------------------------------------
    Check that the number of fragments are supported by
    the underlying network interface.
    -------------------------------------------------*/
    if ( mTotalFragments > 32 )    // TODO: Redirect to the current netif
    {
      LOG_ERROR( "Packet too large. NetIf only supports %d fragments, but %d are needed.\r\n", 32, mTotalFragments );
      return Chimera::Status::NOT_SUPPORTED;
    }

    /*-------------------------------------------------------------------------------
    Validate memory requirements
    -------------------------------------------------------------------------------*/
    /*-------------------------------------------------
    Check if enough memory is available to handle the
    raw data + the fragment control structure overhead.
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<INetMgr> lock( *mContext );
    const size_t freeMem = mContext->availableMemory();

    /*-------------------------------------------------
    Calculate the expected memory consumption:

        User payload data
      + Payload for first fragment
      + Total num fragment structures
      --------------------------------------
      = allocation Size
    -------------------------------------------------*/
    const size_t allocationSize = size + ( Fragment_sPtr::size() * mTotalFragments );
    if ( freeMem <= allocationSize )
    {
      LOG_DEBUG( "Socket out of memory. Tried to allocate %d bytes from remaining %d\r\n", allocationSize, freeMem );
      return Chimera::Status::MEMORY;
    }

    /*-------------------------------------------------------------------------------
    Construct the fragment list from user data
    -------------------------------------------------------------------------------*/
    size_t bytesLeft          = size; /**< Bytes to pack into fragments */
    size_t dataOffset         = 0;    /**< Byte offset into the user data */
    const uint16_t randomUUID = s_rng() % std::numeric_limits<uint16_t>::max();

    for ( size_t fragCnt = 0; fragCnt < mTotalFragments; fragCnt++ )
    {
      /*-------------------------------------------------
      Determine how many bytes to allocate
      -------------------------------------------------*/
      const size_t fragmentDataSize = std::min( bytesLeft, mFragmentationSize );

      /*-------------------------------------------------
      Allocate a new fragment
      -------------------------------------------------*/
      auto newFrag    = allocFragment( mContext, fragmentDataSize );
      newFrag->length = fragmentDataSize;
      newFrag->uuid   = randomUUID;
      newFrag->number = fragCnt;

      void **data_buffer = newFrag->data.get();
      memcpy( *( data_buffer ), ( reinterpret_cast<const uint8_t *const>( buffer ) + dataOffset ), fragmentDataSize );

      /*-------------------------------------------------
      Insert the packet into the list
      -------------------------------------------------*/
      Fragment_sPtr previousHead = std::move( head );

      head       = newFrag;
      head->next = previousHead;

      /*-------------------------------------------------
      Update tracking data
      -------------------------------------------------*/
      bytesLeft -= fragmentDataSize;
      dataOffset += fragmentDataSize;
    }

    this->sort();

    return true;
  }


  bool Packet::unpack( void * buffer, const size_t size )
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
    Fragment_sPtr fragPtr = head;
    size_t offset = 0;

    while( fragPtr && ( offset < size ) )
    {
      void **data_ptr = fragPtr->data.get();
      uint8_t *dest   = reinterpret_cast<uint8_t *>( buffer ) + offset;

      memcpy( dest, *( data_ptr ), fragPtr->length );

      offset += fragPtr->length;
      fragPtr = fragPtr->next;
    }

    return ( !fragPtr && ( offset <= size ) );
  }


  size_t Packet::numFragments()
  {
    return mTotalFragments;
  }

  /**
   * @brief Deduces the total memory consumption of the packet in bytes
   *
   * @param head      Start of the packet
   * @return size_t
   */
  size_t Packet::size()
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
    Fragment_sPtr fragPtr = head;
    size_t byteSize     = 0;

    while ( fragPtr )
    {
      byteSize += fragPtr->length;
      fragPtr = fragPtr->next;
    }

    return byteSize;
  }


  uint16_t Packet::getUUID()
  {
    return head->uuid;
  }


  void Packet::printPayload()
  {
    /*-------------------------------------------------
    Print out the data
    -------------------------------------------------*/
    Fragment_sPtr fragPtr = head;

    char msgBuffer[ 512 ];

    auto bytes = scnprintf( msgBuffer, sizeof( msgBuffer ), "Fragment Data: " );
    while ( fragPtr )
    {
      for ( size_t byte = 0; byte < fragPtr->length; byte++ )
      {
        void ** raw_buffer = fragPtr->data.get();
        bytes += scnprintf( msgBuffer + bytes, sizeof( msgBuffer ) - bytes, "0x%02x ",
                            reinterpret_cast<const uint8_t *>( *raw_buffer )[ byte ] );
      }
      fragPtr = fragPtr->next;
    }

    LOG_INFO( "%s\r\n", msgBuffer );
  }

}    // namespace Ripple
