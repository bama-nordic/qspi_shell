import time
import serial

class WiFiUtilsClient():
    """Client class to communicate with DUT via Serial"""

    def __init__(self, port='/dev/ttyACM2', baudrate=115200):
        self.port = port
        self.baudrate = baudrate

    def connect(self) -> str:
        """
        Establish connection with the device
        :return: an error message
        """
        self.ser = serial.Serial(
            port=self.port,
            baudrate=self.baudrate,
            bytesize = serial.EIGHTBITS, #number of bits per bytes
            parity = serial.PARITY_NONE ,#set parity check: no parity
            stopbits = serial.STOPBITS_ONE #number of stop bits
        )

        if self.ser.isOpen() :
          return 'CONNECTED'
        else :
          return 'CONNECTION FAILED!!'

    def close(self) -> None:
        """Close connection with the device"""
        #print('Closing Serial connection')
        self.ser.close()

    def execute_command(self, cmd: str, timeout=10):
        """Send command to the device"""
        #print(cmd)
        cmd = f'{cmd}\n'.encode(encoding='UTF-8')
        try:
            self.ser.write(cmd)
        except Exception:
            raise
        time.sleep(0.1)

    def read(self, size=1) -> str:
        """Read data from the device"""
        #self.ser.reset_input_buffer()
        out = bytes()
        try:
            while True:
                data = self.ser.inWaiting()
                if not data:
                    return out
                out = out + self.ser.read(data)
                time.sleep(1)
        except Exception:
            raise

        return out

    def read_wrd(self,addr):
        self.execute_command(f'wifiutils read_wrd  {addr}')
        rx = self.read().decode().split("\r\n")[-3]
        print(rx)

    def write_wrd(self,addr,data):
        self.execute_command(f'wifiutils write_wrd  {addr} {data}')

    def write_blk(self,addr,pattern,incr,wrd_len):
        self.execute_command(f'wifiutils write_blk  {addr} {pattern} {incr} {wrd_len}')

    def read_blk(self,addr,wrd_len):
        self.execute_command(f'wifiutils read_blk  {addr} {wrd_len}')
        lines = int(wrd_len/4)
        num_lines = lines if ((wrd_len%4) == 0) else lines+1
        rx = self.read().decode().split("\r\n")[-(num_lines+1):-1]
        print("\n".join(rx))

    def wifi_on(self):
        self.execute_command(f'wifiutils wifi_on')
        rx = self.read().decode()
        rx = rx.replace("wifiutils wifi_on","")
        rx = rx.replace("uart:~$","")
        print(rx)

    def wifi_off(self):
        self.execute_command(f'wifiutils wifi_off')
        rx = self.read().decode()
        rx = rx.replace("wifiutils wifi_off","")
        rx = rx.replace("uart:~$","")
        print(rx)

    def memmap(self):
        self.execute_command(f'wifiutils memmap')
        rx = self.read().decode()
        rx = rx.replace("wifiutils memmap","")
        rx = rx.replace("uart:~$","")
        print(rx)

    def help(self):
        self.execute_command(f'wifiutils help')
        rx = self.read().decode()
        rx = rx.replace("wifiutils help","")
        rx = rx.replace("uart:~$","")
        print(rx)

    def memtest(self,addr,pattern,incr,wrd_len):
        self.execute_command(f'wifiutils memtest  {addr} {pattern} {incr} {wrd_len}')
        rx = self.read().decode().split("\r\n")[-2]
        print(rx)

    # Poll the OTP READY register
    def poll_otp_ready(self):
        rx = 0
        poll = 0

        while rx != 4 and poll != 10:
            self.execute_command(f'wifiutils read_wrd  0x01B804')
            rx = int(self.read().decode().split("\r\n")[-3], 16)
            rx &= 4 #mask
            poll += 1

        if poll >= 10:
            raise NameError('Poll OTP READY timed out')

    # Poll the OTP WRDONE register
    def poll_otp_wrdone(self):
        rx = 0
        poll = 0

        while rx != 1 and poll != 10:
            self.execute_command(f'wifiutils read_wrd  0x01B804')
            rx = int(self.read().decode().split("\r\n")[-3], 16)
            rx &= 1 #mask
            poll += 1

        if poll >= 10:
            raise NameError('Poll OTP WRDONE timed out')

    # Poll the OTP RDVALID register
    def poll_otp_rdvalid(self):
        #print("Polling OTP RDVALID")
        rx = 0
        poll = 0

        while rx != 2 and poll != 10:
            self.execute_command(f'wifiutils read_wrd  0x01B804')
            rx = int(self.read().decode().split("\r\n")[-3], 16)
            rx &= 2 #mask
            poll += 1

        if poll >= 10:
            raise NameError('Poll OTP RDVALID timed out')

    # Read an OTP location (no display)
    def read_otp_location(self,addr):
        self.write_wrd(0x01B810, addr)                         # Read address
        self.poll_otp_rdvalid()                                # Check read is valid
        self.execute_command(f'wifiutils read_wrd  0x01B814')  # Read RDDATA register

    # Read an OTP location and display contents
    def read_otp_location_disp(self,addr):
        self.read_otp_location(addr)
        rx = int(self.read().decode().split("\r\n")[-3], 16)   # Get read value from string
        print("OTP location " + "0x{:02x}".format(addr) + " : " + str(hex(rx))) # Read back read register

    # Write to an OTP location
    def write_otp_location(self,addr,data):
        self.write_wrd(0x01B808, addr) # Write address
        self.write_wrd(0x01B80C, data) # Write data
        self.poll_otp_wrdone()         # Poll WRDONE

    # Poll an OTP location
    def poll_otp_location(self,addr,data):
        rx = 0
        poll = 0

        while rx != data and poll != 10:
            self.read_otp_location(addr)
            rx = int(self.read().decode().split("\r\n")[-3], 16)   # Get read value from string
            poll += 1

        if poll >= 10:
            print("Poll failed : OTP location " + "0x{:02x}".format(addr) + " : " + str(hex(rx))) # Read back read register
            raise NameError('Poll OTP location timed out')
        else:
            print("Poll sucessful : OTP location " + "0x{:02x}".format(addr) + " : " + str(hex(rx))) # Read back read register

    def req_otp_standby_mode(self):
        self.write_wrd(0x01B800, 0x0)
        self.poll_otp_ready()
        print("Sucessfully entered OTP standby mode")

    def req_otp_read_mode(self):
        self.write_wrd(0x01B800, 0x1)
        self.poll_otp_ready()
        print("Sucessfully entered OTP Read mode")

    def req_otp_byte_write_mode(self):
        self.write_wrd(0x01B800, 0x42)
        self.poll_otp_ready()
        print("Sucessfully entered OTP byte write mode")

    # Set the IOVDD to 2.5V for OTP writing
    def otp_wr_voltage_2V5(self):
        print("Setting OTP voltage IOVDD to 2.5V")
        self.req_otp_standby_mode()
        #self.execute_command(f'wifiutils read_wrd  0x19004') # read the power reg
        #rx = int(self.read().decode().split("\r\n")[-3], 16)
        #rx &= 15 # modify # 0b0001111
        #rx |= 48 # 0b0110000
        self.execute_command(f'wifiutils write_wrd  0x19004 0x3b') # write modified back

    # Set the IOVDD to 1.8v for OTP reading
    def otp_rd_voltage_1V8(self):
        print("Setting OTP voltage IOVDD to 1.8V")
        self.req_otp_standby_mode()
        #self.execute_command(f'wifiutils read_wrd  0x19004') # read the power reg
        #rx = int(self.read().decode().split("\r\n")[-3], 16)
        #rx &= 15 # modify
        self.execute_command(f'wifiutils write_wrd  0x19004 0xb') # write modified back

    # Read out the NORDICPROTECT OTP locations
    def read_otp_nordicprotect_locations(self):
        print("Reading NORDICPROTECT OTP locations")
        self.otp_rd_voltage_1V8()
        self.req_otp_read_mode()
        self.read_otp_location_disp(0x0)
        self.read_otp_location_disp(0x1)
        self.read_otp_location_disp(0x2)
        self.read_otp_location_disp(0x3)
        self.req_otp_standby_mode()

    # Read out the CUSTOMERPROTECT OTP locations
    def read_otp_customerprotect_locations(self):
        print("Reading CUSTOMERPROTECT OTP locations")
        self.otp_rd_voltage_1V8()
        self.req_otp_read_mode()
        self.read_otp_location_disp(0x40)
        self.read_otp_location_disp(0x41)
        self.read_otp_location_disp(0x42)
        self.read_otp_location_disp(0x43)
        self.req_otp_standby_mode()
    
    # Write to the NORDICPROTECT OTP locations
    def write_otp_nordicprotect_locations(self,data):
        print("Writing NORDICPROTECT OTP locations")
        self.otp_wr_voltage_2V5()
        self.req_otp_byte_write_mode()
        self.write_otp_location(0x0,data)
        self.write_otp_location(0x1,data)
        self.write_otp_location(0x2,data)
        self.write_otp_location(0x3,data)
        self.req_otp_standby_mode()
        self.otp_rd_voltage_1V8()

    # Write to the CUSTOMERPROTECT OTP locations
    def write_otp_customerprotect_locations(self,data):
        print("Writing CUSTOMERPROTECT OTP locations")
        self.otp_wr_voltage_2V5()
        self.req_otp_byte_write_mode()
        self.write_otp_location(0x40,data)
        self.write_otp_location(0x41,data)
        self.write_otp_location(0x42,data)
        self.write_otp_location(0x43,data)
        self.req_otp_standby_mode()
        self.otp_rd_voltage_1V8()

    # Read out the LCS.TEST OTP locations
    def read_otp_lcs_test_locations(self):
        print("Reading LCS.TEST OTP locations")
        self.otp_rd_voltage_1V8()
        self.req_otp_read_mode()
        self.read_otp_location_disp(0x4)
        self.read_otp_location_disp(0x7)
        self.read_otp_location_disp(0x8)
        self.read_otp_location_disp(0xB)
        self.req_otp_standby_mode()
    
    # Write to the LCS.TEST OTP locations
    def write_otp_lcs_test_locations(self,data):
        print("Writing LCS.TEST OTP locations")
        self.otp_wr_voltage_2V5()
        self.req_otp_byte_write_mode()
        self.write_otp_location(0x4,data)
        self.write_otp_location(0x7,data)
        self.write_otp_location(0x8,data)
        self.write_otp_location(0xB,data)
        self.req_otp_standby_mode()
        self.otp_rd_voltage_1V8()

if __name__ == '__main__':
    
    import argparse

    # Argument parser
    parser = argparse.ArgumentParser(
        description = '''
            Used to run wifi tests over QPSI
        '''
    )
    parser.add_argument('-t',
                        '--test',
                        help='Full or relative path to the output directory. Defaults to %(default)s',
                        type=str,
                        required=False,
                        default='otp_jtag_bringup_fff.py',
                        dest='test')
    args = parser.parse_args()

    # Connect to serial port
    myser =  WiFiUtilsClient()
    status = myser.connect()

    myser.wifi_on()

    # Open OTP JTAG bringup script
    exec(open(args.test).read())

    # Close serial port
    myser.close()
