# qspi_shell
A (Q)SPI shell for Sheliak test and debug

# Commands supported -

uart:~$ wifiutils read_wrd  <address> 

         ex: $ wifiutils read_wrd 0x0c0000
  
uart:~$ wifiutils write_wrd  <address> <data>

         ex: $ wifiutils write_wrd 0x0c0000 0xabcd1234
  
uart:~$ wifiutils read_blk    <address> <num_words>

         ex: $ wifiutils read_blk 0x0c0000 64
         Note - num_words can be a maximum of 2000
  
uart:~$ wifiutils write_blk   <address> <start_pattern> <pattern_increment> <num_words>

         ex: $ wifiutils write_blk 0x0c0000 0xaaaa5555 0 64
         This writes pattern 0xaaaa5555 to 64 locations starting from 0x0c0000


         ex: $ wifiutils write_blk 0x0c0000 0x0 1 64
         This writes pattern 0x0, 0x1,0x2,0x3....etc to 64 locations starting from 0x0c0000
         Note - num_words can be a maximum of 2000
  
uart:~$ wifiutils memtest   <address> <start_pattern> <pattern_increment> <num_words>

         ex: $ wifiutils memtest 0x0c0000 0xaaaa5555 0 64
         This writes pattern 0xaaaa5555 to 64 locations starting from 0x0c0000,
         reads them back and validates them
  
uart:~$ wifiutils wifi_on  

         This writes 1 to BUCKEN (P0.12), waits for 0.5ms and then writes 1 to IOVDD Control (P0.31) 
  
uart:~$ wifiutils wifi_off 

         This writes 0 to IOVDD Control (P0.31) and then writes 0 to BUCKEN Control (P0.12)

uart:~$ wifiutils sleep_stats
         
         This continuously does the RPU sleep/wake cycle and displays stats

uart:~$ wifiutils gpio_config
         
         Configures BUCKEN(P0.12) as o/p, IOVDD control (P0.31) as output and HOST_IRQ (P0.23) as input
         and interruptible with a ISR hooked to it

uart:~$ wifiutils qspi_init
         
         Initializes QSPI driver functions

uart:~$ wifiutils pwron
         
         Sets BUCKEN=1, delay, IOVDD cntrl=1

uart:~$ wifiutils rpuwake
         
         Wakeup RPU: Write 0x1 to WRSR2 register

uart:~$ wifiutils rpuclks_on
         
         Enables all gated RPU clocks. Only SysBUS and PKTRAM will work w/o this setting enabled

uart:~$ wifiutils wrsr2 <val>
         
         writes <val> (0/1) to WRSR2 reg - takes LSByte of <val>

uart:~$ wifiutils rdsr1
         
         Reads RDSR1 Register

uart:~$ wifiutils rdsr2
         
         Reads RDSR2 Register

uart:~$ wifiutils help
         
         Lists all commands with usage example(s)

# Notes -

* All addresses accepted by the utility have to be 32-bit word aligned. Non word-aligned addresses will not be accepted.
* All lengths specified (num_words) are 32-bit word lengths. No support for byte read/write (Where required, the user has to use read-modify-write sequence to implement byte writes)
* All memory blocks (start address + length) used for read/write/memtest etc should be within a specific memory block (PKTRAM, GRAM etc - see 'memmap' command for details). Any address range straddling these blocks will not be accepted.
* write_blk and read_blk commands accept a maximum of 2000 words while memtest command accepts any valid length of words
* Python script provided can be used in the python interactive shell - this can be used in place of a serial port application (TeraTerm/minicom etc). Example usage is provided in the table above as logs)
* The script is provided as-is only as an example. The user is expected to extend it to their usage.
* The nRF700x Hex file will NOT work on nRF53+FPGA setup since there will be a conflict about who drives the BUCKEN (this will be driven from nRF5340 GPIO on nRF700x). Versions for nRF5340+FPGA are added in the table.
