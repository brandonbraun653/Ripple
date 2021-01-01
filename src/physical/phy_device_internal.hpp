/********************************************************************************
 *  File Name:
 *    phy_device_internal.hpp
 *
 *  Description:
 *    Internal functions that should only be inside the PHY layer
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLY_PHYSICAL_INTERNAL_HPP
#define RIPPLY_PHYSICAL_INTERNAL_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

/* Ripple Includes */
#include <Ripple/src/physical/phy_device_types.hpp>

namespace Ripple::PHY
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using StatusReg_t = uint8_t;

  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  /**
   *  The status register's bit 7 should only be zero when reading from the
   *  device, so it makes a convenient "inavlid" flag for commands that return
   *  the status register.
   */
  static constexpr StatusReg_t INVALID_STATUS_REG = ( 1u << 7 );

  /*-------------------------------------------------------------------------------
  Private Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Perform a single SPI transaction with the device
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  txBuffer    Buffer for data to be transmitted
   *  @param[out] rxBuffer    Buffer for received data to be placed
   *  @param[in]  length      Number of bytes in the transaction
   *  @return Chimera::Status_t
   */
  Chimera::Status_t spiTransaction( Handle &handle, const void *const txBuffer, void *const rxBuffer,
                                    const size_t length );

  /**
   *  Reads a register on the device and returns the current value of that register
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  addr    The address of the register to read
   *  @return uint8_t
   */
  uint8_t readRegister( Handle &handle, const uint8_t addr );

  /**
   *  Reads a multibyte register into a buffer
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  addr        The address of the register to read
   *  @param[out] buf         The buffer to read into
   *  @param[in]  len         The total number of bytes to read from the register
   *  @return StatusReg_t
   */
  StatusReg_t readRegister( Handle &handle, const uint8_t addr, void *const buf, size_t len );

  /**
   *  Writes a register on the device with a given value
   *
   *  @warning  This operation overwrites the entire register
   *  @note     Validation only works in debug builds
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  addr        The address of the register to write
   *  @param[in]  value       The data to write to that register
   *  @return StatusReg_t
   */
  StatusReg_t writeRegister( Handle &handle, const uint8_t addr, const uint8_t value );

  /**
   *  Writes a register on the device with multiple bytes
   *
   *  @warning  This operation overwrites the entire register
   *  @note     Validation only works in debug builds
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  addr        The address of the register to write
   *  @param[in]  buffer      The data to write to that register
   *  @param[in]  len         The number of bytes to write from the buffer
   *  @return StatusReg_t
   */
  StatusReg_t writeRegister( Handle &handle, const uint8_t addr, const void *const buffer, size_t len );

  /**
   *  Writes a single byte command to the device
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  cmd         The command to write
   *  @return StatusReg_t
   */
  StatusReg_t writeCommand( Handle &handle, const uint8_t cmd );

  /**
   *  Writes a mult-byte command to the device
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  cmd         The command to write
   *  @param[in]  buffer      Data payload of the command
   *  @param[in]  length      Number of bytes in the data buffer
   *  @return StatusReg_t
   */
  StatusReg_t writeCommand( Handle &handle, const uint8_t cmd, const void *const buffer, const size_t length );

  /**
   *  Sends a read command to the device and returns a number of bytes
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  cmd         The command to write
   *  @param[out] buffer      Data buffer to place read bytes into
   *  @param[in]  length      Number of bytes to read. Buffer must be able to hold this amount.
   *  @return StatusReg_t
   */
  StatusReg_t readCommand( Handle &handle, const uint8_t cmd, void *const buffer, const size_t length );

  /**
   *  Checks if the bit mask is set on the given register
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  reg         Which register to check
   *  @param[in]  bitmask     Bit mask to validate
   *  @return bool
   */
  bool registerIsBitmaskSet( Handle &handle, const uint8_t reg, const uint8_t bitmask );

  /**
   *  Checks if any flag in the bit mask is set on the given register
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  reg         Which register to check
   *  @param[in]  bitmask     Bit mask to validate
   *  @return bool
   */
  bool registerIsAnySet( Handle &handle, const uint8_t reg, const uint8_t bitmask );

  /**
   *  Performs a read/modify/write operation on a register to set specific bits
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  addr        The address of the register to write
   *  @param[in]  mask        Mask of which bits should be set
   *  @return uint8_t         The chip's status register
   */
  StatusReg_t setRegisterBits( Handle &handle, const uint8_t addr, const uint8_t mask );

  /**
   *  Performs a read/modify/write operation on a register to clear specific bits
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  addr        The address of the register to write
   *  @param[in]  mask        Mask of which bits should be cleared
   *  @return uint8_t         The chip's status register
   */
  StatusReg_t clrRegisterBits( Handle &handle, const uint8_t addr, const uint8_t mask );

}    // namespace Ripple::PHY

#endif /* !RIPPLY_PHYSICAL_INTERNAL_HPP */
