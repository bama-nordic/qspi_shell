/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <devicetree.h>
#include <shell/shell.h>

#include "qspi_if.h"
#include "spi_if.h"

#define SHELIAK_SOC     1
#define SLEEP_TIME_MS   2
#define PIN_BUCKEN      12  //P0.12
#define PIN_IOVDD       31  //P0.31

extern struct qspi_config *qspi_config;
const struct qspi_dev *qdev;

const struct device *gpio_dev;
static bool hl_flag;
static int selected_blk;

static uint32_t shk_memmap[][2] = {
    {0x000000, 0x03FFFF}, //0 SysBus
    {0x040000, 0x07FFFF}, //1 PBus
    {0x0C0000, 0x0F0FFF}, //2 PKTRAM
    {0x080000, 0x092000}, //3 GRAM
    {0x100000, 0x134000}, //4 LMAC_ROM
    {0x140000, 0x14C000}, //5 LMAC_RET_RAM
    {0x180000, 0x190000}, //6 LMAC_SCR_RAM
    {0x200000, 0x261800}, //7 UMAC_ROM
    {0x280000, 0x2A4000}, //8 UMAC_RET_RAM
    {0x300000, 0x338000}  //9 UMAC_SCR_RAM
};

void print_memmap()
{
    printk( "                                            \n"); 
    printk( " ===========================================\n");
    printk( "         Sheliak memory map                 \n");
    printk( " ===========================================\n");
    printk( " SysBus         : 0x%06x - 0x%06x (%d words)\n",shk_memmap[0][0], shk_memmap[0][1],(shk_memmap[0][1]-shk_memmap[0][0])>>2);
    printk( " PBus           : 0x%06x - 0x%06x (%d words)\n",shk_memmap[1][0], shk_memmap[1][1],(shk_memmap[1][1]-shk_memmap[1][0])>>2);
    printk( " PKTRAM         : 0x%06x - 0x%06x (%d words)\n",shk_memmap[2][0], shk_memmap[2][1],(shk_memmap[2][1]-shk_memmap[2][0])>>2);
    printk( " GRAM           : 0x%06x - 0x%06x (%d words)\n",shk_memmap[3][0], shk_memmap[3][1],(shk_memmap[3][1]-shk_memmap[3][0])>>2);
    printk( " LMAC_ROM       : 0x%06x - 0x%06x (%d words)\n",shk_memmap[4][0], shk_memmap[4][1],(shk_memmap[4][1]-shk_memmap[4][0])>>2);
    printk( " LMAC_RET_RAM   : 0x%06x - 0x%06x (%d words)\n",shk_memmap[5][0], shk_memmap[5][1],(shk_memmap[5][1]-shk_memmap[5][0])>>2);
    printk( " LMAC_SCR_RAM   : 0x%06x - 0x%06x (%d words)\n",shk_memmap[6][0], shk_memmap[6][1],(shk_memmap[6][1]-shk_memmap[6][0])>>2);
    printk( " UMAC_ROM       : 0x%06x - 0x%06x (%d words)\n",shk_memmap[7][0], shk_memmap[7][1],(shk_memmap[7][1]-shk_memmap[7][0])>>2);
    printk( " UMAC_RET_RAM   : 0x%06x - 0x%06x (%d words)\n",shk_memmap[8][0], shk_memmap[8][1],(shk_memmap[8][1]-shk_memmap[8][0])>>2);
    printk( " UMAC_SCR_RAM   : 0x%06x - 0x%06x (%d words)\n",shk_memmap[9][0], shk_memmap[9][1],(shk_memmap[9][1]-shk_memmap[9][0])>>2);
    printk( "                                            \n"); 
}

static int validate_addr_blk(uint32_t start_addr, uint32_t end_addr,uint32_t block_no) 
{
    if (  ((start_addr >= shk_memmap[block_no][0]) && (start_addr <= shk_memmap[block_no][1])) \
    && ((end_addr >= shk_memmap[block_no][0]) && (end_addr <= shk_memmap[block_no][1])) )
    {
    if ((block_no == 2)) {
        hl_flag = 0;
    }
    selected_blk = block_no; //Save the selected block no
        return 1;
    } else { 
        //printk("Err!!! start_addr = 0x%08x, end_addr = 0x%08x, block = %d\n",start_addr,end_addr,block_no);
    return 0;
    }
} 

