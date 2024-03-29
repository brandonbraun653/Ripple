/********************************************************************************
 *  File Name:
 *    packet.cpp
 *
 *  Description:
 *    Implementation of a generic packet that may be transmitted over the Ripple
 *    network. Supports fragmentation.
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/*-----------------------------------------------------------------------------
Includes
-----------------------------------------------------------------------------*/
#include <Aurora/logging>
#include <Ripple/netstack>
#include <etl/random.h>
#include <limits>
#include <utility>


/*-----------------------------------------------------------------------------
Literals
-----------------------------------------------------------------------------*/
#define DEBUG_MODULE ( false )


namespace Ripple
{
  /*---------------------------------------------------------------------------
  Constants
  ---------------------------------------------------------------------------*/
  static constexpr size_t DFLT_FRAG_SIZE = 24;
  static constexpr size_t MAX_NUM_FRAGS  = 32;


  /*---------------------------------------------------------------------------
  Static Data
  ---------------------------------------------------------------------------*/
  static etl::random_xorshift s_rng;


  /*---------------------------------------------------------------------------
  Public Functions
  ---------------------------------------------------------------------------*/
  Packet_sPtr allocPacket( Aurora::Memory::IHeapAllocator *const context )
  {
    s_rng.initialise( Chimera::millis() );

    Packet_sPtr local = Packet_sPtr( context );
    local->mContext   = context;

    return local;
  }


  /*---------------------------------------------------------------------------
  Classes
  ---------------------------------------------------------------------------*/
  Packet::Packet() : mContext( nullptr ), mFragmentationSize( DFLT_FRAG_SIZE ), mTotalFragments( 0 )
  {
  }


  Packet::Packet( Aurora::Memory::IHeapAllocator *const context ) :
      mContext( context ), mFragmentationSize( DFLT_FRAG_SIZE ), mTotalFragments( 0 )
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
    if ( mTotalFragments > MAX_NUM_FRAGS )    // TODO: Redirect to the current netif
    {
      LOG_ERROR( "Packet too large. NetIf only supports %d fragments, but %d are needed.\r\n", MAX_NUM_FRAGS, mTotalFragments );
      return Chimera::Status::NOT_SUPPORTED;
    }

    /*-------------------------------------------------------------------------------
    Validate memory requirements
    -------------------------------------------------------------------------------*/
    /*-------------------------------------------------
    Check if enough memory is available to handle the
    raw data + the fragment control structure overhead.
    -------------------------------------------------*/
    Chimera::Thread::LockGuard<Aurora::Memory::IHeapAllocator> lock( *mContext );
    const size_t freeMem = mContext->available();

