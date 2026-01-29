/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "exmif_spi.h"
#include "snapshot_types.h"

LOG_MODULE_REGISTER(flash_test, CONFIG_LOG_DEFAULT_LEVEL);

/* W25Q256FW Flash Parameters */
#define FLASH_SECTOR_SIZE    CONFIG_EXMIF_FLASH_TEST_SECTOR_SIZE
#define FLASH_PAGE_SIZE      CONFIG_EXMIF_FLASH_TEST_PAGE_SIZE

#define FLASH_TEST_SIZE      CONFIG_EXMIF_DUMP_SIZE

#define RAM_SIZE (512*1024 + 256 *1024)
BUILD_ASSERT(FLASH_TEST_SIZE <= RAM_SIZE, "Not enough RAM");

static int flash_read_jedec_id(uint8_t *id)
{
	exmif_spi_transfer_req_t req = {
		.command = FLASH_CMD_READ_JEDEC_ID,
		.address = ADDRESS_NOT_VALID,
		.direction = EXMIF_SPI_RX,
		.data_buf = id,
		.data_bytes = 3,
		.use_4byte_addr = false,
	};

	return exmif_spi_transceive(&req);
}

static int flash_read_data(uint32_t address, uint8_t *data, uint32_t len)
{
	exmif_spi_transfer_req_t req = {
		.command = FLASH_CMD_READ_DATA,
		.address = address,
		.direction = EXMIF_SPI_RX,
		.data_buf = data,
		.data_bytes = len,
		.use_4byte_addr = false,
	};

	return exmif_spi_transceive(&req);
}

int main(void)
{
	int ret;
	uint8_t jedec_id[3];
	const uint32_t total_sectors = FLASH_TEST_SIZE / FLASH_SECTOR_SIZE;
	uint8_t *read_buffer = (uint8_t *)0x2f000000;
	volatile recovery_manifest *manifest = (recovery_manifest *)0x2f000000;


	/* Initialize EXMIF */
	ret = exmif_spi_init();
	if (ret != 0) {
		LOG_ERR("EXMIF init failed: %d", ret);
		return ret;
	}

	/* Read JEDEC ID */
	ret = flash_read_jedec_id(jedec_id);
	if (ret != 0) {
		LOG_ERR("Failed to read JEDEC ID: %d", ret);
		return ret;
	}

	LOG_INF("JEDEC ID: %02x %02x %02x", jedec_id[0], jedec_id[1], jedec_id[2]);
	uint32_t start_time = k_uptime_get_32();

	/* Verify it's a Winbond flash */
	if (jedec_id[0] != 0xEF) {
		LOG_WRN("Unexpected manufacturer ID (expected 0xEF for Winbond)");
	}

	/* Read and dump all sectors */
	for (uint32_t sector = 0; sector < total_sectors; sector++) {
		uint32_t sector_address = sector * FLASH_SECTOR_SIZE;

		for (uint32_t page_offset = 0; page_offset < FLASH_SECTOR_SIZE;
				page_offset += FLASH_PAGE_SIZE) {
			uint32_t page_address = sector_address + page_offset;

			memset(read_buffer, 0xFF, FLASH_PAGE_SIZE);

			ret = flash_read_data(page_address, read_buffer, FLASH_PAGE_SIZE);
			if (ret != 0) {
				LOG_ERR("  Read failed at 0x%08x", page_address);
				return ret;
			}

			read_buffer += FLASH_PAGE_SIZE;
		}
		LOG_INF(".");
	}

	uint32_t elapsed_ms = k_uptime_get_32() - start_time;

	/* Finish marker */
	volatile const uint32_t end = (0x2f000000 + RAM_SIZE);
	*(uint32_t *)(end - 8) = 0xdeadbeef;
	*(uint32_t *)(end - 4) = elapsed_ms; /* some semi-random data */

	LOG_INF("Time elapsed: %u.%03u seconds", elapsed_ms / 1000, elapsed_ms % 1000);
	LOG_INF("manifest %p", (void *)manifest);

	return 0;
}