static int validate_addr(uint32_t start_addr, uint32_t len)
{
    int ret;
    uint32_t end_addr;
    end_addr = start_addr+len-1;    

    hl_flag = 1;

    ret  = validate_addr_blk(start_addr, end_addr, 0);
    ret |= validate_addr_blk(start_addr, end_addr, 1);
    ret |= validate_addr_blk(start_addr, end_addr, 2);
    ret |= validate_addr_blk(start_addr, end_addr, 3);
    ret |= validate_addr_blk(start_addr, end_addr, 4);
    ret |= validate_addr_blk(start_addr, end_addr, 5);
    ret |= validate_addr_blk(start_addr, end_addr, 6);
    ret |= validate_addr_blk(start_addr, end_addr, 7);
    ret |= validate_addr_blk(start_addr, end_addr, 8);
    ret |= validate_addr_blk(start_addr, end_addr, 9);

    if (!ret) {
        printk("Address validation failed - pls check memmory map and re-try\n");
    print_memmap();
    return 0;
    } else {
    return 1;
    }

}

static int cmd_write_wrd(const struct shell *shell, size_t argc, char **argv)
{
    uint32_t val;
    uint32_t addr;

    addr = strtoul(argv[1], NULL, 0);
    val =  strtoul(argv[2], NULL, 0);

    if (!validate_addr(addr, 1)) return -1;

    if ((selected_blk == 4) || (selected_blk == 7)) {
        shell_print(shell, "Error... Cannot write to ROM blocks");
    }

    qdev->write(addr , &val, 4); //write(addr, &data, len)
    //shell_print(shell, "Written 0x%08x to 0x%08x\n", val, addr);

    return 0;
}


static int cmd_write_blk(const struct shell *shell, size_t argc, char **argv)
{
    // $write_blk 0xc0000 0xdeadbeef 16
    uint32_t pattern;
    uint32_t addr;
    uint32_t num_words;
    uint32_t offset;
    uint32_t *buff;
    int i;
    
    addr = strtoul(argv[1], NULL, 0);
    pattern = strtoul(argv[2], NULL, 0);
    offset = strtoul(argv[3], NULL, 0);
    num_words = strtoul(argv[4], NULL, 0);

    if (num_words > 2000) {
        shell_print(shell, "Presently supporting block read/write only upto 2000 32-bit words");
        return -1;
    }
    
    if (!validate_addr(addr, num_words*4)) return -1;

    if ((selected_blk == 4) || (selected_blk == 7)) {
        shell_print(shell, "Error... Cannot write to ROM blocks");
    }

    buff = (uint32_t *) k_malloc(num_words*4);
    for (i=0; i<num_words; i++) {
        buff[i] = pattern+i*offset;
        //printk("%08x\n", buff[i]);
    }

    qdev->write(addr , buff, num_words*4); //write(addr, &data, len)
    //shell_print(shell, "Written 0x%08x to %d words starting from 0x%08x\n", pattern, num_words, addr);
    
    k_free(buff);

    return 0;
}


static int cmd_read_wrd(const struct shell *shell, size_t argc, char **argv)
{
    uint32_t val;
    uint32_t addr;

    addr = strtoul(argv[1], NULL, 0);
    if (!validate_addr(addr, 1)) return -1;

    //shell_print(shell, "hl_read = %d",(int) hl_flag);
    (hl_flag)? qdev->hl_read(addr, &val, 4) : qdev->read(addr, &val, 4);

    //shell_print(shell, "addr = 0x%08x Read val = 0x%08x\n",addr, val);
    shell_print(shell, "0x%08x\n",val);
    return 0;
}

