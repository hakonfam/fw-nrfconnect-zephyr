/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EXMIF_SPI_H
#define EXMIF_SPI_H

#include <stdint.h>
#include <stdbool.h>

#define ADDRESS_NOT_VALID 0xFFFFFFFF

typedef enum {
	EXMIF_SPI_TX = 0,
	EXMIF_SPI_RX = 1,
} exmif_spi_direction_t;

typedef struct {
	uint8_t command;
	uint32_t address;
	exmif_spi_direction_t direction;
	uint8_t *data_buf;
	uint32_t data_bytes;
	bool use_4byte_addr;
} exmif_spi_transfer_req_t;

/**
 * @brief Initialize the EXMIF SPI controller
 * @return 0 on success, negative error code on failure
 */
int exmif_spi_init(void);

/**
 * @brief Perform an SPI transfer
 * @param req Transfer request parameters
 * @return 0 on success, negative error code on failure
 */
int exmif_spi_transceive(const exmif_spi_transfer_req_t *req);

/* Standard SPI NOR Flash Commands (JEDEC SFDP compatible) */
#define FLASH_CMD_WRITE_ENABLE       0x06
#define FLASH_CMD_WRITE_DISABLE      0x04
#define FLASH_CMD_READ_STATUS_1      0x05
#define FLASH_CMD_READ_STATUS_2      0x35
#define FLASH_CMD_WRITE_STATUS       0x01
#define FLASH_CMD_READ_DATA          0x03
#define FLASH_CMD_READ_DATA_4BYTE    0x13
#define FLASH_CMD_PAGE_PROGRAM       0x02
#define FLASH_CMD_PAGE_PROGRAM_4BYTE 0x12
#define FLASH_CMD_SECTOR_ERASE       0x20
#define FLASH_CMD_SECTOR_ERASE_4BYTE 0x21
#define FLASH_CMD_BLOCK_ERASE_32K    0x52
#define FLASH_CMD_BLOCK_ERASE_64K    0xD8
#define FLASH_CMD_CHIP_ERASE         0xC7
#define FLASH_CMD_READ_JEDEC_ID      0x9F
#define FLASH_CMD_ENTER_4BYTE_MODE   0xB7
#define FLASH_CMD_EXIT_4BYTE_MODE    0xE9
#define FLASH_CMD_RELEASE_POWERDOWN  0xAB

/* Status Register bits */
#define FLASH_STATUS_BUSY            0x01
#define FLASH_STATUS_WEL             0x02

#endif /* EXMIF_SPI_H */
