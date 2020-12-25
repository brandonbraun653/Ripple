/********************************************************************************
 *  File Name:
 *    phy_device_driver.cpp
 *
 *  Description:
 *    Implements the physical device driver for an NRF24L01 module
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Chimera Includes */
#include <Chimera/common>

/* Ripple Includes */
#include <Ripple/physical>

namespace Ripple::PHY
{
  /*-------------------------------------------------------------------------------
  Open/Close Functions
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openDevice( const DriverConfig &cfg, DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t closeDevice( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  /*-------------------------------------------------------------------------------
  Device Commands
  -------------------------------------------------------------------------------*/
  Chimera::Status_t flushTX( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t flushRX( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t startListening( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t stopListening( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t pauseListening( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t resumeListening( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t toggleDynamicPayloads( DriverHandle &handle, const bool state )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t toggleAutoAck( DriverHandle &handle, const bool state, const PipeNumber pipe )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  /*-------------------------------------------------------------------------------
  Data Pipe Operations
  -------------------------------------------------------------------------------*/
  Chimera::Status_t openWritePipe( DriverHandle &handle, const PhysicalAddress address )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t closeWritePipe( DriverHandle &handle )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t openReadPipe( DriverHandle &handle, const PipeNumber pipe, const PhysicalAddress address )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t closeReadPipe( DriverHandle &handle, const PipeNumber pipe )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t readFIFO( DriverHandle &handle, void *const buffer, const size_t length )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t writePipe( DriverHandle &handle, const void *const buffer, const size_t length )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  Chimera::Status_t stageAckPayload( DriverHandle &handle, const PipeNumber pipe, const void *const buffer, size_t length )
  {
    return Chimera::Status::NOT_SUPPORTED;
  }


  /*-------------------------------------------------------------------------------
  Data Setters/Getters
  -------------------------------------------------------------------------------*/
  Reg8_t getStatusRegister( DriverHandle &handle )
  {
  }


  Chimera::Status_t setPALevel( DriverHandle &handle, const PowerAmplitude level )
  {
  }


  PowerAmplitude getPALevel( DriverHandle &handle )
  {
  }


  Chimera::Status_t setDataRate( DriverHandle &handle, const DataRate speed )
  {
  }


  DataRate getDataRate( DriverHandle &handle )
  {
  }


  Chimera::Status_t setRetries( DriverHandle &handle, const AutoRetransmitDelay delay, const size_t count )
  {
  }


  AutoRetransmitDelay getRTXDelay( DriverHandle &handle )
  {
  }


  size_t getRTXCount( DriverHandle &handle )
  {
  }


  Chimera::Status_t setRFChannel( DriverHandle &handle, const size_t channel )
  {
  }


  size_t getRFChannel( DriverHandle &handle )
  {
  }


  Chimera::Status_t setStaticPayloadSize( DriverHandle &handle, const size_t size, const PipeNumber pipe )
  {
  }


  size_t getStaticPayloadSize( DriverHandle &handle, const PipeNumber pipe )
  {
  }


  PipeNumber getFIFOPayloadAvailable( DriverHandle &handle )
  {
  }


  size_t getFIFOPayloadSize( DriverHandle &handle )
  {
  }

}  // namespace Ripple::PHY
