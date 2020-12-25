/********************************************************************************
 *  File Name:
 *    phy_device_register.hpp
 *
 *  Description:
 *    Register definitions for the NRF24L01+
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef RIPPLE_PHYSICAL_DEVICE_REGISTERS_HPP
#define RIPPLE_PHYSICAL_DEVICE_REGISTERS_HPP

/* STL Includes */
#include <cstdint>

namespace Ripple::PHY
{
  /*-------------------------------------------------------------------------------
  Command Instructions
  -------------------------------------------------------------------------------*/
  static constexpr uint8_t CMD_REGISTER_MASK       = 0x1F; /**< Masks off the largest available register address */
  static constexpr uint8_t CMD_R_REGISTER          = 0x00; /**< Read command and status registers  */
  static constexpr uint8_t CMD_W_REGISTER          = 0x20; /**< Write command and status registers  */
  static constexpr uint8_t CMD_R_RX_PAYLOAD        = 0x61; /**< Read RX Payload (1-32 bytes) */
  static constexpr uint8_t CMD_W_TX_PAYLOAD        = 0xA0; /**< Write TX Payload (1-32 bytes) */
  static constexpr uint8_t CMD_FLUSH_TX            = 0xE1; /**< Flush TX FIFO, used in TX Mode */
  static constexpr uint8_t CMD_FLUSH_RX            = 0xE2; /**< Flush RX FIFO, used in RX Mode */
  static constexpr uint8_t CMD_REUSE_TX_PL         = 0xE3; /**< Reuse last transmitted payload (PTX device only) */
  static constexpr uint8_t CMD_ACTIVATE            = 0x50; /**< Activates R_RX_PL_WID, W_ACK_PAYLOAD, W_TX_PAYLOAD_NOACK */
  static constexpr uint8_t CMD_R_RX_PL_WID         = 0x60; /**< Read RX payload width for the top payload in the RX FIFO */
  static constexpr uint8_t CMD_W_ACK_PAYLOAD       = 0xA8; /**< Write Payload together with ACK packet */
  static constexpr uint8_t CMD_W_TX_PAYLOAD_NO_ACK = 0xB0; /**< Disables AUTOACK on this specific packet */
  static constexpr uint8_t CMD_NOP                 = 0xFF; /**< No operation */

  /*-------------------------------------------------------------------------------
  Register Addresses
  -------------------------------------------------------------------------------*/
  static constexpr uint8_t REG_CONFIG      = 0x00; /**< Configuration Register */
  static constexpr uint8_t REG_EN_AA       = 0x01; /**< Enable Auto Acknowledgment */
  static constexpr uint8_t REG_EN_RXADDR   = 0x02; /**< Enable RX Addresses */
  static constexpr uint8_t REG_SETUP_AW    = 0x03; /**< Setup of Address Width */
  static constexpr uint8_t REG_SETUP_RETR  = 0x04; /**< Setup of Automatic Retransmission */
  static constexpr uint8_t REG_RF_CH       = 0x05; /**< RF Channel Frequency Settings */
  static constexpr uint8_t REG_RF_SETUP    = 0x06; /**< RF Channel Settings Register */
  static constexpr uint8_t REG_STATUS      = 0x07; /**< Status Register */
  static constexpr uint8_t REG_OBSERVE_TX  = 0x08; /**< Transmit Observe */
  static constexpr uint8_t REG_CD          = 0x09; /**< Carrier Detect */
  static constexpr uint8_t REG_RX_ADDR_P0  = 0x0A; /**< Receive Address Data Pipe 0 */
  static constexpr uint8_t REG_RX_ADDR_P1  = 0x0B; /**< Receive Address Data Pipe 1 */
  static constexpr uint8_t REG_RX_ADDR_P2  = 0x0C; /**< Receive Address Data Pipe 2 */
  static constexpr uint8_t REG_RX_ADDR_P3  = 0x0D; /**< Receive Address Data Pipe 3 */
  static constexpr uint8_t REG_RX_ADDR_P4  = 0x0E; /**< Receive Address Data Pipe 4 */
  static constexpr uint8_t REG_RX_ADDR_P5  = 0x0F; /**< Receive Address Data Pipe 5 */
  static constexpr uint8_t REG_TX_ADDR     = 0x10; /**< Transmit Address */
  static constexpr uint8_t REG_RX_PW_P0    = 0x11; /**< Number of bytes in RX Payload Data Pipe 0 */
  static constexpr uint8_t REG_RX_PW_P1    = 0x12; /**< Number of bytes in RX Payload Data Pipe 1 */
  static constexpr uint8_t REG_RX_PW_P2    = 0x13; /**< Number of bytes in RX Payload Data Pipe 2 */
  static constexpr uint8_t REG_RX_PW_P3    = 0x14; /**< Number of bytes in RX Payload Data Pipe 3 */
  static constexpr uint8_t REG_RX_PW_P4    = 0x15; /**< Number of bytes in RX Payload Data Pipe 4 */
  static constexpr uint8_t REG_RX_PW_P5    = 0x16; /**< Number of bytes in RX Payload Data Pipe 5 */
  static constexpr uint8_t REG_FIFO_STATUS = 0x17; /**< FIFO Status Register */
  static constexpr uint8_t REG_DYNPD       = 0x1C; /**< Enable Dynamic Payload Length for Data Pipes */
  static constexpr uint8_t REG_FEATURE     = 0x1D; /**< Feature Register */

