PSB_SELECT       = 0x0001

PSB_L3           = 0x0002
PSB_R3           = 0x0004

PSB_START        = 0x0008
PSB_PAD_UP       = 0x0010
PSB_PAD_RIGHT    = 0x0020
PSB_PAD_DOWN     = 0x0040
PSB_PAD_LEFT     = 0x0080

PSB_L2           = 0x0100
PSB_R2           = 0x0200
PSB_L1           = 0x0400
PSB_R1           = 0x0800

PSB_GREEN        = 0x1000
PSB_RED          = 0x2000
PSB_BLUE         = 0x4000
PSB_PINK         = 0x8000

PSB_TRIANGLE     = 0x1000
PSB_CIRCLE       = 0x2000
PSB_CROSS        = 0x4000
PSB_SQUARE       = 0x8000

#Guitar button constants
GREEN_FRET	= 0x0200
RED_FRET	= 0x2000
YELLOW_FRET	= 0x1000
BLUE_FRET	= 0x4000
ORANGE_FRET	= 0x8000
STAR_POWER	= 0x0100
UP_STRUM	= 0x0010
DOWN_STRUM	= 0x0040
WHAMMY_BAR	= 0x0008

#These are stick values
PSS_RX  	= 5
PSS_RY  	= 6
PSS_LX 	 	= 7
PSS_LY  	= 8

#These are analog buttons
PSAB_PAD_RIGHT    = 9
PSAB_PAD_UP       = 11
PSAB_PAD_DOWN     = 12
PSAB_PAD_LEFT     = 10

PSAB_L2           = 19
PSAB_R2           = 20
PSAB_L1           = 17
PSAB_R1           = 18

PSAB_GREEN        = 13
PSAB_RED          = 14
PSAB_BLUE         = 15
PSAB_PINK         = 16

PSAB_TRIANGLE     = 13
PSAB_CIRCLE       = 14
PSAB_CROSS        = 15
PSAB_SQUARE       = 16

import math, serial, struct, time
import vjoy


serial_port = "/dev/ttyACM1"
baud_rate = 57600

s = serial.Serial(serial_port, baud_rate, timeout = 1)

buttons = 0
old_buttons = 0
rumble = [0, 0]

data = '                                                   '

def mapVal(x, min, max, outMin, outMax):
	return outMin + (x - min) * (outMax - outMin) / (max - min)

def getVJoyInfo():
	return {
		'name':     'Sony DualShock2',
		'relaxis':  [],
		'absaxis':  [vjoy.ABS_X, vjoy.ABS_Y, vjoy.ABS_THROTTLE, vjoy.ABS_RUDDER, vjoy.ABS_RX, vjoy.ABS_RY, vjoy.ABS_RZ, vjoy.ABS_Z, vjoy.ABS_HAT0X, vjoy.ABS_HAT0Y],
		'feedback': [vjoy.FF_RUMBLE, vjoy.FF_RUMBLE],
		'maxeffects' : 2,
		'buttons':  [vjoy.BTN_A,	vjoy.BTN_B,
		             vjoy.BTN_X,	vjoy.BTN_Y,
					 vjoy.BTN_SELECT, vjoy.BTN_START,
					 vjoy.BTN_THUMBL, vjoy.BTN_THUMBR],
		'enable_ff': True
	}

def try_reconnect():
	global s
	print "attempting to reconnect"
	s = serial.Serial(serial_port, baud_rate, timeout = 1)

def doVJoyThink():
	global buttons, old_buttons, data, s

	time.sleep(0.01)

	try:
		s.write(chr(rumble[0]) + chr(rumble[1]))
		data = s.read(21)
	
	except serial.SerialException:
		try_reconnect()
		return []

	except OSError:
		try_reconnect()
		return []

	events = []

	if len(data) != 21: return events	
	
	buttons = (ord(data[4]) << 8) + ord(data[3])
	
	rx = ord(data[PSS_RX])
	ry = ord(data[PSS_RY])

	lx = ord(data[PSS_LX])
	ly = ord(data[PSS_LY])

	events.append([vjoy.EV_ABS, vjoy.ABS_X, mapVal(lx, 0, 255, -32767, 32766)])
	events.append([vjoy.EV_ABS, vjoy.ABS_Y, mapVal(ly, 255, 0, -32767, 32766)])

	events.append([vjoy.EV_ABS, vjoy.ABS_RUDDER, mapVal(rx, 0, 255, -32767, 32766)])
	events.append([vjoy.EV_ABS, vjoy.ABS_THROTTLE, mapVal(ry, 255, 0, -32767, 32766)])

	r1 = ord(data[PSAB_R1])
	r2 = ord(data[PSAB_R2])

	l1 = ord(data[PSAB_L1])
	l2 = ord(data[PSAB_L2])

	events.append([vjoy.EV_ABS, vjoy.ABS_RX, mapVal(l2, 0, 255, 0, 32766)])
	events.append([vjoy.EV_ABS, vjoy.ABS_RY, mapVal(r2, 0, 255, 0, 32766)])
	events.append([vjoy.EV_ABS, vjoy.ABS_Z, mapVal(l1, 0, 255, 0, 32766)])
	events.append([vjoy.EV_ABS, vjoy.ABS_RZ, mapVal(r1, 0, 255, 0, 32766)])

	dpad_x = ord(data[PSAB_PAD_UP]) - ord(data[PSAB_PAD_DOWN])
	dpad_y = ord(data[PSAB_PAD_RIGHT]) - ord(data[PSAB_PAD_LEFT])

	events.append([vjoy.EV_ABS, vjoy.ABS_HAT0X, mapVal(dpad_x, -255, 255, -32767, 32766)])
	events.append([vjoy.EV_ABS, vjoy.ABS_HAT0Y, mapVal(dpad_y, -255, 255, -32767, 32766)])

	#buttons_on =  ~(buttons & ~old_buttons)
	buttons_on = buttons

	#use analog buttons - they are on if beyond a threshold

	tri = 1 if ord(data[PSAB_TRIANGLE]) > 5 else 0
	sq = 1 if ord(data[PSAB_SQUARE]) > 5 else 0
	cir = 1 if ord(data[PSAB_CIRCLE])> 5 else 0
	x = 1 if ord(data[PSAB_CROSS]) > 5 else 0

	events.append([vjoy.EV_KEY, vjoy.BTN_A, tri])
	events.append([vjoy.EV_KEY, vjoy.BTN_B, sq])
	events.append([vjoy.EV_KEY, vjoy.BTN_X, x])
	events.append([vjoy.EV_KEY, vjoy.BTN_Y, cir])

	events.append([vjoy.EV_KEY, vjoy.BTN_SELECT, (~buttons & PSB_SELECT)])
	events.append([vjoy.EV_KEY, vjoy.BTN_START, (~buttons & PSB_START)])
	events.append([vjoy.EV_KEY, vjoy.BTN_THUMBL, (~buttons & PSB_L3)])
	events.append([vjoy.EV_KEY, vjoy.BTN_THUMBR, (~buttons & PSB_R3)])

   	return events

# Handle force feedback effect uploads.
def doVJoyUploadFeedback(effect):
	global rumble
    	rumble[0] = effect['rumble']['strong_magnitude']
    	rumble[1] = effect['rumble']['weak_magnitude']

# Erase a feedback effect
def doVJoyEraseFeedback(effectid):
   	global rumble
	print effectid
    	rumble[effectid] = 0

# Handle miscellaneous input events.  You probably won't need this.
def doVJoyEvent(evtype, evcode, evvalue):
    pass

