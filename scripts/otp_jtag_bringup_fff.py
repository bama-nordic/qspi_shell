# WARNING - TEST IS DESTRUCIVE ON ASIC - OTP LOCATIONS WILL BE WRITTEN
# WARNING - TEST IS DESTRUCIVE ON ASIC - OTP LOCATIONS WILL BE WRITTEN

# Poll OTP ready flag
myser.poll_otp_ready()

# Set timing registers to 40MHz
myser.write_wrd(0x01B820, 0x0)
myser.write_wrd(0x01B824, 0x30D8B)

# Write 0x50FA50FA to NORDICPROTECT and CUSTOMERPROTECT to enable r/w on all locations
myser.write_otp_nordicprotect_locations(0x50fa50fa)
myser.write_otp_customerprotect_locations(0x50fa50fa)

# Read back PROTECT locations
#myser.read_otp_nordicprotect_locations()
#myser.read_otp_customerprotect_locations()

print("FFF bringup complete. Please perform a hard reset and run otp_jtag_bringup_test.py")