  /*-------------------------------------------------------------------------------
  Register Bit Definitions
  -------------------------------------------------------------------------------*/
  /*-------------------------------------------------
  Config Register (CONFIG)
  -------------------------------------------------*/
  static constexpr uint8_t CONFIG_Mask            = 0x7F;
  static constexpr uint8_t CONFIG_Reset           = 0x08;
  static constexpr uint8_t CONFIG_MASK_RX_DR_Pos  = 6u;
  static constexpr uint8_t CONFIG_MASK_RX_DR_Msk  = 1u << CONFIG_MASK_RX_DR_Pos;
  static constexpr uint8_t CONFIG_MASK_RX_DR      = CONFIG_MASK_RX_DR_Msk;
  static constexpr uint8_t CONFIG_MASK_TX_DS_Pos  = 5u;
  static constexpr uint8_t CONFIG_MASK_TX_DS_Msk  = 1u << CONFIG_MASK_TX_DS_Pos;
  static constexpr uint8_t CONFIG_MASK_TX_DS      = CONFIG_MASK_TX_DS_Msk;
  static constexpr uint8_t CONFIG_MASK_MAX_RT_Pos = 4u;
  static constexpr uint8_t CONFIG_MASK_MAX_RT_Msk = 1u << CONFIG_MASK_MAX_RT_Pos;
  static constexpr uint8_t CONFIG_MASK_MAX_RT     = CONFIG_MASK_MAX_RT_Msk;
  static constexpr uint8_t CONFIG_EN_CRC_Pos      = 3u;
  static constexpr uint8_t CONFIG_EN_CRC_Msk      = 1u << CONFIG_EN_CRC_Pos;
  static constexpr uint8_t CONFIG_EN_CRC          = CONFIG_EN_CRC_Msk;
  static constexpr uint8_t CONFIG_CRCO_Pos        = 2u;
  static constexpr uint8_t CONFIG_CRCO_Msk        = 1u << CONFIG_CRCO_Pos;
  static constexpr uint8_t CONFIG_CRCO            = CONFIG_CRCO_Msk;
  static constexpr uint8_t CONFIG_PWR_UP_Pos      = 1u;
  static constexpr uint8_t CONFIG_PWR_UP_Msk      = 1u << CONFIG_PWR_UP_Pos;
  static constexpr uint8_t CONFIG_PWR_UP          = CONFIG_PWR_UP_Msk;
  static constexpr uint8_t CONFIG_PRIM_RX_Pos     = 0u;
  static constexpr uint8_t CONFIG_PRIM_RX_Msk     = 1u << CONFIG_PRIM_RX_Pos;
  static constexpr uint8_t CONFIG_PRIM_RX         = CONFIG_PRIM_RX_Msk;

