/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <hal/nrf_gpio.h>

#include "exmif_spi.h"

LOG_MODULE_REGISTER(exmif_spi, CONFIG_LOG_DEFAULT_LEVEL);

static NRF_EXMIF_Type *exmif = (NRF_EXMIF_Type *)DT_REG_ADDR(DT_NODELABEL(exmif));

static void exmif_init_gpios(void)
{
	LOG_INF("Configuring EXMIF GPIOs");

	const uint32_t pin_mask = ((1 << 0) |   /* CLK */
				   (1 << 3) |   /* CS_n */
				   (1 << 5) |   /* DQ1/MISO */
				   (1 << 7) |   /* DQ0/MOSI */
				   (1 << 9) |   /* DQ3/HOLD */
				   (1 << 10));  /* DQ2/WP_n */

	NRF_P6_S->RETAINCLR = pin_mask;

	NRF_P6_S->PORTCNF.DRIVECTRL =
		(GPIO_PORTCNF_DRIVECTRL_IMPEDANCE50_Enable << GPIO_PORTCNF_DRIVECTRL_IMPEDANCE50_Pos);

	for (uint32_t i = 0; i < 14; i++) {
		if ((1 << i) & pin_mask) {
			NRF_P6_S->PIN_CNF[i] =
				((GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
				 (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
				 (GPIO_PIN_CNF_DRIVE0_H0 << GPIO_PIN_CNF_DRIVE0_Pos) |
				 (GPIO_PIN_CNF_DRIVE1_H1 << GPIO_PIN_CNF_DRIVE1_Pos) |
				 (GPIO_PIN_CNF_CTRLSEL_EXMIF << GPIO_PIN_CNF_CTRLSEL_Pos));
		}
	}
}

static void exmif_reset_module(void)
{
	uint32_t delay_us = 5;

	LOG_DBG("Resetting EXMIF module");
	exmif->RESET = 0x01;
	k_busy_wait(delay_us);
	exmif->RESET = 0x00;
	k_busy_wait(delay_us);
}

static inline void exmif_enable_slave(void)
{
	exmif->CORE.SSICADDRESS.SER =
		((EXMIF_CORE_SSICADDRESS_SER_SER_SELECTED << EXMIF_CORE_SSICADDRESS_SER_SER_Pos) &
		 EXMIF_CORE_SSICADDRESS_SER_SER_Msk);
}

static inline void exmif_disable_slave(void)
{
	exmif->CORE.SSICADDRESS.SER =
		((EXMIF_CORE_SSICADDRESS_SER_SER_NOTSELECTED << EXMIF_CORE_SSICADDRESS_SER_SER_Pos) &
		 EXMIF_CORE_SSICADDRESS_SER_SER_Msk);
}

static inline void exmif_enable_ssi_controller(void)
{
	exmif->CORE.SSICADDRESS.SSIENR =
		((EXMIF_CORE_SSICADDRESS_SSIENR_SSICEN_ENABLED
		  << EXMIF_CORE_SSICADDRESS_SSIENR_SSICEN_Pos) &
		 EXMIF_CORE_SSICADDRESS_SSIENR_SSICEN_Msk);
}

static inline void exmif_disable_ssi_controller(void)
{
	exmif->CORE.SSICADDRESS.SSIENR =
		((EXMIF_CORE_SSICADDRESS_SSIENR_SSICEN_DISABLE
		  << EXMIF_CORE_SSICADDRESS_SSIENR_SSICEN_Pos) &
		 EXMIF_CORE_SSICADDRESS_SSIENR_SSICEN_Msk);
}

static void exmif_configure_transfer(const exmif_spi_transfer_req_t *req)
{
	uint32_t ctrlr0 = 0;
	uint32_t ctrlr1 = 0;
	uint32_t tmod;
	uint32_t addr_len;

	(void)exmif->CORE.SSICADDRESS.SR;

	/* Power up EXMIF */
	exmif->TASKS_START = 0x1;
	while (!(exmif->EVENTS_STARTED & 0x1)) {
		k_busy_wait(1);
	}

	exmif_disable_ssi_controller();

	/* Determine transfer mode */
	if (req->direction == EXMIF_SPI_RX) {
		tmod = EXMIF_CORE_SSICADDRESS_CTRLR0_TMOD_EEPROM_READ;
	} else {
		tmod = EXMIF_CORE_SSICADDRESS_CTRLR0_TMOD_TX_ONLY;
	}

	/* Configure data frame size (8-bit) */
	ctrlr0 |= ((EXMIF_CORE_SSICADDRESS_CTRLR0_DFS_DFS_08_BIT
		    << EXMIF_CORE_SSICADDRESS_CTRLR0_DFS_Pos) &
		   EXMIF_CORE_SSICADDRESS_CTRLR0_DFS_Msk);

	/* Configure frame format (SPI) */
	ctrlr0 |= ((EXMIF_CORE_SSICADDRESS_CTRLR0_FRF_SPI << EXMIF_CORE_SSICADDRESS_CTRLR0_FRF_Pos) &
		   EXMIF_CORE_SSICADDRESS_CTRLR0_FRF_Msk);

	/* Configure transfer mode */
	ctrlr0 |= ((tmod << EXMIF_CORE_SSICADDRESS_CTRLR0_TMOD_Pos) &
		   EXMIF_CORE_SSICADDRESS_CTRLR0_TMOD_Msk);

	/* Configure clock polarity and phase (Mode 0: CPOL=0, CPHA=0) */
	ctrlr0 |= ((EXMIF_CORE_SSICADDRESS_CTRLR0_SCPH_START_BIT
		    << EXMIF_CORE_SSICADDRESS_CTRLR0_SCPH_Pos) &
		   EXMIF_CORE_SSICADDRESS_CTRLR0_SCPH_Msk);
	ctrlr0 |= ((EXMIF_CORE_SSICADDRESS_CTRLR0_SCPOL_INACTIVE_LOW
		    << EXMIF_CORE_SSICADDRESS_CTRLR0_SCPOL_Pos) &
		   EXMIF_CORE_SSICADDRESS_CTRLR0_SCPOL_Msk);

	/* Standard SPI mode (1-bit) */
	ctrlr0 |= ((EXMIF_CORE_SSICADDRESS_CTRLR0_SPIFRF_SPI_STANDARD
		    << EXMIF_CORE_SSICADDRESS_CTRLR0_SPIFRF_Pos) &
		   EXMIF_CORE_SSICADDRESS_CTRLR0_SPIFRF_Msk);

	/* Set number of data frames for RX modes */
	if (tmod == EXMIF_CORE_SSICADDRESS_CTRLR0_TMOD_RX_ONLY ||
	    tmod == EXMIF_CORE_SSICADDRESS_CTRLR0_TMOD_EEPROM_READ) {
		ctrlr1 = (req->data_bytes > 0) ? (req->data_bytes - 1) : 0;
	}

	/* Determine address length */
	if (req->use_4byte_addr) {
		addr_len = EXMIF_CORE_SSICADDRESS_SPICTRLR0_ADDRL_ADDR_L32;
	} else {
		addr_len = EXMIF_CORE_SSICADDRESS_SPICTRLR0_ADDRL_ADDR_L24;
	}

	exmif->CORE.SSICADDRESS.CTRLR0 = ctrlr0;
	exmif->CORE.SSICADDRESS.CTRLR1 = ctrlr1;

	exmif->CORE.SSICADDRESS.SPICTRLR0 =
		((EXMIF_CORE_SSICADDRESS_SPICTRLR0_TRANSTYPE_TT0
		  << EXMIF_CORE_SSICADDRESS_SPICTRLR0_TRANSTYPE_Pos) &
		 EXMIF_CORE_SSICADDRESS_SPICTRLR0_TRANSTYPE_Msk) |
		((addr_len << EXMIF_CORE_SSICADDRESS_SPICTRLR0_ADDRL_Pos) &
		 EXMIF_CORE_SSICADDRESS_SPICTRLR0_ADDRL_Msk) |
		((EXMIF_CORE_SSICADDRESS_SPICTRLR0_INSTL_INST_L8
		  << EXMIF_CORE_SSICADDRESS_SPICTRLR0_INSTL_Pos) &
		 EXMIF_CORE_SSICADDRESS_SPICTRLR0_INSTL_Msk) |
		((1UL << EXMIF_CORE_SSICADDRESS_SPICTRLR0_CLKSTRETCHEN_Pos) &
		 EXMIF_CORE_SSICADDRESS_SPICTRLR0_CLKSTRETCHEN_Msk);

	/* Set baud rate divider (higher = slower clock)
	 * EXMIF clock is typically 320MHz, so:
	 *   divider 8  = 40 MHz
	 *   divider 16 = 20 MHz
	 *   divider 32 = 10 MHz
	 *   divider 64 =  5 MHz
	 */
	exmif->CORE.SSICADDRESS.BAUDR =
		((32 << EXMIF_CORE_SSICADDRESS_BAUDR_SCKDV_Pos) &
		 EXMIF_CORE_SSICADDRESS_BAUDR_SCKDV_Msk);

	/* Set FIFO thresholds */
	exmif->CORE.SSICADDRESS.TXFTLR =
		((0UL << EXMIF_CORE_SSICADDRESS_TXFTLR_TFT_Pos) &
		 EXMIF_CORE_SSICADDRESS_TXFTLR_TFT_Msk);
	exmif->CORE.SSICADDRESS.RXFTLR =
		((0UL << EXMIF_CORE_SSICADDRESS_RXFTLR_RFT_Pos) &
		 EXMIF_CORE_SSICADDRESS_RXFTLR_RFT_Msk);

	exmif_disable_slave();
}

static int exmif_wait_transfer_complete(void)
{
	uint32_t timeout = 100000;

	while (timeout--) {
		uint32_t sr = exmif->CORE.SSICADDRESS.SR;
		if (!(sr & EXMIF_CORE_SSICADDRESS_SR_BUSY_Msk)) {
			return 0;
		}
		k_busy_wait(1);
	}

	LOG_ERR("Transfer timeout");
	return -ETIMEDOUT;
}

static int exmif_tx_data(const exmif_spi_transfer_req_t *req)
{
	bool slave_enabled = false;
	uint8_t prefix[6];
	uint32_t prefix_len = 0;

	/* Add command */
	prefix[prefix_len++] = req->command;

	/* Add address if valid */
	if (req->address != ADDRESS_NOT_VALID) {
		if (req->use_4byte_addr) {
			prefix[prefix_len++] = (uint8_t)((req->address >> 24) & 0xFF);
		}
		prefix[prefix_len++] = (uint8_t)((req->address >> 16) & 0xFF);
		prefix[prefix_len++] = (uint8_t)((req->address >> 8) & 0xFF);
		prefix[prefix_len++] = (uint8_t)(req->address & 0xFF);
	}

	/* Calculate total TX length */
	uint32_t total_tx_len = prefix_len;
	if (req->direction == EXMIF_SPI_TX && req->data_buf != NULL) {
		total_tx_len += req->data_bytes;
	}

	for (uint32_t i = 0; i < total_tx_len; i++) {
		uint8_t byte_to_write;

		if (i < prefix_len) {
			byte_to_write = prefix[i];
		} else {
			byte_to_write = req->data_buf[i - prefix_len];
		}

		/* Wait for TX FIFO space */
		if (!(exmif->CORE.SSICADDRESS.SR & EXMIF_CORE_SSICADDRESS_SR_TFNF_Msk)) {
			if (!slave_enabled) {
				exmif_enable_slave();
				slave_enabled = true;
			}
			while (!(exmif->CORE.SSICADDRESS.SR & EXMIF_CORE_SSICADDRESS_SR_TFNF_Msk)) {
				__NOP();
			}
		}
		exmif->CORE.SSICADDRESS.DR[0] = byte_to_write;
	}

	if (!slave_enabled) {
		exmif_enable_slave();
	}

	return 0;
}

static int exmif_rx_data(uint8_t *data, uint32_t len)
{
	uint32_t timeout = 100000;
	uint32_t original_len = len;

	while (len && timeout) {
		uint32_t rxflr = exmif->CORE.SSICADDRESS.RXFLR & EXMIF_CORE_SSICADDRESS_RXFLR_RXTFL_Msk;
		if (rxflr == 0) {
			/* No data available, wait a bit */
			k_busy_wait(1);
			timeout--;
			continue;
		}
		while (rxflr && len) {
			*data++ = exmif->CORE.SSICADDRESS.DR[0];
			rxflr--;
			len--;
		}
		/* Reset timeout when we receive data */
		timeout = 100000;
	}

	if (len > 0) {
		LOG_ERR("RX timeout: received %u of %u bytes", original_len - len, original_len);
		return -ETIMEDOUT;
	}

	return 0;
}

int exmif_spi_init(void)
{
	LOG_INF("Initializing EXMIF SPI");

	exmif_init_gpios();

	/* Power up EXMIF */
	exmif->TASKS_START = 0x1;
	while (!(exmif->EVENTS_STARTED & 0x1)) {
		k_busy_wait(1);
	}

	exmif_reset_module();

	exmif_enable_ssi_controller();
	k_busy_wait(5);
	exmif_disable_ssi_controller();

	LOG_INF("EXMIF SPI initialized");
	return 0;
}

int exmif_spi_transceive(const exmif_spi_transfer_req_t *req)
{
	int ret;

	exmif_configure_transfer(req);
	exmif_enable_ssi_controller();

	switch (req->direction) {
	case EXMIF_SPI_TX:
		ret = exmif_tx_data(req);
		break;

	case EXMIF_SPI_RX:
		ret = exmif_tx_data(req);
		if (ret != 0) {
			break;
		}
		ret = exmif_rx_data(req->data_buf, req->data_bytes);
		break;

	default:
		LOG_ERR("Invalid direction");
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		ret = exmif_wait_transfer_complete();
	}

	exmif_disable_slave();
	exmif_disable_ssi_controller();

	return ret;
}
