/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing QSPI device specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <stdio.h>
#include <string.h>

#include "qspi_if.h"
#include "spi_if.h"

struct qspi_config config;

#ifdef QSPI_IF
struct qspi_dev qspi = {
	.init = qspi_init,
	.read = qspi_read,
	.write = qspi_write,
	.hl_read = qspi_hl_read,
};
#else
struct qspi_dev spim = {
	.init = spim_init,
	.read = spim_read,
	.write = spim_write,
	.hl_read = spim_hl_read,
};
#endif

struct qspi_config *qspi_defconfig(void)
{

	memset(&config, 0, sizeof(struct qspi_config));

#if defined(QSPI_IF)
	config.addrmode = NRF_QSPI_ADDRMODE_24BIT;
	config.RDC4IO = 0xA0;
	config.easydma = true;
	config.quad_spi = true;
#endif
	config.addrmask = 0x800000; /* set bit23 (incr. addr mode) */

	config.freq = 8; /* 8Mhz */

	config.test_name = "QSPI TEST";
	config.test_hlread = false;
	config.test_iteration = 0;

	config.qspi_slave_latency = 0;

	if (config.freq > 8) /* 16Mhz */
		config.qspi_slave_latency = 1;

	config.encryption = config.CMD_CNONCE = false;

#ifdef QSPI_IF
	/*For #Bit 6 Enable below: i.e ALL Ones for QSPI Key*/
	memset(&config.p_cfg.key, 0xff, sizeof(config.p_cfg.key));

	config.p_cfg.nonce[0] = 0x16181648;
	config.p_cfg.nonce[1] = 0x0;
	config.p_cfg.nonce[2] = 0x1;

#endif /*QSPI_IF*/

	return &config;
}

struct qspi_dev *qspi_dev()
{
#ifdef QSPI_IF
	return &qspi;
#else
	return &spim;
#endif
}
