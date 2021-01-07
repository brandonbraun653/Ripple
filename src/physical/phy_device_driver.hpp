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
#include <Ripple/src/session/session_types.hpp>

namespace Ripple::Physical
{
  /*-------------------------------------------------------------------------------
  Utility Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Extracts the handle from the session context
   *
   *  @param[in]  session       The current session
   *  @return Handle *
   */
  Handle *getHandle( Session::Context session );


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
  Chimera::Status_t openDevice( const DeviceConfig &cfg, Handle &handle );

  /**
   *  Closes the device associated with the handle
   *
   *  @param[in]  handle        The device to close
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeDevice( Handle &handle );


  /*-------------------------------------------------------------------------------
  Device Commands
  -------------------------------------------------------------------------------*/
  /**
   *  Resets the device registers to their default settings
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t resetRegisterDefaults( Handle &handle );

  /**
   *  Clears out the TX FIFO
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t flushTX( Handle &handle );

  /**
   *  Clears out the RX FIFO
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t flushRX( Handle &handle );

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
  Chimera::Status_t startListening( Handle &handle );

  /**
   *  Stop listening for RX messages and switch to transmit mode. Does nothing if already
   *  stopped listening.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t stopListening( Handle &handle );

  /**
   *  Pauses a currently listening device. Does nothing if the device is not listening.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t pauseListening( Handle &handle );

  /**
   *  Resumes listening a paused device. Does nothing if the device is not paused.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t resumeListening( Handle &handle );

  /**
   *  Enables/disables sending payloads along with ACK frames. Must have
   *  dynamic payloads enabled as well.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  state       Enable/disable ack payloads
   *  @return Chimera::Status_t
   */
  Chimera::Status_t toggleAckPayloads( Handle &handle, const bool state );

  /**
   *  Enable dynamically-sized payloads for both TX and ACK packets
   *
   *  This disables dynamic payloads on ALL pipes. Since ACK Payloads requires Dynamic Payloads, ACK Payloads
   *  are also disabled. If dynamic payloads are later re-enabled and ACK payloads are desired then enableAckPayload()
   *  must be called again as well.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        Which pipe to act on
   *  @param[in]  state       Enable/disable dynamic payloads
   *  @return Chimera::Status_t
   */
  Chimera::Status_t toggleDynamicPayloads( Handle &handle, const PipeNumber pipe, const bool state );

  /**
   *  Enable/disable the ability to selectively enable the auto-ack functionality
   *  on a per-packet basis. This affects availability of the W_TX_PAYLOAD_NOACK
   *  command.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  state       Enable/disable the dynamic ack function
   *  @return Chimera::Status_t
   */
  Chimera::Status_t toggleDynamicAck( Handle &handle, const bool state );

  /**
   *  Enable/disable the Auto-Ack functionality upon packet reception
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  state       Enable/disable auto-ack
   *  @param[in]  pipe        Which pipe to act on
   *  @return Chimera::Status_t
   */
  Chimera::Status_t toggleAutoAck( Handle &handle, const bool state, const PipeNumber pipe );

  /**
   *  Enable/disable the device Feature register
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  state       Enable/disable
   *  @return Chimera::Status_t
   */
  Chimera::Status_t toggleFeatures( Handle &handle, const bool state );

  /**
   *  Enables/disables the device radio power
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  state       Enable/disable
   *  @return Chimera::Status_t
   */
  Chimera::Status_t togglePower( Handle &handle, const bool state );


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
  Chimera::Status_t openWritePipe( Handle &handle, const MACAddress address );

  /**
   *  Closes pipe 0 for writing. Can be safely called without previously calling open.
   *
   *  @param[in]  handle      Handle to the device
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeWritePipe( Handle &handle );

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
  Chimera::Status_t openReadPipe( Handle &handle, const PipeNumber pipe, const MACAddress address );

  /**
   *  Close a pipe after it has been previously opened.
   *  Can be safely called without having previously opened a pipe.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        Which pipe number to close, 0-5.
   *  @return Chimera::Status_t
   */
  Chimera::Status_t closeReadPipe( Handle &handle, const PipeNumber pipe );

  /**
   *  Read the available FIFO payload into a buffer
   *
   *  @param[in]  handle      Handle to the device
   *  @param[out] buffer      Pointer to a buffer where the data should be written. Should be sized for max packet.
   *  @param[in]  length      Number of bytes to read from FIFO into the buffer
   *  @return Chimera::Status_t
   */
  Chimera::Status_t readPayload( Handle &handle, void *const buffer, const size_t length );

  /**
   *  Immediately writes data to pipe 0 under the assumption that the hardware has already
   *  been configured properly for the transfer.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  buffer      Array of data to be sent
   *  @param[in]  length      Number of bytes to be sent from the buffer
   *  @param[in]  type        Should the hardware expect an ACK from receiver or not?
   *  @return Chimera::Status_t
   */
  Chimera::Status_t writePayload( Handle &handle, const void *const buffer, const size_t length, const PayloadType type );

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
  Chimera::Status_t stageAckPayload( Handle &handle, const PipeNumber pipe, const void *const buffer, size_t length );


  /*-------------------------------------------------------------------------------
  Data Setters/Getters
  -------------------------------------------------------------------------------*/
  /**
   *  Reads every register on the device and updates the handle's register
   *  map cache with the data.
   *
   *  @warning  This is a time consuming operation, meant for debugging only.
   *
   *  @param[in]  handle      Handle to the device
   *  @return void
   */
  void readAllRegisters( Handle &handle );

  /**
   *  Reads the status register
   *
   *  @param[in]  handle      Handle to the device
   *  @return Reg8_t
   */
  Reg8_t getStatusRegister( Handle &handle );

