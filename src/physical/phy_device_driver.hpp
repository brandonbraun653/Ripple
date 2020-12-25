/********************************************************************************
 *  File Name:
 *    phy_device_driver.hpp
 *
 *  Description:
 *    Driver interface to the RF24 physical device
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PHYSICAL_DEVICE_DRIVER_HPP
#define RIPPLE_PHYSICAL_DEVICE_DRIVER_HPP

/* Chimera Includes */
#include <Chimera/common>

/* Ripple Includes */
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/physical/phy_device_types.hpp>

namespace Ripple::PHY
{
  /*-------------------------------------------------------------------------------
  Open/Close Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Uses the configuration data to initialize a new physical device.
   *
   *  @param[in]  cfg           Settings to apply to the device upon opening
   *  @param[in]  handle        Configured device handle, if opening is successful
   *  @return Chimera::Status_t
   */
  Chimera::Status_t openDevice( const DriverConfig &cfg, DriverHandle &handle );

  /**
   *  Closes the device associated with the handle
   *
   *  @param[in]  handle        The device to close
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeDevice( DriverHandle &handle );


  /*-------------------------------------------------------------------------------
  Device Commands
  -------------------------------------------------------------------------------*/
  /**
   *  Clears out the TX FIFO
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t flushTX( DriverHandle &handle );

  /**
   *  Clears out the RX FIFO
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t flushRX( DriverHandle &handle );

  /**
   *  Start listening on the pipes opened for reading.
   *
   *  1. Be sure to call openReadPipe() first.
   *  2. Do not call write() while in this mode, without first calling stopListening().
   *  3. Call available() to check for incoming traffic, and read() to get it.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t startListening( DriverHandle &handle );

  /**
   *  Stop listening for RX messages and switch to transmit mode. Does nothing if already
   *  stopped listening.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t stopListening( DriverHandle &handle );

  /**
   *  Pauses a currently listening device. Does nothing if the device is not listening.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t pauseListening( DriverHandle &handle );

  /**
   *  Resumes listening a paused device. Does nothing if the device is not paused.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t resumeListening( DriverHandle &handle );

  /**
   *  Enable dynamically-sized payloads for both TX and ACK packets
   *
   *  This disables dynamic payloads on ALL pipes. Since ACK Payloads requires Dynamic Payloads, ACK Payloads
   *  are also disabled. If dynamic payloads are later re-enabled and ACK payloads are desired then enableAckPayload()
   *  must be called again as well.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  state       Enable/disable dynamic payloads
   *  @return void
   */
  Chimera::Status_t toggleDynamicPayloads( DriverHandle &handle, const bool state );

  /**
   *  Enable/disable the Auto-Ack functionality upon packet reception
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  state       Enable/disable auto-ack
   *  @param[in]  pipe        Which pipe to act on
   *  @return Chimera::Status_t
   */
  Chimera::Status_t toggleAutoAck( DriverHandle &handle, const bool state, const PipeNumber pipe );


  /*-------------------------------------------------------------------------------
  Data Pipe Operations
  -------------------------------------------------------------------------------*/
  /**
   *  Open pipe 0 to write to an address. This is the only pipe that can do this.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  address     The address for pipe 0 to write to
   *  @return Chimera::Status_t
   */
  Chimera::Status_t openWritePipe( DriverHandle &handle, const PhysicalAddress address );

  /**
   *  Closes pipe 0 for writing. Can be safely called without previously calling open.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeWritePipe( DriverHandle &handle );

  /**
   *  Open any pipe for reading. Up to 6 pipes can be open for reading at once.  Open all the required
   *  reading pipes, and then call startListening().
   *
   *  @note
   *   Pipes 0 and 1 will store a full 5-byte address. Pipes 2-5 will technically
   *   only store a single byte, borrowing up to 4 additional bytes from pipe #1 per the
   *   assigned address width.
   *
   *  @warning
   *     Pipes 1-5 should share the same address, except the first byte. Only the first byte in the array should be
   *     unique
   *
   *  @warning
   *     Pipe 0 is also used by the writing pipe.  So if you open pipe 0 for reading, and then startListening(), it
   *     will overwrite the writing pipe.  Ergo, do an openWritingPipe() again before write().
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  number      Which pipe to open, 0-5.
   *  @param[in]  address     The address you want the pipe to listen to
   *  @return Chimera::Status_t
   */
  Chimera::Status_t openReadPipe( DriverHandle &handle, const PipeNumber pipe, const PhysicalAddress address );

  /**
   *  Close a pipe after it has been previously opened.
   *  Can be safely called without having previously opened a pipe.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        Which pipe number to close, 0-5.
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeReadPipe( DriverHandle &handle, const PipeNumber pipe );

  /**
   *  Read the available FIFO payload into a buffer
   *
   *  @param[in]  handle      Handle to the device
   *  @param[out] buffer      Pointer to a buffer where the data should be written
   *  @param[in]  length      Maximum number of bytes to read into the buffer
   *  @return Chimera::Status_t
   */
  Chimera::Status_t readFIFO( DriverHandle &handle, void *const buffer, const size_t length );

