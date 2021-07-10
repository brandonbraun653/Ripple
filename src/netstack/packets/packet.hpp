/********************************************************************************
 *  File Name:
 *    packet.hpp
 *
 *  Description:
 *    High level packet interface for Ripple
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PACKET_HPP
#define RIPPLE_PACKET_HPP

/* ETL Includes */
#include <etl/list.h>
#include <etl/map.h>
#include <etl/queue.h>

/* Aurora Includes */
#include <Aurora/memory>

/* Ripple Includes */
#include <Ripple/src/netstack/packets/fragment.hpp>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Forwared Declarations
  -------------------------------------------------------------------------------*/
  class Fragment;
  class Packet;

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  template<size_t SIZE>
  using PacketQueue = etl::queue<Aurora::Memory::shared_ptr<Packet>, SIZE>;

  using Packet_sPtr = Aurora::Memory::shared_ptr<Packet>;

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Packet_sPtr allocPacket( Aurora::Memory::IHeapAllocator *const context );

  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct PacketAssemblyCB
  {
    bool inProgress;
    bool remove;
    Packet_sPtr packet;
    size_t bytesRcvd;
    size_t startRxTime;
    size_t lastTimeoutCheck;
    size_t timeout;

    /**
     * @brief Default construct a new PacketAssemblyCB
     */
    explicit PacketAssemblyCB()
    {
      clear();
    }

    /**
     * @brief Construct a new PacketAssemblyCB object with memory allocator
     *
     * @param context     Memory allocator
     */
    explicit PacketAssemblyCB( Aurora::Memory::IHeapAllocator *const context )
    {
      clear();
      packet = allocPacket( context );
    }

    explicit PacketAssemblyCB( PacketAssemblyCB &&obj )
    {
      this->inProgress       = obj.inProgress;
      this->remove           = obj.remove;
      this->packet           = obj.packet;
      this->bytesRcvd        = obj.bytesRcvd;
      this->startRxTime      = obj.startRxTime;
      this->lastTimeoutCheck = obj.lastTimeoutCheck;
      this->timeout          = obj.timeout;

      obj.inProgress       = false;
      obj.remove           = false;
      obj.packet           = Packet_sPtr();
      obj.bytesRcvd        = 0;
      obj.startRxTime      = 0;
      obj.lastTimeoutCheck = 0;
      obj.timeout          = 0;
    }

    /**
     * @brief Resets the assembly to defaults
     *
     */
    void clear()
    {
      inProgress       = false;
      remove           = false;
      packet           = Packet_sPtr();
      bytesRcvd        = 0;
      startRxTime      = 0;
      lastTimeoutCheck = 0;
      timeout          = 0;
    }
  };

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  template<size_t SIZE>
  using AssemblyMap = etl::map<uint32_t, PacketAssemblyCB, SIZE>;

  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   * @brief Top level interface for raw packets on the network
   *
   */
  class Packet
  {
  public:
    Packet();
    Packet( Aurora::Memory::IHeapAllocator *const context );
    ~Packet();

    /*-------------------------------------------------
    Actions
    -------------------------------------------------*/
    void sort();
    bool pack( const void *const buffer, const size_t size );
    bool unpack( void * buffer, const size_t size );

    /*-------------------------------------------------
    Information
    -------------------------------------------------*/
    size_t numFragments();

    /**
     * @brief Current size held in the fragment list
     *
     * @return size_t
     */
    size_t size();

    uint16_t getUUID();

    /*-------------------------------------------------
    Debugging Utilities
    -------------------------------------------------*/
    void printPayload();


    Fragment_sPtr head;

  protected:
    friend Packet_sPtr allocPacket( Aurora::Memory::IHeapAllocator *const context );

    Aurora::Memory::IHeapAllocator *mContext;
    void assignCRC( uint32_t value );

  private:
    size_t mFragmentationSize;
    uint16_t mTotalFragments;
  };

}    // namespace Ripple

#endif /* !RIPPLE_PACKET_HPP */
