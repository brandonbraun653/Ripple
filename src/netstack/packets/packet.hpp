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
  class PacketAssembly
  {
  public:
    enum class RemoveErr
    {
      UNKNOWN,            /**< Default invalid reason */
      COMPLETED,          /**< Packet completed successfully. No errors. */
      TIMEOUT,            /**< Assembly time limit was reached */
      CORRUPTION,         /**< Packet was corrupted somehow */
      SOCK_Q_FULL,        /**< Destination socket receive queue was full */
      SOCK_NOT_FOUND,     /**< Destination socket wasn't found */
    };

    bool inProgress;         /**< Is the assembly still accumulating fragments? */
    bool remove;             /**< Flag to mark for removal */
    RemoveErr whyRemove;     /**< Reason for removal */
    Packet_sPtr packet;      /**< Fragment container */
    size_t bytesRcvd;        /**< Total number of bytes received */
    size_t startRxTime;      /**< Time the assembly started */
    size_t lastTimeoutCheck; /**< Last time a timeout check was performed */
    size_t timeout;          /**< Time delta the assembly has to build the message */

    /**
     * @brief Default construct a new PacketAssembly
     */
    explicit PacketAssembly()
    {
      clear();
    }

    /**
     * @brief Construct a new PacketAssembly object with memory allocator
     *
     * @param context     Memory allocator
     */
    explicit PacketAssembly( Aurora::Memory::IHeapAllocator *const context )
    {
      clear();
      packet = allocPacket( context );
    }

    /**
     * @brief Move construct a new Packet Assembly object
     *
     * @param obj     The old object
     */
    explicit PacketAssembly( PacketAssembly &&obj )
    {
      this->inProgress       = obj.inProgress;
      this->remove           = obj.remove;
      this->packet           = obj.packet;
      this->bytesRcvd        = obj.bytesRcvd;
      this->startRxTime      = obj.startRxTime;
      this->lastTimeoutCheck = obj.lastTimeoutCheck;
      this->timeout          = obj.timeout;
      this->whyRemove        = obj.whyRemove;

      obj.clear();
    }

    /**
     * @brief Resets the assembly to defaults
     */
    void clear()
    {
      inProgress       = false;
      remove           = false;
      whyRemove        = RemoveErr::UNKNOWN;
      packet           = Packet_sPtr();
      bytesRcvd        = 0;
      startRxTime      = 0;
      lastTimeoutCheck = 0;
      timeout          = 0;
    }


    /**
     * @brief Helper method to convert removal reason error code into a string
     * @return const char*
     */
    const char *whyRemoveString()
    {
      switch( whyRemove )
      {
        case RemoveErr::COMPLETED:
          return "Packet built successfully";
          break;

        case RemoveErr::CORRUPTION:
          return "Packet was corrupted";
          break;

        case RemoveErr::TIMEOUT:
          return "Packet assembly timed out";
          break;

        case RemoveErr::SOCK_Q_FULL:
          return "Destination socket receive queue was full";
          break;

        case RemoveErr::SOCK_NOT_FOUND:
          return "Destination socket was not found";
          break;

        case RemoveErr::UNKNOWN:
        default:
          return "Unknown reason";
          break;
      };
    }
  };

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  template<size_t SIZE>
  using AssemblyMap = etl::map<const uint32_t, PacketAssembly, SIZE>;

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
    /**
     * @brief Total number of fragments the packet is broken into
     *
     * @return size_t
     */
    size_t numFragments() const;

    /**
     * @brief Current size held in the fragment list
     *
     * @return size_t
     */
    size_t size();

    /**
     * @brief Returns the unique ID of the packet
     *
     * @return uint16_t
     */
    uint16_t getUUID();

    /**
     * @brief Validates if all expected fragments exist
     * @note Assumes packet memory is not modified while function executes
     *
     * @return true     All fragments are present
     * @return false    One or more fragments are missing
     */
    bool isMissingFragments() const;

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