  /*-------------------------------------------------
  Enable Auto Acknowledgement Register (EN_AA)
  -------------------------------------------------*/
  static constexpr uint8_t EN_AA_Mask   = 0x3F;
  static constexpr uint8_t EN_AA_Reset  = 0x3F;
  static constexpr uint8_t EN_AA_P5_Pos = 5u;
  static constexpr uint8_t EN_AA_P5_Msk = 1u << EN_AA_P5_Pos;
  static constexpr uint8_t EN_AA_P5     = EN_AA_P5_Msk;
  static constexpr uint8_t EN_AA_P4_Pos = 4u;
  static constexpr uint8_t EN_AA_P4_Msk = 1u << EN_AA_P4_Pos;
  static constexpr uint8_t EN_AA_P4     = EN_AA_P4_Msk;
  static constexpr uint8_t EN_AA_P3_Pos = 3u;
  static constexpr uint8_t EN_AA_P3_Msk = 1u << EN_AA_P3_Pos;
  static constexpr uint8_t EN_AA_P3     = EN_AA_P3_Msk;
  static constexpr uint8_t EN_AA_P2_Pos = 2u;
  static constexpr uint8_t EN_AA_P2_Msk = 1u << EN_AA_P2_Pos;
  static constexpr uint8_t EN_AA_P2     = EN_AA_P2_Msk;
  static constexpr uint8_t EN_AA_P1_Pos = 1u;
  static constexpr uint8_t EN_AA_P1_Msk = 1u << EN_AA_P1_Pos;
  static constexpr uint8_t EN_AA_P1     = EN_AA_P1_Msk;
  static constexpr uint8_t EN_AA_P0_Pos = 0u;
  static constexpr uint8_t EN_AA_P0_Msk = 1u << EN_AA_P0_Pos;
  static constexpr uint8_t EN_AA_P0     = EN_AA_P0_Msk;

  /*-------------------------------------------------
  Enabled RX Address Pipes (EN_RXADDR)
  -------------------------------------------------*/
  static constexpr uint8_t EN_RXADDR_Mask   = 0x3F;
  static constexpr uint8_t EN_RXADDR_Reset  = 0x03;
  static constexpr uint8_t EN_RXADDR_P5_Pos = 5u;
  static constexpr uint8_t EN_RXADDR_P5_Msk = 1u << EN_RXADDR_P5_Pos;
  static constexpr uint8_t EN_RXADDR_P5     = EN_RXADDR_P5_Msk;
  static constexpr uint8_t EN_RXADDR_P4_Pos = 4u;
  static constexpr uint8_t EN_RXADDR_P4_Msk = 1u << EN_RXADDR_P4_Pos;
  static constexpr uint8_t EN_RXADDR_P4     = EN_RXADDR_P4_Msk;
  static constexpr uint8_t EN_RXADDR_P3_Pos = 3u;
  static constexpr uint8_t EN_RXADDR_P3_Msk = 1u << EN_RXADDR_P3_Pos;
  static constexpr uint8_t EN_RXADDR_P3     = EN_RXADDR_P3_Msk;
  static constexpr uint8_t EN_RXADDR_P2_Pos = 2u;
  static constexpr uint8_t EN_RXADDR_P2_Msk = 1u << EN_RXADDR_P2_Pos;
  static constexpr uint8_t EN_RXADDR_P2     = EN_RXADDR_P2_Msk;
  static constexpr uint8_t EN_RXADDR_P1_Pos = 1u;
  static constexpr uint8_t EN_RXADDR_P1_Msk = 1u << EN_RXADDR_P1_Pos;
  static constexpr uint8_t EN_RXADDR_P1     = EN_RXADDR_P1_Msk;
  static constexpr uint8_t EN_RXADDR_P0_Pos = 0u;
  static constexpr uint8_t EN_RXADDR_P0_Msk = 1u << EN_RXADDR_P0_Pos;
  static constexpr uint8_t EN_RXADDR_P0     = EN_RXADDR_P0_Msk;

