/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "exmif_spi.h"

LOG_MODULE_REGISTER(flash_test, CONFIG_LOG_DEFAULT_LEVEL);

/* W25Q256FW Flash Parameters */
#define FLASH_SIZE_16MB      0x1000000   /* 16 MB (24-bit addressing) */
#define FLASH_SIZE_32MB      0x2000000   /* 32 MB (full flash) */
#define FLASH_SECTOR_SIZE    CONFIG_EXMIF_FLASH_TEST_SECTOR_SIZE
#define FLASH_PAGE_SIZE      CONFIG_EXMIF_FLASH_TEST_PAGE_SIZE

/* Test configuration */
#if defined(CONFIG_EXMIF_FLASH_TEST_FULL_RANGE) && CONFIG_EXMIF_FLASH_TEST_FULL_RANGE
#define FLASH_TEST_SIZE      FLASH_SIZE_32MB
#define USE_4BYTE_ADDR       true
#else
#define FLASH_TEST_SIZE      FLASH_SIZE_16MB
#define USE_4BYTE_ADDR       false
#endif

/* Buffer for read/write operations */
static uint8_t page_buffer[FLASH_PAGE_SIZE];
static uint8_t verify_buffer[FLASH_PAGE_SIZE];

static int flash_read_status(uint8_t *status)
{
	exmif_spi_transfer_req_t req = {
		.command = FLASH_CMD_READ_STATUS_1,
		.address = ADDRESS_NOT_VALID,
		.direction = EXMIF_SPI_RX,
		.data_buf = status,
		.data_bytes = 1,
		.use_4byte_addr = false,
	};

	return exmif_spi_transceive(&req);
}

static int flash_wait_ready(uint32_t timeout_ms)
{
	uint8_t status;
	uint32_t start = k_uptime_get_32();

	while ((k_uptime_get_32() - start) < timeout_ms) {
		int ret = flash_read_status(&status);
		if (ret != 0) {
			return ret;
		}

		if (!(status & FLASH_STATUS_BUSY)) {
			return 0;
		}

		k_msleep(1);
	}

	LOG_ERR("Flash busy timeout");
	return -ETIMEDOUT;
}

static int flash_write_enable(void)
{
	exmif_spi_transfer_req_t req = {
		.command = FLASH_CMD_WRITE_ENABLE,
		.address = ADDRESS_NOT_VALID,
		.direction = EXMIF_SPI_TX,
		.data_buf = NULL,
		.data_bytes = 0,
		.use_4byte_addr = false,
	};

	int ret = exmif_spi_transceive(&req);
	if (ret != 0) {
		return ret;
	}

	/* Verify WEL bit is set */
	uint8_t status;
	ret = flash_read_status(&status);
	if (ret != 0) {
		return ret;
	}

	if (!(status & FLASH_STATUS_WEL)) {
		LOG_ERR("Write enable failed");
		return -EIO;
	}

	return 0;
}

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

static int flash_enter_4byte_mode(void)
{
	exmif_spi_transfer_req_t req = {
		.command = FLASH_CMD_ENTER_4BYTE_MODE,
		.address = ADDRESS_NOT_VALID,
		.direction = EXMIF_SPI_TX,
		.data_buf = NULL,
		.data_bytes = 0,
		.use_4byte_addr = false,
	};

	LOG_INF("Entering 4-byte address mode");
	return exmif_spi_transceive(&req);
}

static int flash_sector_erase(uint32_t address, bool use_4byte)
{
	int ret;

	ret = flash_write_enable();
	if (ret != 0) {
		return ret;
	}

	exmif_spi_transfer_req_t req = {
		.command = use_4byte ? FLASH_CMD_SECTOR_ERASE_4BYTE : FLASH_CMD_SECTOR_ERASE,
		.address = address,
		.direction = EXMIF_SPI_TX,
		.data_buf = NULL,
		.data_bytes = 0,
		.use_4byte_addr = use_4byte,
	};

	ret = exmif_spi_transceive(&req);
	if (ret != 0) {
		return ret;
	}

	/* Wait for erase to complete (sector erase can take up to 400ms) */
	return flash_wait_ready(500);
}