static int cmd_read_blk(const struct shell *shell, size_t argc, char **argv)
{
    uint32_t *buff;
    uint32_t addr;
    uint32_t num_words;
    uint32_t rem;
    int i;

    // $read_blk 0xc0000 16

    addr = strtoul(argv[1], NULL, 0);
    num_words = strtoul(argv[2], NULL, 0);
    
    if (num_words > 2000) {
        shell_print(shell, "Presently supporting block read/write only upto 2000 32-bit words");
        return -1;
    }

    if (!validate_addr(addr, num_words*4)) return -1;

    buff = (uint32_t *) k_malloc(num_words*4);
    
    //shell_print(shell, "hl_read = %d",(int) hl_flag);
    (hl_flag)? qdev->hl_read(addr, buff, num_words*4) : qdev->read(addr, buff, num_words*4);
    
    for (i=0; i<num_words; i+=4) {
        rem = num_words - i;
        switch (rem) {
            case 1:
                shell_print(shell, "%08x",buff[i]);
                break;
            case 2:
                shell_print(shell, "%08x %08x",buff[i],buff[i+1]);
                break;
            case 3:
                shell_print(shell, "%08x %08x %08x",buff[i],buff[i+1],buff[i+2]);
                break;
            default :
                shell_print(shell, "%08x %08x %08x %08x",buff[i],buff[i+1],buff[i+2],buff[i+3]);
                break;
        }
            
    }
    k_free(buff);

    return 0;
}

static int cmd_memtest(const struct shell *shell, size_t argc, char **argv)
{
    // $write_blk 0xc0000 0xdeadbeef 16
    uint32_t pattern;
    uint32_t addr;
    uint32_t num_words;
    uint32_t offset;
    uint32_t *buff, *rxbuff;
    int i;

    addr = strtoul(argv[1], NULL, 0);
    pattern = strtoul(argv[2], NULL, 0);
    offset = strtoul(argv[3], NULL, 0);
    num_words = strtoul(argv[4], NULL, 0);

#if 0
    if (num_words > 2000) {
        shell_print(shell, "Presently supporting block read/write only upto 2000 32-bit words");
        return -1;
    }
#endif

    if (!validate_addr(addr, num_words*4)) return -1;

    if ((selected_blk == 4) || (selected_blk == 7)) {
        shell_print(shell, "Error... Cannot write to ROM blocks");
    }

    buff = (uint32_t *) k_malloc(2000*4);
    rxbuff = (uint32_t *) k_malloc(2000*4);

    int32_t rem_words = num_words;
    uint32_t test_chunk, chunk_no = 0;

    while (rem_words >0)
    {
        test_chunk = (rem_words < 2000)? rem_words : 2000;

        for (i=0; i<test_chunk; i++) {
            buff[i] = pattern+(i+chunk_no*2000)*offset;
            //printk("%08x\n", buff[i]);
        }

        qdev->write(addr , buff, test_chunk*4); //write(addr, &data, len)
        //shell_print(shell, "Written 0x%08x to %d words starting from 0x%08x\n", pattern, num_words, addr);

        (hl_flag)? qdev->hl_read(addr, rxbuff, test_chunk*4) : qdev->read(addr, rxbuff, test_chunk*4);

        if (memcmp(buff,rxbuff,test_chunk*4) != 0) {
            shell_print(shell, "memtest failed");
            k_free(rxbuff);
            k_free(buff);
            return -1;
        }
        rem_words -= 2000;
        chunk_no ++;
    }
    shell_print(shell, "memtest PASSED");
    k_free(rxbuff);
    k_free(buff);

    return 0;
}


static int cmd_wifi_on(const struct shell *shell, size_t argc, char **argv)
{

#if SHELIAK_SOC 
    struct qspi_config *cfg;
    uint32_t rpu_clks = 0x100;

    cfg = qspi_defconfig();
    qdev = qspi_dev(); // QSPI

    gpio_pin_set(gpio_dev, PIN_BUCKEN, 1); // BUCKEN = 1
    k_msleep(SLEEP_TIME_MS);
    gpio_pin_set(gpio_dev, PIN_IOVDD, 1); // IOVDD CNTRL = 1

    shell_print(shell, "Enabled BUCKEN...");
    shell_print(shell, "Enabled IOVDD...");

    qdev->init(cfg);

    // Enable RPU Clocks
    qdev->write(0x048C20 , &rpu_clks, 4); //write(addr, &data, len)
#else
    shell_print(shell, "Wi-Fi ON is not implemented for FPGA");
#endif

    return 0;
}

