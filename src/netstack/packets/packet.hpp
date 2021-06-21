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

/* Ripple Includes */
#include <Ripple/src/netstack/net_mgr_intf.hpp>
#include <Ripple/src/netstack/util/memory.hpp>
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
  using PacketQueue = etl::queue<RefPtr<Packet>, SIZE>;

  using Packet_sPtr = RefPtr<Packet>;

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Packet_sPtr allocPacket( INetMgr *const context );

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
    PacketAssemblyCB()
    {
      clear();
    }

    /**
     * @brief Construct a new PacketAssemblyCB object with memory allocator
     *
     * @param context     Memory allocator
     */
    PacketAssemblyCB( INetMgr *const context )
    {
      clear();
      packet = allocPacket( context );
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
    Packet( INetMgr *const context );
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
    friend Packet_sPtr allocPacket( INetMgr *const context );

    INetMgr *mContext;
    void assignCRC( uint32_t value );

  private:
    size_t mFragmentationSize;
    uint16_t uuid;
    uint16_t totalLength;
    uint16_t mTotalFragments;
  };

}    // namespace Ripple

#endif /* !RIPPLE_PACKET_HPP */