  /**
   *  Immediately writes data to pipe 0 under the assumption that the hardware has already
   *  been configured for TX transfers. This prevents the software from going through the
   *  full TX configuration process each time a packet needs to be sent.
   *
   *  Prerequisite Calls:
   *    1. openWritePipe()
   *    2. setChannel()
   *    3. stopListening() [Pipe 0 only]
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  buffer      Array of data to be sent
   *  @param[in]  length      Number of bytes to be sent from the buffer
   *  @return Chimera::Status_t
   */
  Chimera::Status_t writePipe( DriverHandle &handle, const void *const buffer, const size_t length );

  /**
   *  Write an ACK payload for the specified pipe
   *
   *  The next time a message is received on the given pipe, the buffer data will be
   *  be sent back in the ACK packet.
   *
   *  @warning Only three ACK payloads can be pending at any time as there are only 3 FIFO buffers.
   *  @note ACK payloads are dynamic payloads, which only works on pipes 0 and 1 by default. Call
   *  enableDynamicPayloads() to enable on all pipes.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        Which pipe will get this response
   *  @param[in]  buffer      Data to be sent
   *  @param[in]  length      Length of the data to send, up to 32 bytes max.
   *  @return Chimera::Status_t
   */
  Chimera::Status_t stageAckPayload( DriverHandle &handle, const PipeNumber pipe, const void *const buffer, size_t length );


  /*-------------------------------------------------------------------------------
  Data Setters/Getters
  -------------------------------------------------------------------------------*/
  /**
   *  Reads the status register
   *
   *  @param[in]  handle      Handle to the device
   *  @return Reg8_t
   */
  Reg8_t getStatusRegister( DriverHandle &handle );

  /**
   *  Set the power amplifier level
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  level       Desired power amplifier level
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setPALevel( DriverHandle &handle, const PowerAmplitude level );

  /**
   *  Get the current power amplitude level
   *
   *  @param[in]  handle      Handle to the device
   *  @return PowerAmplitude
   */
  PowerAmplitude getPALevel( DriverHandle &handle );

  /**
   *  Set the TX/RX data rate
   *
   *  @warning setting RF24_250KBPS will fail for non (+) units
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  speed       Desired speed for the radio to TX/RX with
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setDataRate( DriverHandle &handle, const DataRate speed );

  /**
   *  Get the current transmission data rate
   *
   *  @param[in]  handle      Handle to the device
   *  @return DataRate
   */
  DataRate getDataRate( DriverHandle &handle );

  /**
   *  Set the number and delay of retries upon failed transfer
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  delay       How long to wait between each retry
   *  @param[in]  count       How many retries before giving up, max 15
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setRetries( DriverHandle &handle, const AutoRetransmitDelay delay, const size_t count );

  /**
   *  Gets the current configured retransmit delay
   *
   *  @param[in]  handle      Handle to the device
   *  @return AutoRetransmitDelay
   */
  AutoRetransmitDelay getRTXDelay( DriverHandle &handle );

  /**
   *  Gets the current configured transmit retry count
   *
   *  @param[in]  handle      Handle to the device
   *  @return size_t
   */
  size_t getRTXCount( DriverHandle &handle );

  /**
   *  Set RF communication channel
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  channel     Which RF channel to communicate on, 0-125
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setRFChannel( DriverHandle &handle, const size_t channel );

  /**
   *  Get the current RF communication channel
   *
   *  @param[in]  handle      Handle to the device
   *  @return size_t
   */
  size_t getRFChannel( DriverHandle &handle );

  /**
   *  Set static payload size
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  size        The number of bytes in the payload
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setStaticPayloadSize( DriverHandle &handle, const size_t size, const PipeNumber pipe );

  /**
   *  Get the currently configured static payload size
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        The pipe to check
   *  @return size_t
   */
  size_t getStaticPayloadSize( DriverHandle &handle, const PipeNumber pipe );

  /**
   *  Check if data is available to be read on any pipe. If so, returns a bitfield that indicates
   *  which pipe is ready.
   *
   *  @param[in]  handle      Handle to the device
   *  @return PipeNumber
   */
  PipeNumber getFIFOPayloadAvailable( DriverHandle &handle );

  /**
   *  Gets how many bytes are available in the latest RX FIFO payload
   *
   *  @param[in]  handle      Handle to the device
   *  @return size_t
   */
  size_t getFIFOPayloadSize( DriverHandle &handle );

}    // namespace Ripple::PHY

#endif /* !RIPPLE_PHYSICAL_DEVICE_DRIVER_HPP */