static int cmd_wifi_off(const struct shell *shell, size_t argc, char **argv)
{

#if SHELIAK_SOC
    gpio_pin_set(gpio_dev, PIN_IOVDD, 0); // IOVDD CNTRL = 0
    gpio_pin_set(gpio_dev, PIN_BUCKEN, 0); // BUCKEN = 0
    shell_print(shell, "Disabled IOVDD...");
    shell_print(shell, "Disabled BUCKEN...");
#else
    shell_print(shell, "Wi-Fi OFF is not implemented for FPGA");
#endif

    return 0;
}

static int cmd_memmap(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "                                            "); 
    shell_print(shell, " ===========================================");
    shell_print(shell, "         Sheliak memory map                 ");
    shell_print(shell, " ===========================================");
    shell_print(shell, " SysBus         : 0x%06x - 0x%06x (%d words)",shk_memmap[0][0], shk_memmap[0][1],(1+(shk_memmap[0][1]-shk_memmap[0][0]))>>2);
    shell_print(shell, " PBus           : 0x%06x - 0x%06x (%d words)",shk_memmap[1][0], shk_memmap[1][1],(1+(shk_memmap[1][1]-shk_memmap[1][0]))>>2);
    shell_print(shell, " PKTRAM         : 0x%06x - 0x%06x (%d words)",shk_memmap[2][0], shk_memmap[2][1],(1+(shk_memmap[2][1]-shk_memmap[2][0]))>>2);
    shell_print(shell, " GRAM           : 0x%06x - 0x%06x (%d words)",shk_memmap[3][0], shk_memmap[3][1],(1+(shk_memmap[3][1]-shk_memmap[3][0]))>>2);
    shell_print(shell, " LMAC_ROM       : 0x%06x - 0x%06x (%d words)",shk_memmap[4][0], shk_memmap[4][1],(1+(shk_memmap[4][1]-shk_memmap[4][0]))>>2);
    shell_print(shell, " LMAC_RET_RAM   : 0x%06x - 0x%06x (%d words)",shk_memmap[5][0], shk_memmap[5][1],(1+(shk_memmap[5][1]-shk_memmap[5][0]))>>2);
    shell_print(shell, " LMAC_SCR_RAM   : 0x%06x - 0x%06x (%d words)",shk_memmap[6][0], shk_memmap[6][1],(1+(shk_memmap[6][1]-shk_memmap[6][0]))>>2);
    shell_print(shell, " UMAC_ROM       : 0x%06x - 0x%06x (%d words)",shk_memmap[7][0], shk_memmap[7][1],(1+(shk_memmap[7][1]-shk_memmap[7][0]))>>2);
    shell_print(shell, " UMAC_RET_RAM   : 0x%06x - 0x%06x (%d words)",shk_memmap[8][0], shk_memmap[8][1],(1+(shk_memmap[8][1]-shk_memmap[8][0]))>>2);
    shell_print(shell, " UMAC_SCR_RAM   : 0x%06x - 0x%06x (%d words)",shk_memmap[9][0], shk_memmap[9][1],(1+(shk_memmap[9][1]-shk_memmap[9][0]))>>2);
    shell_print(shell, "                                            "); 
    return 0; 
}

