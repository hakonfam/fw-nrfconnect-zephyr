.. zephyr:code-sample:: exmif-flash-test
   :name: EXMIF Flash Test
   :relevant-api: flash_interface

   Test external SPI flash via EXMIF controller (W25Q256FW).

Overview
********

This sample tests an external SPI flash chip connected via the EXMIF
(External Memory Interface) controller on nRF54H20.

The sample performs the following operations on each sector:

1. Erase the sector
2. Verify the sector is erased (all 0xFF)
3. Write a test pattern to each page
4. Read back and verify the data
5. Erase the sector again
6. Verify the sector is erased

Configuration Options
*********************

The following Kconfig options are available:

``CONFIG_EXMIF_FLASH_TEST_ERASE_ONLY``
   When enabled, only erases the flash without writing/verifying data.
   Useful for quickly resetting the flash to a known erased state.

``CONFIG_EXMIF_FLASH_TEST_FULL_RANGE``
   When enabled, tests the entire 32MB flash range using 4-byte addressing.
   When disabled (default), only tests the first 16MB using 24-bit addressing.

``CONFIG_EXMIF_FLASH_TEST_SECTOR_SIZE``
   The erase sector size of the flash chip (default: 4096).

``CONFIG_EXMIF_FLASH_TEST_PAGE_SIZE``
   The page program size of the flash chip (default: 256).

Hardware Setup
**************

Connect the W25Q256FW flash to the following GPIO pins:

+----------+--------+------------------+
| Signal   | GPIO   | Flash Pin        |
+==========+========+==================+
| CLK      | P6.0   | Pin 6 (CLK)      |
+----------+--------+------------------+
| CS#      | P6.3   | Pin 1 (CS#)      |
+----------+--------+------------------+
| DQ0/MOSI | P6.7   | Pin 5 (DI)       |
+----------+--------+------------------+
| DQ1/MISO | P6.5   | Pin 2 (DO)       |
+----------+--------+------------------+
| DQ2/WP#  | P6.10  | Pin 3 (WP#)      |
+----------+--------+------------------+
| DQ3/HOLD | P6.9   | Pin 7 (HOLD#)    |
+----------+--------+------------------+
| VCC      | 1.8V   | Pin 8 (VCC)      |
+----------+--------+------------------+
| GND      | GND    | Pin 4 (GND)      |
+----------+--------+------------------+

Building and Running
********************

Build the sample for nRF54H20:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/drivers/exmif_flash_test

To test the full 32MB range with 4-byte addressing:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/drivers/exmif_flash_test -- \
     -DCONFIG_EXMIF_FLASH_TEST_FULL_RANGE=y

To only erase the flash (skip write/verify):

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/drivers/exmif_flash_test -- \
     -DCONFIG_EXMIF_FLASH_TEST_ERASE_ONLY=y

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS ***
   [00:00:00.000,000] <inf> flash_test: ===========================================
   [00:00:00.000,000] <inf> flash_test: EXMIF Flash Test Sample
   [00:00:00.000,000] <inf> flash_test: ===========================================
   [00:00:00.000,000] <inf> flash_test: Flash size to test: 16 MB
   [00:00:00.000,000] <inf> flash_test: Sector size: 4096 bytes
   [00:00:00.000,000] <inf> flash_test: Total sectors: 4096
   [00:00:00.000,000] <inf> flash_test: Mode: FULL TEST (write/verify/erase)
   [00:00:00.000,000] <inf> flash_test: ===========================================
   [00:00:00.001,000] <inf> exmif_spi: Initializing EXMIF SPI
   [00:00:00.002,000] <inf> flash_test: JEDEC ID: ef 60 19
   [00:00:00.002,000] <inf> flash_test: Starting flash test...
   [00:00:00.002,000] <inf> flash_test: Testing sector at 0x00000000
   [00:00:00.050,000] <inf> flash_test:   Sector 0x00000000: PASSED
   ...
   [00:05:30.000,000] <inf> flash_test: ===========================================
   [00:05:30.000,000] <inf> flash_test: Flash Test Complete
   [00:05:30.000,000] <inf> flash_test: ===========================================
   [00:05:30.000,000] <inf> flash_test: Sectors tested: 4096
   [00:05:30.000,000] <inf> flash_test: Sectors failed: 0
   [00:05:30.000,000] <inf> flash_test: Time elapsed: 330.000 seconds
   [00:05:30.000,000] <inf> flash_test: Result: ALL TESTS PASSED
   [00:05:30.000,000] <inf> flash_test: ===========================================
