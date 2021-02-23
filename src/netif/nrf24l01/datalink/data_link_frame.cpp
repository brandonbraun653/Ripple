/********************************************************************************
 *  File Name:
 *    data_link_frame.cpp
 *
 *  Description:
 *    Implements framing utilities
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>

namespace Ripple::NetIf::NRF24::DataLink
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  Frame::Frame() :
      nextHop( 0 ), receivedPipe( Physical::PipeNumber::PIPE_INVALID ),
      rtxCount( Physical::AutoRetransmitCount::ART_COUNT_INVALID ), rtxDelay( Physical::AutoRetransmitDelay::ART_DELAY_UNKNOWN )
  {
    /*-------------------------------------------------
    Reset the packed data fields to defaults
    -------------------------------------------------*/
    static_assert( std::is_pod<PackedFrame>::value );
    memset( &wireData, 0, sizeof( PackedFrame ) );

    wireData.control.version = CTRL_STRUCTURE_VERSION;
  }


  Frame::Frame( const Frame &other ) :
      nextHop( other.nextHop ), receivedPipe( other.receivedPipe ), rtxCount( other.rtxCount ), rtxDelay( other.rtxDelay )
  {
    memcpy( &wireData, &other.wireData, sizeof( PackedFrame ) );
  }


  Frame::Frame( const Frame &&other ) :
      nextHop( other.nextHop ), receivedPipe( other.receivedPipe ), rtxCount( other.rtxCount ), rtxDelay( other.rtxDelay )
  {
    memcpy( &wireData, &other.wireData, sizeof( PackedFrame ) );
  }


  void Frame::operator=( const Frame &other )
  {
    /*-------------------------------------------------
    This was required for integration with the
    FrameQueue ETL structure.
    -------------------------------------------------*/
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
    if( !data || !size || ( size > ARRAY_COUNT( PackedFrame::userData ) ) )
    {
      return 0;
    }

    /*-------------------------------------------------
    Clear out old data, copy in new data, set lengths
    -------------------------------------------------*/
    memset( wireData.userData, 0, sizeof( PackedFrame::userData ) );
    memcpy( wireData.userData, data, size );
    wireData.control.dataLength  = size;
    wireData.control.frameLength = sizeof( PackedFrame ) - sizeof( PackedFrame::userData ) + size ;

    return size;
  }


  size_t Frame::readUserData( void *const data, const size_t size )
  {
    /*-------------------------------------------------
    Input protection
    -------------------------------------------------*/
    if( !data || !size || ( size > ARRAY_COUNT( PackedFrame::userData ) ) )
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

}  // namespace Ripple::NetIf::NRF24::DataLink
