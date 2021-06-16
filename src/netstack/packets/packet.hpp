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
  Structures
  -------------------------------------------------------------------------------*/
  struct PacketAssemblyCB
  {
    bool inProgress;
    bool remove;
    RefPtr<Packet> packet;
    size_t bytesRcvd;
    size_t startRxTime;
    size_t lastTimeoutCheck;
    size_t timeout;
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
    bool isValid();
    void sort();
    void updateCRC();

    bool pack( const void *const buffer, const size_t size );
    bool unpack( void *const buffer, const size_t size );

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

    /**
     * @brief Expected max size of this packet
     *
     * @return size_t
     */
    size_t maxSize();
    uint32_t readCRC();
    uint32_t calcCRC();
    uint16_t getUUID();

    /*-------------------------------------------------
    Debugging Utilities
    -------------------------------------------------*/
    void printPayload();


    Fragment_sPtr head;

  private:
    INetMgr * mContext;
    uint16_t uuid;
    uint16_t totalLength;
    uint16_t totalFragments;

  };


  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Packet_sPtr allocPacket( INetMgr *const context );

}  // namespace Ripple

#endif  /* !RIPPLE_PACKET_HPP */
