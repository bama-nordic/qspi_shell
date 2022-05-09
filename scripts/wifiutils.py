import time
import serial

class WiFiUtilsClient():
    """Client class to communicate with DUT via Serial"""

    def __init__(self, port='/dev/ttyACM1', baudrate=115200):
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
        time.sleep(2)
        #rx = self.read().decode()
        rx = self.read().decode().split("\r\n")[-2]
        print(rx)



if __name__ == '__main__':

    myser =  WiFiUtilsClient()
    status = myser.connect()

    print("Starting PKTRAM tests")
    myser.memtest(0x0c0000,0,1,50176) #  (PKTRAM)

    #print("Starting GRAM tests")
    #myser.memtest(0x080000,0,1,18432) #  (GRAM)

    #print("Starting LMAC_RET_RAM tests")
    #myser.memtest(0x140000,0,1,12288) #  (LMAC_RET_RAM)

    #print("Starting LMAC_SCR_RAM tests")
    #myser.memtest(0x180000,0,1,16384) #  (LMAC_SCR_RAM)

    #print("Starting UMAC_RET_RAM tests")
    #myser.memtest(0x280000,0,1,36864) #  (UMAC_RET_RAM)

    #print("Starting UMAC_SCR_RAM tests")
    #myser.memtest(0x300000,0,1,57344) #  (UMAC_SCR_RAM)

    myser.close()