static int flash_page_program(uint32_t address, const uint8_t *data, uint32_t len, bool use_4byte)
{
	int ret;

	if (len > FLASH_PAGE_SIZE) {
		LOG_ERR("Page program size exceeds page size");
		return -EINVAL;
	}

	ret = flash_write_enable();
	if (ret != 0) {
		return ret;
	}

	exmif_spi_transfer_req_t req = {
		.command = use_4byte ? FLASH_CMD_PAGE_PROGRAM_4BYTE : FLASH_CMD_PAGE_PROGRAM,
		.address = address,
		.direction = EXMIF_SPI_TX,
		.data_buf = (uint8_t *)data,
		.data_bytes = len,
		.use_4byte_addr = use_4byte,
	};

	ret = exmif_spi_transceive(&req);
	if (ret != 0) {
		return ret;
	}

	/* Wait for program to complete (page program can take up to 3ms) */
	return flash_wait_ready(10);
}

static int flash_read_data(uint32_t address, uint8_t *data, uint32_t len, bool use_4byte)
{
	exmif_spi_transfer_req_t req = {
		.command = use_4byte ? FLASH_CMD_READ_DATA_4BYTE : FLASH_CMD_READ_DATA,
		.address = address,
		.direction = EXMIF_SPI_RX,
		.data_buf = data,
		.data_bytes = len,
		.use_4byte_addr = use_4byte,
	};

	return exmif_spi_transceive(&req);
}

static void generate_test_pattern(uint8_t *buf, uint32_t len, uint32_t seed)
{
	for (uint32_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)((seed + i) ^ (seed >> 8));
	}
}

static int verify_erased(uint32_t address, uint32_t len, bool use_4byte)
{
	int ret;
	uint32_t offset = 0;

	while (offset < len) {
		uint32_t chunk = MIN(len - offset, FLASH_PAGE_SIZE);

		ret = flash_read_data(address + offset, verify_buffer, chunk, use_4byte);
		if (ret != 0) {
			LOG_ERR("Read failed at 0x%08x", address + offset);
			return ret;
		}

		for (uint32_t i = 0; i < chunk; i++) {
			if (verify_buffer[i] != 0xFF) {
				LOG_ERR("Erase verify failed at 0x%08x: expected 0xFF, got 0x%02x",
					address + offset + i, verify_buffer[i]);
				return -EIO;
			}
		}

		offset += chunk;
	}

	return 0;
}

static int test_sector(uint32_t sector_address, bool use_4byte)
{
	int ret;

	LOG_INF("Testing sector at 0x%08x", sector_address);

	/* Step 1: Erase sector */
	LOG_DBG("  Erasing...");
	ret = flash_sector_erase(sector_address, use_4byte);
	if (ret != 0) {
		LOG_ERR("  Erase failed");
		return ret;
	}

#if !defined(CONFIG_EXMIF_FLASH_TEST_ERASE_ONLY) || !CONFIG_EXMIF_FLASH_TEST_ERASE_ONLY
	/* Step 2: Verify erased */
	LOG_DBG("  Verifying erase...");
	ret = verify_erased(sector_address, FLASH_SECTOR_SIZE, use_4byte);
	if (ret != 0) {
		LOG_ERR("  Erase verify failed");
		return ret;
	}

	/* Step 3: Write test pattern to each page in sector */
	LOG_DBG("  Writing...");
	for (uint32_t page_offset = 0; page_offset < FLASH_SECTOR_SIZE;
	     page_offset += FLASH_PAGE_SIZE) {
		uint32_t page_address = sector_address + page_offset;

		generate_test_pattern(page_buffer, FLASH_PAGE_SIZE, page_address);

		ret = flash_page_program(page_address, page_buffer, FLASH_PAGE_SIZE, use_4byte);
		if (ret != 0) {
			LOG_ERR("  Page program failed at 0x%08x", page_address);
			return ret;
		}
	}

	/* Step 4: Read back and verify */
	LOG_DBG("  Verifying data...");
	for (uint32_t page_offset = 0; page_offset < FLASH_SECTOR_SIZE;
	     page_offset += FLASH_PAGE_SIZE) {
		uint32_t page_address = sector_address + page_offset;

		generate_test_pattern(page_buffer, FLASH_PAGE_SIZE, page_address);

		ret = flash_read_data(page_address, verify_buffer, FLASH_PAGE_SIZE, use_4byte);
		if (ret != 0) {
			LOG_ERR("  Read failed at 0x%08x", page_address);
			return ret;
		}

		if (memcmp(page_buffer, verify_buffer, FLASH_PAGE_SIZE) != 0) {
			LOG_ERR("  Data verify failed at 0x%08x", page_address);
			for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
				if (page_buffer[i] != verify_buffer[i]) {
					LOG_ERR("    Byte %u: expected 0x%02x, got 0x%02x",
						i, page_buffer[i], verify_buffer[i]);
					break;
				}
			}
			return -EIO;
		}
	}

	/* Step 5: Erase again and verify */
	LOG_DBG("  Final erase...");
	ret = flash_sector_erase(sector_address, use_4byte);
	if (ret != 0) {
		LOG_ERR("  Final erase failed");
		return ret;
	}

	ret = verify_erased(sector_address, FLASH_SECTOR_SIZE, use_4byte);
	if (ret != 0) {
		LOG_ERR("  Final erase verify failed");
		return ret;
	}
