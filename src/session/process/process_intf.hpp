/********************************************************************************
 *  File Name:
 *    process_intf.hpp
 *
 *  Description:
 *    Process interface declarations
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PROCESS_INTERFACES_HPP
#define RIPPLE_PROCESS_INTERFACES_HPP

/* Ripple Includes */
#include <Ripple/src/session/session_process.hpp>

namespace Ripple::Session::Process
{
  /*-------------------------------------------------------------------------------
  Classes
  -------------------------------------------------------------------------------*/
  class JoinNetwork : public virtual IProcess
  {
  public:
    JoinNetwork();
    ~JoinNetwork();

    Status getStatus() final override;
    Chimera::Status_t initialize() final override;
    Chimera::Status_t start() final override;
    Chimera::Status_t update() final override;
    Chimera::Status_t close() final override;
    Chimera::Status_t onError() final override;
  };


  class EstablishConnection : public virtual IProcess
  {
  public:
    EstablishConnection();
    ~EstablishConnection();

    Status getStatus() final override;
    Chimera::Status_t initialize() final override;
    Chimera::Status_t start() final override;
    Chimera::Status_t update() final override;
    Chimera::Status_t close() final override;
    Chimera::Status_t onError() final override;
  };


  class TerminateConnection : public virtual IProcess
  {
  public:
    TerminateConnection();
    ~TerminateConnection();

    Status getStatus() final override;
    Chimera::Status_t initialize() final override;
    Chimera::Status_t start() final override;
    Chimera::Status_t update() final override;
    Chimera::Status_t close() final override;
    Chimera::Status_t onError() final override;
  };

}  // namespace Ripple::Session::Process

#endif  /* !RIPPLE_PROCESS_INTERFACES_HPP */