  /*-------------------------------------------------
  Setup of Address Widths (SETUP_AW)
  -------------------------------------------------*/
  static constexpr uint8_t SETUP_AW_Msk    = 0x03;
  static constexpr uint8_t SETUP_AW_Reset  = 0x03;
  static constexpr uint8_t SETUP_AW_AW_Pos = 0u;
  static constexpr uint8_t SETUP_AW_AW_Wid = 0x03;
  static constexpr uint8_t SETUP_AW_AW_Msk = SETUP_AW_AW_Wid << SETUP_AW_AW_Pos;
  static constexpr uint8_t SETUP_AW_AW     = SETUP_AW_AW_Msk;

  /*-------------------------------------------------
  Setup of Automatic Retransmission (SETUP_RETR)
  -------------------------------------------------*/
  static constexpr uint8_t SETUP_RETR_Mask    = 0xFF;
  static constexpr uint8_t SETUP_RETR_Reset   = 0x03;
  static constexpr uint8_t SETUP_RETR_ARD_Pos = 4u;
  static constexpr uint8_t SETUP_RETR_ARD_Msk = 0x0F << SETUP_RETR_ARD_Pos;
  static constexpr uint8_t SETUP_RETR_ARD     = SETUP_RETR_ARD_Msk;
  static constexpr uint8_t SETUP_RETR_ARC_Pos = 0u;
  static constexpr uint8_t SETUP_RETR_ARC_Msk = 0x0F << SETUP_RETR_ARC_Pos;
  static constexpr uint8_t SETUP_RETR_ARC     = SETUP_RETR_ARC_Msk;

  /*-------------------------------------------------
  RF Channel (RF_CH)
  -------------------------------------------------*/
  static constexpr uint8_t RF_CH_Mask  = 0x7F;
  static constexpr uint8_t RF_CH_Reset = 0x02;

  /*-------------------------------------------------
  RF Setup (RF_SETUP)
  -------------------------------------------------*/
  static constexpr uint8_t RF_SETUP_Mask           = 0x1F;
  static constexpr uint8_t RF_SETUP_Reset          = 0x0F;
  static constexpr uint8_t RF_SETUP_RF_DR_LOW_Pos  = 5u;
  static constexpr uint8_t RF_SETUP_RF_DR_LOW_Msk  = 1u << RF_SETUP_RF_DR_LOW_Pos;
  static constexpr uint8_t RF_SETUP_RF_DR_LOW      = RF_SETUP_RF_DR_LOW_Msk;
  static constexpr uint8_t RF_SETUP_PLL_LOCK_Pos   = 4u;
  static constexpr uint8_t RF_SETUP_PLL_LOCK_Msk   = 1u << RF_SETUP_PLL_LOCK_Pos;
  static constexpr uint8_t RF_SETUP_PLL_LOCK       = RF_SETUP_PLL_LOCK_Msk;
  static constexpr uint8_t RF_SETUP_RF_DR_HIGH_Pos = 3u;
  static constexpr uint8_t RF_SETUP_RF_DR_HIGH_Msk = 1u << RF_SETUP_RF_DR_HIGH_Pos;
  static constexpr uint8_t RF_SETUP_RF_DR_HIGH     = RF_SETUP_RF_DR_HIGH_Msk;
  static constexpr uint8_t RF_SETUP_RF_DR_Pos      = 3u;
  static constexpr uint8_t RF_SETUP_RF_DR_Msk      = 1u << RF_SETUP_RF_DR_Pos;
  static constexpr uint8_t RF_SETUP_RF_DR          = RF_SETUP_RF_DR_Msk;
  static constexpr uint8_t RF_SETUP_RF_PWR_Pos     = 1u;
  static constexpr uint8_t RF_SETUP_RF_PWR_Wid     = 0x03;
  static constexpr uint8_t RF_SETUP_RF_PWR_Msk     = RF_SETUP_RF_PWR_Wid << RF_SETUP_RF_PWR_Pos;
  static constexpr uint8_t RF_SETUP_RF_PWR         = RF_SETUP_RF_PWR_Msk;
  static constexpr uint8_t RF_SETUP_LNA_HCURR_Pos  = 0u;
  static constexpr uint8_t RF_SETUP_LNA_HCURR_Msk  = 1u << RF_SETUP_LNA_HCURR_Pos;
  static constexpr uint8_t RF_SETUP_LNA_HCURR      = RF_SETUP_LNA_HCURR_Msk;