#endif /* !CONFIG_EXMIF_FLASH_TEST_ERASE_ONLY */

	LOG_INF("  Sector 0x%08x: PASSED", sector_address);
	return 0;
}

int main(void)
{
	int ret;
	uint8_t jedec_id[3];
	uint32_t sectors_tested = 0;
	uint32_t sectors_failed = 0;
	uint32_t total_sectors = FLASH_TEST_SIZE / FLASH_SECTOR_SIZE;

	LOG_INF("===========================================");
	LOG_INF("EXMIF Flash Test Sample");
	LOG_INF("===========================================");
	LOG_INF("Flash size to test: %u MB", FLASH_TEST_SIZE / (1024 * 1024));
	LOG_INF("Sector size: %u bytes", FLASH_SECTOR_SIZE);
	LOG_INF("Total sectors: %u", total_sectors);
#if defined(CONFIG_EXMIF_FLASH_TEST_ERASE_ONLY) && CONFIG_EXMIF_FLASH_TEST_ERASE_ONLY
	LOG_INF("Mode: ERASE ONLY");
#else
	LOG_INF("Mode: FULL TEST (write/verify/erase)");
#endif
	LOG_INF("===========================================");

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

	/* Verify it's a Winbond flash */
	if (jedec_id[0] != 0xEF) {
		LOG_WRN("Unexpected manufacturer ID (expected 0xEF for Winbond)");
	}

	/* Enter 4-byte address mode if testing full range */
	if (USE_4BYTE_ADDR) {
		ret = flash_enter_4byte_mode();
		if (ret != 0) {
			LOG_ERR("Failed to enter 4-byte mode: %d", ret);
			return ret;
		}
	}

	LOG_INF("Starting flash test...");
	uint32_t start_time = k_uptime_get_32();

	/* Test each sector */
	for (uint32_t sector = 0; sector < total_sectors; sector++) {
		uint32_t sector_address = sector * FLASH_SECTOR_SIZE;

		ret = test_sector(sector_address, USE_4BYTE_ADDR);
		if (ret != 0) {
			sectors_failed++;
			LOG_ERR("Sector %u (0x%08x) FAILED", sector, sector_address);
		} else {
			sectors_tested++;
		}

		/* Progress report every 256 sectors (1MB) */
		if ((sector + 1) % 256 == 0) {
			uint32_t mb_done = (sector + 1) * FLASH_SECTOR_SIZE / (1024 * 1024);
			LOG_INF("Progress: %u MB / %u MB tested",
				mb_done, FLASH_TEST_SIZE / (1024 * 1024));
		}
	}

	uint32_t elapsed_ms = k_uptime_get_32() - start_time;

	LOG_INF("===========================================");
	LOG_INF("Flash Test Complete");
	LOG_INF("===========================================");
	LOG_INF("Sectors tested: %u", sectors_tested);
	LOG_INF("Sectors failed: %u", sectors_failed);
	LOG_INF("Time elapsed: %u.%03u seconds", elapsed_ms / 1000, elapsed_ms % 1000);
	if (sectors_failed == 0) {
		LOG_INF("Result: ALL TESTS PASSED");
	} else {
		LOG_ERR("Result: %u FAILURES", sectors_failed);
	}
	LOG_INF("===========================================");

	return (sectors_failed == 0) ? 0 : -1;
}
