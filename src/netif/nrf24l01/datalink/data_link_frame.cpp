/********************************************************************************
 *  File Name:
 *    data_link_frame.cpp
 *
 *  Description:
 *    Implements framing utilities
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/utility>

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>

namespace Ripple::NetIf::NRF24::DataLink
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  Frame::Frame() :
      txAttempts( 0 ), nextHop( 0 ), receivedPipe( Physical::PipeNumber::PIPE_INVALID ),
      rtxCount( Physical::AutoRetransmitCount::ART_COUNT_INVALID ), rtxDelay( Physical::AutoRetransmitDelay::ART_DELAY_UNKNOWN )
  {
    /*-------------------------------------------------
    Reset the packed data fields to defaults
    -------------------------------------------------*/
    static_assert( std::is_trivial<PackedFrame>::value );
    static_assert( std::is_standard_layout<PackedFrame>::value );
    memset( &wireData, 0, sizeof( PackedFrame ) );

    wireData.control.version = CTRL_STRUCTURE_VERSION;
  }


  Frame::Frame( const Frame &other ) :
      txAttempts( other.txAttempts ), nextHop( other.nextHop ), receivedPipe( other.receivedPipe ), rtxCount( other.rtxCount ),
      rtxDelay( other.rtxDelay )
  {
    memcpy( &wireData, &other.wireData, sizeof( PackedFrame ) );
  }


  Frame::Frame( const Frame &&other ) :
      txAttempts( other.txAttempts ), nextHop( other.nextHop ), receivedPipe( other.receivedPipe ), rtxCount( other.rtxCount ),
      rtxDelay( other.rtxDelay )
  {
    memcpy( &wireData, &other.wireData, sizeof( PackedFrame ) );
  }


  void Frame::operator=( const Frame &other )
  {
    /*-------------------------------------------------
    This was required for integration with the
    FrameQueue ETL structure.
    -------------------------------------------------*/
    txAttempts   = other.txAttempts;
    nextHop      = other.nextHop;
    receivedPipe = other.receivedPipe;
    rtxCount     = other.rtxCount;
    rtxDelay     = other.rtxDelay;
    memcpy( &wireData, &other.wireData, sizeof( PackedFrame ) );
  }


  size_t Frame::writeUserData( const void *const data, const size_t size )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !data || !size || ( size > ARRAY_COUNT( PackedFrame::userData ) ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Clear out old data, copy in new data, set lengths
    -------------------------------------------------*/
    memset( wireData.userData, 0, sizeof( PackedFrame::userData ) );
    memcpy( wireData.userData, data, size );
    wireData.control.dataLength = size;

    return size;
  }


  size_t Frame::readUserData( void *const data, const size_t size )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if ( !data || !size || ( size > ARRAY_COUNT( PackedFrame::userData ) ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Copy out the data
    -------------------------------------------------*/
    size_t readLen = std::min<size_t>( size, wireData.control.dataLength );
    memcpy( data, wireData.userData, readLen );

    return readLen;
  }


  void Frame::pack( FrameBuffer &buffer )
  {
    memcpy( buffer.data(), &wireData, sizeof( PackedFrame ) );
    static_assert( sizeof( FrameBuffer ) == sizeof( PackedFrame ) );
  }


  void Frame::unpack( const FrameBuffer &buffer )
  {
    memcpy( &wireData, buffer.data(), sizeof( PackedFrame ) );
  }


  size_t Frame::size()
  {
    return sizeof( PackedFrame );
  }

}    // namespace Ripple::NetIf::NRF24::DataLink
