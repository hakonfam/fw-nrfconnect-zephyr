/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/cache.h>

int main(void)
{
	int counter = 0;

	printk("Starting Hello World on %s\n", CONFIG_BOARD_TARGET);

	while (true) {
		printk("Hello world from %s - count: %d\n", CONFIG_BOARD_TARGET, counter++);
		// Force cache flush after RTT write
		sys_cache_data_flush_all();
		k_sleep(K_SECONDS(5));
	}

	return 0;
}
