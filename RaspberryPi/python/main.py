import qwiic_vl53l1x
import qwiic
import time
import math
import sys
import argparse
from pythonosc import udp_client

osc_client = 0
last_pitch = 0

def SetupOSC(osc_ip, port):
	global osc_client
	osc_client = udp_client.SimpleUDPClient(osc_ip, port)
	global last_pitch
	last_pitch = 1
	print("OSC Initialized")
	return osc_client

def SendOSC(address_string, val):
	global osc_client
	osc_client.send_message(address_string, val)


def SendPitch(val):
	global last_pitch
	if(val != last_pitch):
		weight = .10;
		val = (val * weight) + (last_pitch * (1-weight))
		SendOSC("/pitch", int(val) )
		print("Smoothed: %s" % int(val) )

		# print("sent /pitch " + str(val) )
		last_pitch = val
	

def main(osc_ip, port):
	osc_client = SetupOSC(osc_ip, port);

	results = qwiic.list_devices()
		
	print(results)

	print("\nSparkFun VL53L1X Example 1\n")
	distance_sensor = qwiic_vl53l1x.QwiicVL53L1X()

	# if distance_sensor.isConnected() == False:
	# 	print("The Qwiic VL53L1X device isn't connected to the system. Please check your connection", file=sys.stderr)
	# 	return
	print("Sensor Created")
	distance_sensor.SensorInit()
	distance_sensor.StartTemperatureUpdate()
	distance_sensor.SetDistanceMode(1)
	print("Sensor Initialized")
	# distance_sensor.StopRanging()

	distance_sensor.SetROI(11, 16);

	roi = distance_sensor.GetROI_XY()
	print("Sensor ROI is: X:", roi[0], " Y:", roi[1]);
	distance_sensor.SetInterMeasurementInMs(33)
	distance_sensor.StartRanging()  # Write configuration bytes to initiate measurement
  
	while True:
		try:
			if (distance_sensor.CheckForDataReady() == True ) and (distance_sensor.GetRangeStatus() == 0 ):
				distance = distance_sensor.GetDistance()	 # Get the result of the measurement from the sensor
				SendPitch(distance)
				print("Distance(mm): %s" % distance)
	
			else:
				time.sleep(.005)
			

		except Exception as e:
			print(e)

	distance_sensor.StopRanging()

if __name__== "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--ip", default="127.0.0.1",
	  help="The ip of the OSC server")
	parser.add_argument("--port", type=int, default=8000,
	  help="The port the OSC server is listening on")
	args = parser.parse_args()
	last_pitch = 0
	
	main(args.ip, args.port)