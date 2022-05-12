
# WARNING - TEST IS DESTRUCIVE ON ASIC - OTP LOCATIONS WILL BE WRITTEN
# WARNING - TEST IS DESTRUCIVE ON ASIC - OTP LOCATIONS WILL BE WRITTEN

# Poll OTP ready flag
myser.poll_otp_ready()

# Set timing registers to 40MHz
myser.write_wrd(0x01B820, 0x0)
myser.write_wrd(0x01B824, 0x30D8B)

# Write 0x50fa50fa to LCS.TEST locations to enable TEST life cycle state after hard reset.
myser.write_otp_lcs_test_locations(0x50fa50fa)

# Read back LCS.TEST locations
#myser.read_otp_lcs_test_locations()

print("LCS.Test bringup complete. Please perform a hard reset. After reset, JTAG will be available")