/********************************************************************************
 *  File Name:
 *    config.hpp
 *
 *  Description:
 *    Ripple configuration
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_CONFIGURATION_HPP
#define RIPPLE_CONFIGURATION_HPP


/**
 * Max number of sockets per network context
 */
#if !defined( RIPPLE_CTX_MAX_SOCKETS )
#define RIPPLE_CTX_MAX_SOCKETS ( 4 )
#endif

/**
 * Max number of fragmented packets that are being assembled at any given time
 */
#if !defined( RIPPLE_CTX_MAX_PKT )
#define RIPPLE_CTX_MAX_PKT ( 32 )
#endif

/**
 * Amount of time (ms) a fragmented packet can spend being assembled in the net
 * stack. For example, if a packet has 10 fragments then all 10 fragments must
 * arrive in this time window, else the entire assembly will be discarded.
 */
#if !defined( RIPPLE_PKT_LIFETIME )
#define RIPPLE_PKT_LIFETIME ( 750 * Chimera::Thread::TIMEOUT_1MS )
#endif

#endif  /* !RIPPLE_CONFIGURATION_HPP */