  /*-------------------------------------------------
  Status Register (STATUS)
  -------------------------------------------------*/
  static constexpr uint8_t STATUS_Mask        = 0x7F;
  static constexpr uint8_t STATUS_Reset       = 0x0E;
  static constexpr uint8_t STATUS_RX_DR_Pos   = 6u;
  static constexpr uint8_t STATUS_RX_DR_Msk   = 1u << STATUS_RX_DR_Pos;
  static constexpr uint8_t STATUS_RX_DR       = STATUS_RX_DR_Msk;
  static constexpr uint8_t STATUS_TX_DS_Pos   = 5u;
  static constexpr uint8_t STATUS_TX_DS_Msk   = 1u << STATUS_TX_DS_Pos;
  static constexpr uint8_t STATUS_TX_DS       = STATUS_TX_DS_Msk;
  static constexpr uint8_t STATUS_MAX_RT_Pos  = 4u;
  static constexpr uint8_t STATUS_MAX_RT_Msk  = 1u << STATUS_MAX_RT_Pos;
  static constexpr uint8_t STATUS_MAX_RT      = STATUS_MAX_RT_Msk;
  static constexpr uint8_t STATUS_RX_P_NO_Pos = 1u;
  static constexpr uint8_t STATUS_RX_P_NO_Wid = 0x07;
  static constexpr uint8_t STATUS_RX_P_NO_Msk = STATUS_RX_P_NO_Wid << STATUS_RX_P_NO_Pos;
  static constexpr uint8_t STATUS_RX_P_NO     = STATUS_RX_P_NO_Msk;
  static constexpr uint8_t STATUS_TX_FULL_Pos = 0u;
  static constexpr uint8_t STATUS_TX_FULL_Msk = 1u << STATUS_TX_FULL_Pos;
  static constexpr uint8_t STATUS_TX_FULL     = STATUS_TX_FULL_Msk;

  /*-------------------------------------------------
  Transmit Observe Register (OBSERVE_TX)
  -------------------------------------------------*/
  static constexpr uint8_t OBSERVE_TX_Mask         = 0xFF;
  static constexpr uint8_t OBSERVE_TX_Reset        = 0x00;
  static constexpr uint8_t OBSERVE_TX_PLOS_CNT_Pos = 4u;
  static constexpr uint8_t OBSERVE_TX_PLOS_CNT_Wid = 0x0F;
  static constexpr uint8_t OBSERVE_TX_PLOS_CNT_Msk = OBSERVE_TX_PLOS_CNT_Wid << OBSERVE_TX_PLOS_CNT_Pos;
  static constexpr uint8_t OBSERVE_TX_PLOS_CNT     = OBSERVE_TX_PLOS_CNT_Msk;
  static constexpr uint8_t OBSERVE_TX_ARC_CNT_Pos  = 0u;
  static constexpr uint8_t OBSERVE_TX_ARC_CNT_Wid  = 0x0F;
  static constexpr uint8_t OBSERVE_TX_ARC_CNT_Msk  = OBSERVE_TX_ARC_CNT_Wid << OBSERVE_TX_ARC_CNT_Pos;
  static constexpr uint8_t OBSERVE_TX_ARC_CNT      = OBSERVE_TX_ARC_CNT_Msk;

  /*-------------------------------------------------
  Receive Power Detect Register (RPD)
  -------------------------------------------------*/
  static constexpr uint8_t RPD_Mask    = 0x01;
  static constexpr uint8_t RPD_Reset   = 0x00;
  static constexpr uint8_t RPD_RPD_Pos = 0u;
  static constexpr uint8_t RPD_RPD_Msk = 1u << RPD_RPD_Pos;
  static constexpr uint8_t RPD_RPD     = RPD_RPD_Msk;