  /**
   *  Checks if the RX FIFO is full
   *
   *  @param[in]  handle      Handle to the device
   *  @return bool
   */
  bool rxFifoFull( Handle &handle );

  /**
   *  Checks if the RX FIFO is empty
   *
   *  @param[in]  handle      Handle to the device
   *  @return bool
   */
  bool rxFifoEmpty( Handle &handle );

  /**
   *  Checks if the TX FIFO is full
   *
   *  @param[in]  handle      Handle to the device
   *  @return bool
   */
  bool txFifoFull( Handle &handle );

  /**
   *  Checks if the TX FIFO is empty
   *
   *  @param[in]  handle      Handle to the device
   *  @return bool
   */
  bool txFifoEmpty( Handle &handle );

  /**
   *  Set the power amplifier level
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  level       Desired power amplifier level
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setRFPower( Handle &handle, const RFPower level );

  /**
   *  Get the current power amplitude level
   *
   *  @param[in]  handle      Handle to the device
   *  @return RFPower
   */
  RFPower getRFPower( Handle &handle );

  /**
   *  Set the TX/RX data rate
   *
   *  @warning setting RF24_250KBPS will fail for non (+) units
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  speed       Desired speed for the radio to TX/RX with
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setDataRate( Handle &handle, const DataRate speed );

  /**
   *  Get the current transmission data rate
   *
   *  @param[in]  handle      Handle to the device
   *  @return DataRate
   */
  DataRate getDataRate( Handle &handle );

  /**
   *  Set the number and delay of retries upon failed transfer
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  delay       How long to wait between each retry
   *  @param[in]  count       How many retries before giving up, max 15
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setRetries( Handle &handle, const AutoRetransmitDelay delay, const size_t count );

  /**
   *  Gets the current configured retransmit delay
   *
   *  @param[in]  handle      Handle to the device
   *  @return AutoRetransmitDelay
   */
  AutoRetransmitDelay getRTXDelay( Handle &handle );

  /**
   *  Gets the current configured transmit retry count
   *
   *  @param[in]  handle      Handle to the device
   *  @return size_t
   */
  AutoRetransmitCount getRTXCount( Handle &handle );

  /**
   *  Set RF communication channel
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  channel     Which RF channel to communicate on, 0-125
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setRFChannel( Handle &handle, const size_t channel );

  /**
   *  Get the current RF communication channel
   *
   *  @param[in]  handle      Handle to the device
   *  @return size_t
   */
  size_t getRFChannel( Handle &handle );

  /**
   *  Sets the ISR mask to enable/disable interrupt event generations on the
   *  device's IRQ pin.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  msk         Bit field of which ISRs to enable
   *  @return Chimera::Stauts_t
   */
  Chimera::Status_t setISRMasks( Handle &handle, const uint8_t msk );

  /**
   *  Gets the currently enabled ISR masks
   *
   *  @param[in]  handle      Handle to the device
   *  @return uint8_t
   */
  uint8_t getISRMasks( Handle &handle );

  /**
   *  Clears the requested ISR events
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  msk         Bit field of which ISRs to clear
   *  @return Chimera::Stauts_t
   */
  Chimera::Status_t clrISREvent( Handle &handle, const bfISRMask msk );

  /**
   *  Gets the most recent ISR event flags
   *
   *  @param[in] handle       Handle to the device
   *  @return uint8_t
   */
  uint8_t getISREvent( Handle &handle );

  /**
   *  Set device address width
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  width       Which address width to set
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setAddressWidth( Handle &handle, const AddressWidth width );

  /**
   *  Gets the current address width setting
   *
   *  @param[in]  handle      Handle to the device
   *  @return AddressWidth
   */
  AddressWidth getAddressWidth( Handle &handle );

  /**
   *  Gets the address associated with the RX pipe
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        Which pipe to read
   *  @return MACAddress
   */
  MACAddress getRXPipeAddress( Handle &handle, const PipeNumber pipe );

  /**
   *  Set the CRC length used for RF packet transactions
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  length      Which CRC length to set
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setCRCLength( Handle &handle, const CRCLength length );

  /**
   *  Get the current CRC length
   *
   *  @param[in]  handle      Handle to the device
   *  @return CRCLength
   */
  CRCLength getCRCLength( Handle &handle );

  /**
   *  Set payload size for a given pipe. If using static payloads, the transmitter must
   *  send the exact amount of bytes or else the pipe won't see it. If using dynamic
   *  payloads, this setting will be ignored.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  size        The number of bytes in the payload
   *  @return Chimera::Status_t
   */
  Chimera::Status_t setStaticPayloadSize( Handle &handle, const size_t size, const PipeNumber pipe );

  /**
   *  Get the currently configured static payload size
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        The pipe to check
   *  @return size_t
   */
  size_t getStaticPayloadSize( Handle &handle, const PipeNumber pipe );

  /**
   *  Check if data is available to be read on any pipe. If so, returns a bitfield that indicates
   *  which pipe is ready.
   *
   *  @param[in]  handle      Handle to the device
   *  @return PipeNumber
   */
  PipeNumber getAvailablePayloadPipe( Handle &handle );

  /**
   *  Gets the size of the latest packet, accounting for either static or
   *  dynamic payload width configurations.
   *
   *  @param[in]  handle      Handle to the device
   *  @param[in]  pipe        The pipe to check (only used if static payload enabled)
   *  @return size_t
   */
  size_t getAvailablePayloadSize( Handle &handle, const PipeNumber pipe );

}    // namespace Ripple::Physical

#endif /* !RIPPLE_PHYSICAL_DEVICE_DRIVER_HPP */
