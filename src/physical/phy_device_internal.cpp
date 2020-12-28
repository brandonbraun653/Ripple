/********************************************************************************
 *  File Name:
 *    phy_device_internal.cpp
 *
 *  Description:
 *    Low level device driver interaction
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/event>
#include <Chimera/gpio>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/src/physical/phy_device_driver.hpp>
#include <Ripple/src/physical/phy_device_internal.hpp>
#include <Ripple/src/physical/phy_device_register.hpp>

namespace Ripple::PHY
{
  /*-------------------------------------------------------------------------------
  Private Functions
  -------------------------------------------------------------------------------*/
  Chimera::Status_t spiTransaction( DeviceHandle &handle, const void *const txBuffer, void *const rxBuffer,
                                    const size_t length )
  {
    using namespace Chimera::GPIO;
    using namespace Chimera::Threading;

    /*-------------------------------------------------
    Input protection. Ignore txBuffer/rxBuffer as they
    are allowed to be nullptrs depending on the type of
    transaction.
    -------------------------------------------------*/
    if ( !length )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    /*-------------------------------------------------
    Guarantee access to the device
    -------------------------------------------------*/
    handle.spi->lock();

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    handle.csPin->setState( State::LOW );
    auto spiResult = handle.spi->readWriteBytes( txBuffer, rxBuffer, length );
    handle.spi->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, TIMEOUT_BLOCK );
    handle.csPin->setState( State::HIGH );

    /*-------------------------------------------------
    Unlock the device
    -------------------------------------------------*/
    handle.spi->unlock();
    return spiResult;
  }


  uint8_t readRegister( DeviceHandle &handle, const uint8_t addr )
  {
    uint8_t tempBuffer = std::numeric_limits<uint8_t>::max();
    readRegister( handle, addr, &tempBuffer, sizeof( tempBuffer ) );
    return tempBuffer;
  }


  uint8_t readRegister( DeviceHandle &handle, const uint8_t addr, void *const buf, size_t len )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !buf || !len || ( len > MAX_SPI_DATA_LEN ) )
    {
      return 0;
    }

    /*------------------------------------------------
    Populate the read command
    ------------------------------------------------*/
    handle.txBuffer[ 0 ] = ( CMD_R_REGISTER | ( addr & CMD_REGISTER_MASK ) );
    memset( &handle.txBuffer[ 1 ], CMD_NOP, len );

    /*-------------------------------------------------
    Implement the command
    -------------------------------------------------*/
    if ( Chimera::Status::OK != spiTransaction( handle, handle.txBuffer, handle.rxBuffer, len + 1 ) )
    {
      Chimera::insert_debug_breakpoint();
      return 0;
    }
    else
    {
      /* Copy out the data fields */
      memcpy( buf, &handle.rxBuffer[ 1 ], len );

      /* Status register is in the first byte */
      return handle.rxBuffer[ 0 ];
    }
  }


  uint8_t writeRegister( DeviceHandle &handle, const uint8_t addr, const uint8_t value )
  {
    return writeRegister( handle, addr, &value, sizeof( value ) );
  }


  uint8_t writeRegister( DeviceHandle &handle, const uint8_t addr, const void *const buffer, size_t len )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !buffer || !len || ( len > MAX_SPI_DATA_LEN ) )
    {
      return 0;
    }

    /*------------------------------------------------
    Prepare the write command
    ------------------------------------------------*/
    handle.txBuffer[ 0 ] = ( CMD_W_REGISTER | ( addr & CMD_REGISTER_MASK ) );
    memcpy( &handle.txBuffer[ 1 ], buffer, len );

    /*-------------------------------------------------
    Implement the command
    -------------------------------------------------*/
    if ( Chimera::Status::OK != spiTransaction( handle, handle.txBuffer, handle.rxBuffer, len + 1 ) )
    {
      Chimera::insert_debug_breakpoint();
      return 0;
    }
    else
    {
      /* Status register is in the first byte */
      return handle.rxBuffer[ 0 ];
    }
  }


  uint8_t writeCommand( DeviceHandle &handle, const uint8_t cmd )
  {
    return writeCommand( handle, cmd, nullptr, 0 );
  }


  uint8_t writeCommand( DeviceHandle &handle, const uint8_t cmd, const void *const buffer, const size_t length )
  {
    /*-------------------------------------------------
    Input Protection. Buffer is allowed to be nullptr
    due to some commands not having data.
    -------------------------------------------------*/
    if ( length > MAX_SPI_DATA_LEN )
    {
      return 0;
    }

    /*-------------------------------------------------
    Structure the command
    -------------------------------------------------*/
    size_t cmdLen        = 1;
    handle.txBuffer[ 0 ] = cmd;
    if ( buffer )
    {
      memcpy( &handle.txBuffer[ 1 ], buffer, length );
      cmdLen += length;
    }

    /*-------------------------------------------------
    Send the transaction
    -------------------------------------------------*/
    if ( Chimera::Status::OK != spiTransaction( handle, handle.txBuffer, handle.rxBuffer, cmdLen ) )
    {
      Chimera::insert_debug_breakpoint();
      return 0;
    }
    else
    {
      /* Status register is in the first byte */
      return handle.rxBuffer[ 0 ];
    }
  }


  uint8_t readCommand( DeviceHandle &handle, const uint8_t cmd, void *const buffer, const size_t length )
  {
    /*-------------------------------------------------
    Input Protection. Buffer is allowed to be nullptr
    due to some commands not having data.
    -------------------------------------------------*/
    if ( !buffer || !length || length > MAX_SPI_DATA_LEN )
    {
      return 0;
    }

    /*-------------------------------------------------
    Structure the command sequence
    -------------------------------------------------*/
    size_t cmdLen        = 1;
    handle.txBuffer[ 0 ] = cmd;
    memset( &handle.txBuffer[ 1 ], CMD_NOP, ARRAY_BYTES( DeviceHandle::txBuffer ) - 1 );

    /*-------------------------------------------------
    Send the transaction
    -------------------------------------------------*/
    if ( Chimera::Status::OK != spiTransaction( handle, handle.txBuffer, handle.rxBuffer, cmdLen ) )
    {
      Chimera::insert_debug_breakpoint();
      return 0;
    }
    else
    {
      /* Copy out the payload data */
      memcpy( buffer, &handle.rxBuffer[ 1 ], length );

      /* Status register is in the first byte */
      return handle.rxBuffer[ 0 ];
    }
  }


  bool registerIsBitmaskSet( DeviceHandle &handle, const uint8_t reg, const uint8_t bitmask )
  {
    return ( readRegister( handle, reg ) & bitmask ) == bitmask;
  }


  bool registerIsAnySet( DeviceHandle &handle, const uint8_t reg, const uint8_t bitmask )
  {
    return readRegister( handle, reg ) & bitmask;
  }


  uint8_t setRegisterBits( DeviceHandle &handle, const uint8_t addr, const uint8_t mask )
  {
    uint8_t current = readRegister( handle, addr );
    current |= mask;
    return writeRegister( handle, addr, current );
  }


  uint8_t clrRegisterBits( DeviceHandle &handle, const uint8_t addr, const uint8_t mask )
  {
    uint8_t current = readRegister( handle, addr );
    current &= ~mask;
    return writeRegister( handle, addr, current );
  }

}    // namespace Ripple::PHY
