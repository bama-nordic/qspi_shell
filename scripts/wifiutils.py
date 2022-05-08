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
        print('Closing Serial connection')
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
        #rx = self.read().decode()
        print(rx)

    def write_wrd(self,addr,data):
        self.execute_command(f'wifiutils write_wrd  {addr} {data}')

    def write_blk(self,addr,pattern,incr,wrd_len):
        self.execute_command(f'wifiutils write_blk  {addr} {pattern} {incr} {wrd_len}')

    def read_blk(self,addr,wrd_len):
        self.execute_command(f'wifiutils read_blk  {addr} {wrd_len}')
        #rx = self.read().decode().split("\r\n")
        rx = self.read().decode()
        print(rx)

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
        rx = self.read().decode()
        print(rx)



if __name__ == '__main__':

    myser =  WiFiUtilsClient()
    status = myser.connect()
    #myser.write_blk(0x0c0000, 0xaabbccdd, 0, 16)
    #myser.read_blk(0x0c0000, 16)
    myser.memtest(0x0c0000, 0x0, 0x1, 69)

    myser.close()