    /*-------------------------------------------------
    Calculate the expected memory consumption:

        User payload data
      + Payload for first fragment
      + Total num fragment structures
      --------------------------------------
      = allocation Size
    -------------------------------------------------*/
    const size_t allocationSize = size + ( Fragment_sPtr().size() * mTotalFragments );
    if ( freeMem <= allocationSize )
    {
      LOG_DEBUG( "Out of memory. Tried to allocate %d bytes from remaining %d\r\n", allocationSize, freeMem );
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
      newFrag->total  = mTotalFragments;

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


  bool Packet::unpack( void *buffer, const size_t size ) const
  {
    /*-------------------------------------------------------------------------
    Input Protection
    -------------------------------------------------------------------------*/
    if ( !head || !buffer || !size )
    {
      return false;
    }

    /*-------------------------------------------------------------------------
    Copy the data over
    -------------------------------------------------------------------------*/
    Chimera::Thread::LockGuard<Aurora::Memory::IHeapAllocator> lock( *mContext );

    const Fragment_sPtr *fragPtr = &head;
    size_t offset                = 0;

    while ( *fragPtr && ( offset < size ) )
    {
      /* Get the source and destination pointers */
      void **src    = ( *fragPtr )->data.get();
      uint8_t *dest = reinterpret_cast<uint8_t *>( buffer ) + offset;

      /* Move the data over */
      memcpy( dest, *( src ), ( *fragPtr )->length );

      /* Update trackers */
      offset += ( *fragPtr )->length;   // Offset into user buffer
      fragPtr = &( *fragPtr )->next;    // Next fragment to pull from
    }

    /*-------------------------------------------------------------------------
    Fragment is null (end of list) and some data was copied.
    -------------------------------------------------------------------------*/
    return ( !( *fragPtr ) && ( offset <= size ) );
  }


  size_t Packet::numFragments() const
  {
    return mTotalFragments;
  }


  size_t Packet::size() const
  {
    /*-------------------------------------------------------------------------
    Input Protection
    -------------------------------------------------------------------------*/
    if ( !head )
    {
      return 0;
    }

    /*-------------------------------------------------------------------------
    Walk the list and accumulate bytes
    -------------------------------------------------------------------------*/
    const Fragment_sPtr *fragPtr = &head;
    size_t byteSize              = 0;

    while ( *fragPtr )
    {
      byteSize += ( *fragPtr )->length;
      fragPtr = &( *fragPtr )->next;
    }

    return byteSize;
  }


  uint16_t Packet::getUUID() const
  {
    return head ? head->uuid : 0;
  }


  bool Packet::isMissingFragments() const
  {
    static_assert( MAX_NUM_FRAGS % sizeof( uint8_t ) == 0 );
    static constexpr size_t FLAG_BIT = std::numeric_limits<uint8_t>::digits;

    /*-------------------------------------------------------------------------
    Cache a few values for performance
    -------------------------------------------------------------------------*/
    const auto totalFrags = head->total;

    /*-------------------------------------------------------------------------
    Use individual bits to mark a fragment as received. Must check uniqueness
    of a fragment, not just that X number of fragments are in the packet.
    -------------------------------------------------------------------------*/
    uint8_t flagBits[ MAX_NUM_FRAGS / FLAG_BIT ];
    memset( flagBits, 0, sizeof( flagBits ) );

    static constexpr size_t SEARCHABLE_BITS = sizeof( flagBits ) * FLAG_BIT;

    /*-------------------------------------------------------------------------
    Verify fragment sizing constraints
    -------------------------------------------------------------------------*/
    if ( ( totalFrags > SEARCHABLE_BITS ) || ( this->numFragments() > SEARCHABLE_BITS ) )
    {
      LOG_ERROR_IF( DEBUG_MODULE, "Too many fragments exist [%d] to determine completeness\r\n", totalFrags );
      return false;
    }

    /*-------------------------------------------------------------------------
    Walk each fragment that exists
    -------------------------------------------------------------------------*/
    const Fragment_sPtr *fragPtr = &this->head;
    while ( *fragPtr )
    {
      /*-----------------------------------------------------------------------
      Check for out of bounds values
      -----------------------------------------------------------------------*/
      if ( ( *fragPtr )->number >= totalFrags )
      {
        LOG_ERROR( "Packet corruption. Reported fragment number [%d] is greater than total [%d].\r\n", ( *fragPtr )->number,
                   head->total );
      }

      /*-----------------------------------------------------------------------
      Mark the appropriate bit
      -----------------------------------------------------------------------*/
      const size_t index  = ( *fragPtr )->number / FLAG_BIT;
      const size_t offset = ( *fragPtr )->number - ( index * FLAG_BIT );

      flagBits[ index ] |= ( 1u << offset );
      fragPtr = &( *fragPtr )->next;
    }

    /*-------------------------------------------------------------------------
    Are all fragments present?
    -------------------------------------------------------------------------*/
    size_t countedFragments = 0;
    for ( size_t bitNumber = 0; ( bitNumber < SEARCHABLE_BITS ) && ( bitNumber < totalFrags ); bitNumber++ )
    {
      const size_t index  = bitNumber / FLAG_BIT;
      const size_t offset = bitNumber - ( index * FLAG_BIT );

      if ( ( flagBits[ index ] >> offset ) & 0x1 )
      {
        countedFragments++;
      }
    }

    return !( countedFragments == head->total );
  }


  bool Packet::isUniform() const
  {
    const auto exp               = this->getUUID();
    const Fragment_sPtr *fragPtr = &this->head;

    while ( *fragPtr )
    {
      if ( exp != ( *fragPtr )->uuid )
      {
        return false;
      }
      fragPtr = &( *fragPtr )->next;
    }

    return true;
  }


  bool Packet::isSorted() const
  {
    const Fragment_sPtr *fragPtr = &this->head;
    size_t currentIdx            = 0;

    while ( *fragPtr )
    {
      if ( currentIdx != ( *fragPtr )->number )
      {
        return false;
      }
      fragPtr = &( *fragPtr )->next;
    }

    return true;
  }


  bool Packet::isFullyComposed() const
  {
    /*-------------------------------------------------------------------------
    A packet has been fully composed when:
      1. No fragments are missing from the full list
      2. Each fragment belongs to the same transaction (UUID)
      3. The whole list has been sorted
      4. The first fragment contains the header for the transport layer
    -------------------------------------------------------------------------*/
    return !isMissingFragments() && isUniform() && isSorted() && ( this->head->length >= sizeof( TransportHeader ) );
  }


  void Packet::printPayload() const
  {
    char msgBuffer[ 512 ];
    const Fragment_sPtr *fragPtr = &this->head;
    auto bytes                   = scnprintf( msgBuffer, sizeof( msgBuffer ), "Fragment Data: " );

    while ( *fragPtr )
    {
      for ( size_t byte = 0; byte < ( *fragPtr )->length; byte++ )
      {
        void **raw_buffer = ( *fragPtr )->data.get();
        bytes += scnprintf( msgBuffer + bytes, sizeof( msgBuffer ) - bytes, "0x%02x ",
                            reinterpret_cast<const uint8_t *>( *raw_buffer )[ byte ] );
      }
      fragPtr = &( *fragPtr )->next;
    }

    LOG_INFO( "%s\r\n", msgBuffer );
  }


  bool Packet::checkReferences( const size_t number )
  {
    Fragment_sPtr *frag = &this->head;
    while ( *frag )
    {
      if ( frag->references() != number )
      {
        return false;
      }

      frag = &( *frag )->next;
    }

    return true;
  }

}    // namespace Ripple
