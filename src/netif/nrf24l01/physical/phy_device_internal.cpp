/********************************************************************************
 *  File Name:
 *    phy_device_internal.cpp
 *
 *  Description:
 *    Low level device driver interaction
 *
 *  2020-2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/event>
#include <Chimera/gpio>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/netif/nrf24l01>
#include <Ripple/src/netif/nrf24l01/physical/phy_device_internal.hpp>

namespace Ripple::NetIf::NRF24::Physical
{
  /*-------------------------------------------------------------------------------
  Private Functions
  -------------------------------------------------------------------------------*/
  Chimera::Status_t powerUpDrivers( Handle &handle )
  {
    using namespace Chimera::GPIO;

    Chimera::Status_t initOk = Chimera::Status::OK;

    /*-------------------------------------------------
    Configure the Chip Enable pin
    -------------------------------------------------*/
    handle.cePin = Chimera::GPIO::getDriver( handle.cfg.ce.port, handle.cfg.ce.pin );
    if( !handle.cePin )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    initOk |= handle.cePin->init( handle.cfg.ce );
    handle.cePin->setState( State::HIGH );

    /*-------------------------------------------------
    Configure the IRQ pin
    -------------------------------------------------*/
    handle.irqPin = Chimera::GPIO::getDriver( handle.cfg.irq.port, handle.cfg.irq.pin );
    if( !handle.irqPin )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    initOk |= handle.irqPin->init( handle.cfg.irq );
    handle.irqPin->setState( State::HIGH );

    /*-------------------------------------------------
    Configure the Chip Select pin
    -------------------------------------------------*/
    handle.csPin = Chimera::GPIO::getDriver( handle.cfg.spi.CSInit.port, handle.cfg.spi.CSInit.pin );
    if( !handle.csPin )
    {
      return Chimera::Status::INVAL_FUNC_PARAM;
    }

    initOk |= handle.csPin->init( handle.cfg.spi.CSInit );
    handle.csPin->setState( State::HIGH );

    /*-------------------------------------------------
    Configure the SPI driver
    -------------------------------------------------*/
    handle.spi = Chimera::SPI::getDriver( handle.cfg.spi.HWInit.hwChannel );
    initOk |= handle.spi->init( handle.cfg.spi );

    return initOk;
  }


  Chimera::Status_t spiTransaction( Handle &handle, const void *const txBuffer, void *const rxBuffer,
                                    const size_t length )
  {
    using namespace Chimera::GPIO;
    using namespace Chimera::Thread;

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


  uint8_t readRegister( Handle &handle, const uint8_t addr )
  {
    uint8_t tempBuffer = std::numeric_limits<uint8_t>::max();
    readRegister( handle, addr, &tempBuffer, sizeof( tempBuffer ) );
    return tempBuffer;
  }


  StatusReg_t readRegister( Handle &handle, const uint8_t addr, void *const buf, size_t len )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !buf || !len || ( len > MAX_SPI_DATA_LEN ) )
    {
      return INVALID_STATUS_REG;
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
      return INVALID_STATUS_REG;
    }
    else
    {
      /* Copy out the data fields */
      memcpy( buf, &handle.rxBuffer[ 1 ], len );

      /* Status register is in the first byte */
      handle.lastStatus = handle.rxBuffer[ 0 ];
      return handle.lastStatus;
    }
  }


  StatusReg_t writeRegister( Handle &handle, const uint8_t addr, const uint8_t value )
  {
    return writeRegister( handle, addr, &value, sizeof( value ) );
  }


  StatusReg_t writeRegister( Handle &handle, const uint8_t addr, const void *const buffer, size_t len )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !buffer || !len || ( len > MAX_SPI_DATA_LEN ) )
    {
      return INVALID_STATUS_REG;
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
      return INVALID_STATUS_REG;
    }
    else
    {
      /* Status register is in the first byte */
      handle.lastStatus = handle.rxBuffer[ 0 ];
    }

    /*-------------------------------------------------
    Should the last write be double checked?
    -------------------------------------------------*/
    if ( handle.cfg.verifyRegisters && ( addr != REG_ADDR_STATUS ) )
    {
      uint8_t tmpBuffer[ MAX_SPI_TRANSACTION_LEN ];
      memset( tmpBuffer, 0, MAX_SPI_TRANSACTION_LEN );

      readRegister( handle, addr, tmpBuffer, len );

      if( memcmp( tmpBuffer, buffer, len ) != 0 )
      {
        return INVALID_STATUS_REG;
      }

      static_assert( ARRAY_BYTES( tmpBuffer ) == ARRAY_BYTES( Handle::rxBuffer ) );
    }

    return handle.lastStatus;
  }


  StatusReg_t writeCommand( Handle &handle, const uint8_t cmd )
  {
    return writeCommand( handle, cmd, nullptr, 0 );
  }


  StatusReg_t writeCommand( Handle &handle, const uint8_t cmd, const void *const buffer, const size_t length )
  {
    /*-------------------------------------------------
    Input Protection. Buffer is allowed to be nullptr
    due to some commands not having data.
    -------------------------------------------------*/
    if ( length > MAX_SPI_DATA_LEN )
    {
      return INVALID_STATUS_REG;
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
      return INVALID_STATUS_REG;
    }
    else
    {
      /* Status register is in the first byte */
      handle.lastStatus = handle.rxBuffer[ 0 ];
      return handle.lastStatus;
    }
  }


  StatusReg_t readCommand( Handle &handle, const uint8_t cmd, void *const buffer, const size_t length )
  {
    /*-------------------------------------------------
    Input Protection. Buffer is allowed to be nullptr
    due to some commands not having data.
    -------------------------------------------------*/
    if ( !buffer || !length || length > MAX_SPI_DATA_LEN )
    {
      return INVALID_STATUS_REG;
    }

    /*-------------------------------------------------
    Structure the command sequence
    -------------------------------------------------*/
    size_t cmdLen        = 1 + length;  // +1 for the command byte
    handle.txBuffer[ 0 ] = cmd;
    memset( &handle.txBuffer[ 1 ], CMD_NOP, ARRAY_BYTES( Handle::txBuffer ) - 1 );

    /*-------------------------------------------------
    Send the transaction
    -------------------------------------------------*/
    if ( Chimera::Status::OK != spiTransaction( handle, handle.txBuffer, handle.rxBuffer, cmdLen ) )
    {
      Chimera::insert_debug_breakpoint();
      return INVALID_STATUS_REG;
    }
    else
    {
      /* Copy out the payload data */
      memcpy( buffer, &handle.rxBuffer[ 1 ], length );

      /* Status register is in the first byte */
      handle.lastStatus = handle.rxBuffer[ 0 ];
      return handle.lastStatus;
    }
  }


  bool registerIsBitmaskSet( Handle &handle, const uint8_t reg, const uint8_t bitmask )
  {
    return ( readRegister( handle, reg ) & bitmask ) == bitmask;
  }


  bool registerIsAnySet( Handle &handle, const uint8_t reg, const uint8_t bitmask )
  {
    return readRegister( handle, reg ) & bitmask;
  }


  StatusReg_t setRegisterBits( Handle &handle, const uint8_t addr, const uint8_t mask )
  {
    uint8_t current = readRegister( handle, addr );
    current |= mask;
    return writeRegister( handle, addr, current );
  }


  StatusReg_t clrRegisterBits( Handle &handle, const uint8_t addr, const uint8_t mask )
  {
    uint8_t current = readRegister( handle, addr );
    current &= ~mask;
    return writeRegister( handle, addr, current );
  }

}    // namespace Ripple::Physical