  /*-------------------------------------------------
  RX Address Registers (RX_ADDR_Px)
  -------------------------------------------------*/
  static constexpr uint8_t RX_ADDR_P0_byteWidth = 5u;
  static constexpr uint64_t RX_ADDR_P0_Mask     = 0xFFFFFFFFFF;
  static constexpr uint64_t RX_ADDR_P0_Reset    = 0xE7E7E7E7E7;
  static constexpr uint8_t RX_ADDR_P1_byteWidth = 5u;
  static constexpr uint64_t RX_ADDR_P1_Mask     = 0xFFFFFFFFFF;
  static constexpr uint64_t RX_ADDR_P1_Reset    = 0xC2C2C2C2C2;
  static constexpr uint8_t RX_ADDR_P2_Mask      = 0xFF;
  static constexpr uint8_t RX_ADDR_P2_Reset     = 0xC3;
  static constexpr uint8_t RX_ADDR_P3_Mask      = 0xFF;
  static constexpr uint8_t RX_ADDR_P3_Reset     = 0xC4;
  static constexpr uint8_t RX_ADDR_P4_Mask      = 0xFF;
  static constexpr uint8_t RX_ADDR_P4_Reset     = 0xC5;
  static constexpr uint8_t RX_ADDR_P5_Mask      = 0xFF;
  static constexpr uint8_t RX_ADDR_P5_Reset     = 0xC6;

  /*-------------------------------------------------
  TX Address Register (TX_ADDR)
  -------------------------------------------------*/
  static constexpr uint8_t TX_ADDR_byteWidth = 5u;
  static constexpr uint64_t TX_ADDR_Mask     = 0xFFFFFFFFFF;
  static constexpr uint64_t TX_ADDR_Reset    = 0xE7E7E7E7E7;

  /*-------------------------------------------------
  RX Payload Width Registers (RX_PW_Px)
  -------------------------------------------------*/
  static constexpr uint8_t RX_PW_P0_Mask  = 0x3F;
  static constexpr uint8_t RX_PW_P0_Reset = 0x00;
  static constexpr uint8_t RX_PW_P1_Mask  = 0x3F;
  static constexpr uint8_t RX_PW_P1_Reset = 0x00;
  static constexpr uint8_t RX_PW_P2_Mask  = 0x3F;
  static constexpr uint8_t RX_PW_P2_Reset = 0x00;
  static constexpr uint8_t RX_PW_P3_Mask  = 0x3F;
  static constexpr uint8_t RX_PW_P3_Reset = 0x00;
  static constexpr uint8_t RX_PW_P4_Mask  = 0x3F;
  static constexpr uint8_t RX_PW_P4_Reset = 0x00;
  static constexpr uint8_t RX_PW_P5_Mask  = 0x3F;
  static constexpr uint8_t RX_PW_P5_Reset = 0x00;

  /*-------------------------------------------------
  FIFO Status Register (FIFO_STATUS)
  -------------------------------------------------*/
  static constexpr uint8_t FIFO_STATUS_Mask         = 0x7F;
  static constexpr uint8_t FIFO_STATUS_Reset        = 0x00;
  static constexpr uint8_t FIFO_STATUS_TX_REUSE_Pos = 6u;
  static constexpr uint8_t FIFO_STATUS_TX_REUSE_Msk = 1u << FIFO_STATUS_TX_REUSE_Pos;
  static constexpr uint8_t FIFO_STATUS_TX_REUSE     = FIFO_STATUS_TX_REUSE_Msk;
  static constexpr uint8_t FIFO_STATUS_TX_FULL_Pos  = 5u;
  static constexpr uint8_t FIFO_STATUS_TX_FULL_Msk  = 1u << FIFO_STATUS_TX_FULL_Pos;
  static constexpr uint8_t FIFO_STATUS_TX_FULL      = FIFO_STATUS_TX_FULL_Msk;
  static constexpr uint8_t FIFO_STATUS_TX_EMPTY_Pos = 4u;
  static constexpr uint8_t FIFO_STATUS_TX_EMPTY_Msk = 1u << FIFO_STATUS_TX_EMPTY_Pos;
  static constexpr uint8_t FIFO_STATUS_TX_EMPTY     = FIFO_STATUS_TX_EMPTY_Msk;
  static constexpr uint8_t FIFO_STATUS_RX_FULL_Pos  = 1u;
  static constexpr uint8_t FIFO_STATUS_RX_FULL_Msk  = 1u << FIFO_STATUS_RX_FULL_Pos;
  static constexpr uint8_t FIFO_STATUS_RX_FULL      = FIFO_STATUS_RX_FULL_Msk;
  static constexpr uint8_t FIFO_STATUS_RX_EMPTY_Pos = 0u;
  static constexpr uint8_t FIFO_STATUS_RX_EMPTY_Msk = 1u << FIFO_STATUS_RX_EMPTY_Pos;
  static constexpr uint8_t FIFO_STATUS_RX_EMPTY     = FIFO_STATUS_RX_EMPTY_Msk;