static void cmd_help(const struct shell *shell, size_t argc, char **argv)
{
     
    shell_print(shell, "Supported commands....  "); 
    shell_print(shell, "=========================  "); 
    shell_print(shell, "uart:~$ wifiutils read_wrd    <address> "); 
    shell_print(shell, "         ex: $ wifiutils read_wrd 0x0c0000"); 
    shell_print(shell, "  "); 
    shell_print(shell, "uart:~$ wifiutils write_wrd   <address> <data>"); 
    shell_print(shell, "         ex: $ wifiutils write_wrd 0x0c0000 0xabcd1234"); 
    shell_print(shell, "  "); 
    shell_print(shell, "uart:~$ wifiutils read_blk    <address> <num_words>"); 
    shell_print(shell, "         ex: $ wifiutils read_blk 0x0c0000 64"); 
    shell_print(shell, "         Note - num_words can be a maximum of 2000"); 
    shell_print(shell, "  "); 
    shell_print(shell, "uart:~$ wifiutils write_blk   <address> <start_pattern> <pattern_increment> <num_words>"); 
    shell_print(shell, "         ex: $ wifiutils write_blk 0x0c0000 0xaaaa5555 0 64"); 
    shell_print(shell, "         This writes pattern 0xaaaa5555 to 64 locations starting from 0x0c0000"); 
    shell_print(shell, "         ex: $ wifiutils write_blk 0x0c0000 0x0 1 64"); 
    shell_print(shell, "         This writes pattern 0x0, 0x1,0x2,0x3....etc to 64 locations starting from 0x0c0000"); 
    shell_print(shell, "         Note - num_words can be a maximum of 2000"); 
    shell_print(shell, "  "); 
    shell_print(shell, "uart:~$ wifiutils memtest   <address> <start_pattern> <pattern_increment> <num_words>"); 
    shell_print(shell, "         ex: $ wifiutils memtest 0x0c0000 0xaaaa5555 0 64"); 
    shell_print(shell, "         This writes pattern 0xaaaa5555 to 64 locations starting from 0x0c0000,"); 
    shell_print(shell, "         reads them back and validates them"); 
    shell_print(shell, "  "); 
    shell_print(shell, "uart:~$ wifiutils wifi_on  ");
    shell_print(shell, "         This writes 1 to BUCKEN (P0.12), waits for 0.5ms and then writes 1 to IOVDD Control (P0.31) "); 
    shell_print(shell, "  "); 
    shell_print(shell, "uart:~$ wifiutils wifi_off ");
    shell_print(shell, "         This writes 0 to IOVDD Control (P0.31) and then writes 0 to BUCKEN Control (P0.12)"); 
    shell_print(shell, "  "); 
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_wifiutils,
        SHELL_CMD(write_blk, NULL, "Writes a block of words to Sheliak host memory via QSPI interface",
                                               cmd_write_blk),
        SHELL_CMD(read_blk,   NULL, "Reads a block of words from Sheliak host memory via QSPI interface", cmd_read_blk),
        SHELL_CMD(write_wrd, NULL, "Writes a word to Sheliak host memory via QSPI interface",
                                               cmd_write_wrd),
        SHELL_CMD(read_wrd,   NULL, "Reads a word from Sheliak host memory via QSPI interface", cmd_read_wrd),
        SHELL_CMD(wifi_on,   NULL, "BUCKEN-IOVDD power ON", cmd_wifi_on),
        SHELL_CMD(wifi_off,   NULL, "BUCKEN-IOVDD power OFF", cmd_wifi_off),
        SHELL_CMD(memmap,   NULL, "Gives the full memory map of the Sheliak chip", cmd_memmap),
        SHELL_CMD(memtest,  NULL, "Writes, reads back and validates specified memory on Seliak chip", cmd_memtest),
        SHELL_CMD(help,   NULL, "Help with all supported commmands", cmd_help),
        SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "wifiutils" */
SHELL_CMD_REGISTER(wifiutils, &sub_wifiutils, "wifiutils commands", NULL);


void main(void)
{
    struct qspi_config *cfg;
    uint32_t rpu_clks = 0x100;

#if SHELIAK_SOC
    int ret;

    gpio_dev = device_get_binding("GPIO_0");
    if (gpio_dev == NULL) {
        return;
    }

    ret = gpio_pin_configure(gpio_dev, PIN_BUCKEN, GPIO_OUTPUT);
    if (ret < 0) {
        return;
    }

    ret = gpio_pin_configure(gpio_dev, PIN_IOVDD, GPIO_OUTPUT);
    if (ret < 0) {
        return;
    }

#if 1
    printk("GPIO configuration done...\n\n");

    gpio_pin_set(gpio_dev, PIN_BUCKEN, 1); // Assert BUCKEN
    k_msleep(SLEEP_TIME_MS);
    printk("\nBUCKEN Asserted...\n");
    gpio_pin_set(gpio_dev, PIN_IOVDD, 1); // IOVDD CNTRL = 1

    printk("Enabled IOVDD...\n\n");

    printk("Check voltage on PWRIOVDD on Shelaik chip to confirm Sheliak is powered ON\n\n");
#endif

#endif

#if 1
    cfg = qspi_defconfig();

    qdev = qspi_dev(); // QSPI

    qdev->init(cfg);

    // Enable RPU Clocks
    qdev->write(0x048C20 , &rpu_clks, 4); //write(addr, &data, len)

#endif

} /* main() */
