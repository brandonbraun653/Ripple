/********************************************************************************
 *  File Name:
 *    version.hpp
 *
 *  Description:
 *    Ripple version description
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_VERSION_HPP
#define RIPPLE_VERSION_HPP

/* STL Includes */
#include <string_view>

namespace Thor::HLD
{
  /**
   *  CHANGELOG:
   *
   *  v0.1.0: Implemented a stable datalink layer and ported/improved the
   *          previous NRF24L01 driver from the first cut of this project at:
   *          https://github.com/brandonbraun653/RF24Node
   */
  static constexpr std::string_view VersionString = "0.1.0";

  static constexpr size_t VersionMajor = 0;
  static constexpr size_t VersionMinor = 1;
  static constexpr size_t VersionPatch = 0;
}    // namespace Thor::HLD

#endif /* !RIPPLE_VERSION_HPP */