  /*-------------------------------------------------
  Dynamic Payload Enable Register (DYNPD)
  -------------------------------------------------*/
  static constexpr uint8_t DYNPD_Mask       = 0x3F;
  static constexpr uint8_t DYNPD_Reset      = 0x00;
  static constexpr uint8_t DYNPD_DPL_P5_Pos = 5u;
  static constexpr uint8_t DYNPD_DPL_P5_Msk = 1u << DYNPD_DPL_P5_Pos;
  static constexpr uint8_t DYNPD_DPL_P5     = DYNPD_DPL_P5_Msk;
  static constexpr uint8_t DYNPD_DPL_P4_Pos = 4u;
  static constexpr uint8_t DYNPD_DPL_P4_Msk = 1u << DYNPD_DPL_P4_Pos;
  static constexpr uint8_t DYNPD_DPL_P4     = DYNPD_DPL_P4_Msk;
  static constexpr uint8_t DYNPD_DPL_P3_Pos = 3u;
  static constexpr uint8_t DYNPD_DPL_P3_Msk = 1u << DYNPD_DPL_P3_Pos;
  static constexpr uint8_t DYNPD_DPL_P3     = DYNPD_DPL_P3_Msk;
  static constexpr uint8_t DYNPD_DPL_P2_Pos = 2u;
  static constexpr uint8_t DYNPD_DPL_P2_Msk = 1u << DYNPD_DPL_P2_Pos;
  static constexpr uint8_t DYNPD_DPL_P2     = DYNPD_DPL_P2_Msk;
  static constexpr uint8_t DYNPD_DPL_P1_Pos = 1u;
  static constexpr uint8_t DYNPD_DPL_P1_Msk = 1u << DYNPD_DPL_P1_Pos;
  static constexpr uint8_t DYNPD_DPL_P1     = DYNPD_DPL_P1_Msk;
  static constexpr uint8_t DYNPD_DPL_P0_Pos = 0u;
  static constexpr uint8_t DYNPD_DPL_P0_Msk = 1u << DYNPD_DPL_P0_Pos;
  static constexpr uint8_t DYNPD_DPL_P0     = DYNPD_DPL_P0_Msk;

  /*-------------------------------------------------
  Feature Register (FEATURE)
  -------------------------------------------------*/
  static constexpr uint8_t FEATURE_MSK            = 0x07;
  static constexpr uint8_t FEATURE_Reset          = 0x00;
  static constexpr uint8_t FEATURE_EN_DPL_Pos     = 2u;
  static constexpr uint8_t FEATURE_EN_DPL_Msk     = 1u << FEATURE_EN_DPL_Pos;
  static constexpr uint8_t FEATURE_EN_DPL         = FEATURE_EN_DPL_Msk;
  static constexpr uint8_t FEATURE_EN_ACK_PAY_Pos = 1u;
  static constexpr uint8_t FEATURE_EN_ACK_PAY_Msk = 1u << FEATURE_EN_ACK_PAY_Pos;
  static constexpr uint8_t FEATURE_EN_ACK_PAY     = FEATURE_EN_ACK_PAY_Msk;
  static constexpr uint8_t FEATURE_EN_DYN_ACK_Pos = 0u;
  static constexpr uint8_t FEATURE_EN_DYN_ACK_Msk = 1u << FEATURE_EN_DYN_ACK_Pos;
  static constexpr uint8_t FEATURE_EN_DYN_ACK     = FEATURE_EN_DYN_ACK_Msk;

}    // namespace Ripple::PHY

#endif /* !RIPPLE_PHYSICAL_DEVICE_REGISTERS_HPP */
