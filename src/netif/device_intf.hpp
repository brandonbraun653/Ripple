/********************************************************************************
 *  File Name:
 *    virtual_intf.hpp
 *
 *  Description:
 *    Virtual network interface declaration
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_VIRTUAL_NET_INTERFACE_HPP
#define RIPPLE_VIRTUAL_NET_INTERFACE_HPP

/* STL Includes */
#include <cstddef>

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/callback>
#include <Chimera/thread>

/* Ripple Includes */
#include <Ripple/src/netif/device_types.hpp>
#include <Ripple/src/shared/cmn_types.hpp>
#include <Ripple/src/netstack/types.hpp>


namespace Ripple::NetIf
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  /**
   *  Virtual class that provides the ARP service interface. Given that there is a wide
   *  number of transports supported, the hardware addressing scheme is not assumed to
   *  be similar. This requires a custom ARP class for each network interface type.
   */
  class IARP
  {
  public:
    virtual ~IARP() = default;

    /**
     *  Adds a new entry to the ARP table
     *
     *  @param[in]  ip          IP address to key off of
     *  @param[in]  mac         Pointer to the MAC address associated with the IP
     *  @param[in]  size        Size of the MAC address type
     *  @return Chimera::Status_t
     */
    virtual Chimera::Status_t addARPEntry( const IPAddress &ip, const void *const mac, const size_t size ) = 0;

    /**
     *  Removes an entry from the ARP table
     *
     *  @param[in]  ip          IP address of entry to remove
     *  @return Chimera::Status_t
     */
    virtual Chimera::Status_t dropARPEntry( const IPAddress &ip ) = 0;

    /**
     *  Looks up the interface specific MAC address attached to IP address
     *
     *  @param[in]  ip          IP address to key off of
     *  @param[out] mac         Pointer to the MAC address buffer to be modified
     *  @param[in]  size        Size of the MAC address type
     *  @return bool
     */
    virtual bool arpLookUp( const IPAddress &ip, void *const mac, const size_t size ) = 0;

    /**
     *  Looks up the IP address for a given interface specific MAC address
     *
     *  @param[in]  mac         MAC address buffer
     *  @param[in]  size        Size of the MAC address buffer
     *  @return IPAddress       IP address tied to the MAC
     */
    virtual IPAddress arpLookUp( const void *const mac, const size_t size ) = 0;
  };


  /**
   *  Abstract class that all network interface drivers must implement to be
   *  compatible with the higher level stack.
   */
  class INetIf : public Chimera::Callback::DelegateService<INetIf, CallbackId, CallbackReport>, public Chimera::Thread::Lockable<INetIf>
  {
  public:
    virtual ~INetIf() = default;

    /**
     *  Performs the interface initialization sequence. This method is called when the network
     *  stack boots up.
     *
     *  @param[in]  context     Network context
     *  @return bool            Boot success/fail
     */
    virtual bool powerUp( void * context ) = 0;

    /**
     *  Powers down the network interface
     *  @return void
     */
    virtual void powerDn() = 0;

    /**
     *  Retrieves a list of fragments for passing up the stack.
     *
     *  @note The returned data is a mix of fragments from multiple unique packets.
     *        It's the upper layers' job to break this apart and sort them into their
     *        respective packet assembly containers.
     *
     *  @param[in]  fragmentList          Pointer to a list of fragments
     *  @return Chimera::Status_t
     *
     *  @retval Chimera::Status::OK       The entire fragment list was allocated
     *  @retval Chimera::Status::READY    Ready to give the next fragment
     *  @retval Chimera::Status::EMPTY    No fragments are available
     *  @retval Chimera::Status::MEMORY   Not enough memory available
     *  @retval Chimera::Status::FAIL     Some kind of unhandled error occurred
     */
    virtual Chimera::Status_t recv( MsgFrag ** fragmentList ) = 0;

    /**
     *  Transmits a message directly to the given IP address. This function should not
     *  need to perform any kind of network path calculations as that's handled by
     *  higher layers in the stack. This is a "dumb" send directly to a known device
     *  in the ARP cache, or don't send at all.
     *
     *  @param[in]  head        Root of the message to send
     *  @param[in]  ip          Address to send to
     *  @return Chimera::Status_t
     *
     *  @retval Chimera::Status::OK       The entire fragment list was sent
     *  @retval Chimera::Status::READY    Ready to transmit the next fragment
     *  @retval Chimera::Status::FULL     Cannot accept any more fragments just yet
     *  @retval Chimera::Status::MEMORY   There was an issue with memory
     *  @retval Chimera::Status::FAIL     Some kind of unhandled error occurred
     */
    virtual Chimera::Status_t send( const MsgFrag *const head, const IPAddress &ip ) = 0;

    /**
     *  Gets the interface's address resolver
     *  @return IARP *
     */
    virtual IARP *addressResolver() = 0;

    /**
     *  Max data size that the network interface can reasonably handle
     *  @return size_t
     */
    virtual size_t maxTransferSize() const = 0;

    /**
     *  Returns the maximum link speed the interface can handle in bytes/sec
     *  @return size_t
     */
    virtual size_t linkSpeed() const = 0;

    /**
     *  Returns last system time the interface was active
     *  @return size_t
     */
    virtual size_t lastActive() const = 0;

  private:
    friend Chimera::Thread::Lockable<INetIf>;
  };


}    // namespace Ripple::NetIf

#endif /* !RIPPLE_VIRTUAL_NET_INTERFACE_HPP */
