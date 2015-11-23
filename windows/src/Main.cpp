// vJoyClient.cpp : Simple feeder application with a FFB demo
//


// Monitor Force Feedback (FFB) vJoy device
#include "stdafx.h"
//#include "Devioctl.h"
#include "public.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "vjoyinterface.h"
#include "Serial.h"
#include "PS2X.h"
#include "Math.h"

//#define _VERBOSE //verbose debugging
#define PI 3.14159265359
#include <math.h>
#include <string>

// Default device ID (Used when ID not specified)
#define DEV_ID		1

// Prototypes
void  Ffb_PrintRawData(PVOID data);
void  CALLBACK FfbFunction1(PVOID cb, PVOID data);

BOOL PacketType2Str(FFBPType Type, LPTSTR Str);
BOOL EffectType2Str(FFBEType Ctrl, LPTSTR Str);
BOOL DevCtrl2Str(FFB_CTRL Type, LPTSTR Str);
BOOL EffectOpStr(FFBOP Op, LPTSTR Str);
int  Polar2Deg(BYTE Polar);
int  Byte2Percent(BYTE InByte);
int TwosCompByte2Int(BYTE in);
int TwosCompWord2Int(WORD in);

int ffb_direction = 0;
int ffb_strength = 0;


JOYSTICK_POSITION_V2 iReport; // The structure that holds the full position data

float direction(float x, float y) {
	if (x > 0)
		return atan(y / x);
	if (x < 0)
		return atan(y / x) + PI;
	if (y > 0)
		return PI / 2;
	if (y < 0)
		return -PI / 2;
	return 0; // no direction
}

enum ControllerMode
{
	XJOY,
	XJOY_2AXIS, // steering and combined accel/brake (ie nfs)
	DIGITAL,
	ANALOG,
	INVALID
};

int
__cdecl
_tmain(int argc, _TCHAR* argv[])
{
	int stat = 0;
	UINT DevID = DEV_ID;
	USHORT X = 0;
	USHORT Y = 0;
	USHORT Z = 0;
	LONG   Btns = 0;
	

	PVOID pPositionMessage;
	UINT	IoCode = LOAD_POSITIONS;
	UINT	IoSize = sizeof(JOYSTICK_POSITION);
	// HID_DEVICE_ATTRIBUTES attrib;
	BYTE id = 1;
	UINT iInterface = 1;

	// Define the effect names
	static FFBEType FfbEffect= (FFBEType)1;
	LPCTSTR FfbEffectName[] =
	{"NONE", "Constant Force", "Ramp", "Square", "Sine", "Triangle", "Sawtooth Up",\
	"Sawtooth Down", "Spring", "Damper", "Inertia", "Friction", "Custom Force"};

	_tprintf("Initializing vJoyDualShock2 ...\n");

	ControllerMode joyType = INVALID;
	char* portName = "COM4";

	// parsing command line
	
#ifdef _DEBUG
	_tprintf("\ncommand line input debug\n"); 
	for (int i = 1; i < argc; i++)
	{
		_tprintf("%d. %s\n", i, argv[i]);
	}
	_tprintf("\n");
#endif

//#define getNext() std::string((argc > i) ? argv[i+1] : "")

	for (int i = 1; i < argc; i++)
	{
		std::string option(argv[i]);

		if (option == "-mode")
		{
			//std::string value = getNext();
			std::string value(( argc > i ) ? argv[i + 1] : "invalid");

			if (value == "xjoy")
			{
				joyType = XJOY;
				_tprintf("Starting in xjoy mode\n\n");
			}
			else if (value == "xjoy2")
			{
				joyType = XJOY_2AXIS;
				_tprintf("Starting in xjoy (combined axis) mode\n\n");
			}
			else if (value == "digital")
			{
				joyType = DIGITAL;
				_tprintf("Starting in analog mode\n\n");
			}
			else if (value == "analog")
			{
				joyType = ANALOG;
				_tprintf("Starting in digital mode\n\n");
			}
			else if (joyType == INVALID)
			{
				_tprintf("No valid input specified for -mode, using default (xjoy)\n\n");
				joyType = XJOY;
			}
		}
		else if (option == "-id")
		{
			if (argc > i)
			{
				// Set the target Joystick - get it from the command-line 
				DevID = _tstoi(argv[i + 1]);
			}
		}
		else if (argv[i] == "-port")
		{
			if (argc > i){
				// Set the serial port
				portName = argv[i + 1];
			}
		}
	}

	if (joyType == INVALID)
	{
		_tprintf("Starting in xjoy mode\n\n");
		joyType = XJOY;
	}

	// Get the driver attributes (Vendor ID, Product ID, Version Number)
	if (!vJoyEnabled())
	{
		_tprintf("Function vJoyEnabled Failed - make sure that vJoy is installed and enabled\n");
		int dummy = getchar();
		stat = - 2;
		goto Exit;
	}
	else
	{
		wprintf(L"Vendor: %s\nProduct :%s\nVersion Number:%s\n", 
			static_cast<wchar_t *>(GetvJoyManufacturerString()), 
			static_cast<wchar_t *>(GetvJoyProductString()), 
			static_cast<wchar_t *>(GetvJoySerialNumberString()));
	};

	// Get the status of the vJoy device before trying to acquire it
	VjdStat status = GetVJDStatus(DevID);

	switch (status)
	{
	case VJD_STAT_OWN:
		_tprintf("vJoy device %d is already owned by this feeder\n", DevID);
		break;
	case VJD_STAT_FREE:
		_tprintf("vJoy device %d is free\n", DevID);
		break;
	case VJD_STAT_BUSY:
		_tprintf("vJoy device %d is already owned by another feeder\nCannot continue\n", DevID);
		return -3;
	case VJD_STAT_MISS:
		_tprintf("vJoy device %d is not installed or disabled\nCannot continue\n", DevID);
		return -4;
	default:
		_tprintf("vJoy device %d general error\nCannot continue\n", DevID);
		return -1;
	};

	// Acquire the vJoy device
	if (!AcquireVJD(DevID))
	{
		_tprintf("Failed to acquire vJoy device number %d.\n", DevID);
		int dummy = getchar();
		stat = -1;
		goto Exit;
	}
	else
		_tprintf("Acquired device number %d - OK\n", DevID);
		


	// Start FFB  (Not needed from 2.1.6)
#if 0
	BOOL Ffbstarted = FfbStart(DevID);
	if (!Ffbstarted)
	{
		_tprintf("Failed to start FFB on vJoy device number %d.\n", DevID);
		int dummy = getchar();
		stat = -3;
		goto Exit;
	}
	else
		_tprintf("Started FFB on vJoy device number %d - OK\n", DevID);

#endif // 1

	// Register Generic callback function
	// At this point you instruct the Receptor which callback function to call with every FFB packet it receives
	// It is the role of the designer to register the right FFB callback function
	FfbRegisterGenCB(FfbFunction1, NULL);

	Serial* serialPort = new Serial(portName);

	if (serialPort->IsConnected())
	{
		_tprintf("Connected to Port: %s - OK\n", portName);
	}
	else
	{
		_tprintf("Failed to connect to port %s.\n", portName);
		goto Exit;
	}

	char ps2data[21] = "";
	char returnData[2] = "";
	int dataLength = 21;
	int readResult = 0;
	

	// Start endless loop
	// The loop injects position data to the vJoy device
	// If it fails it let's the user try again
	//
	// FFB Note:
	// All FFB activity is performed in a separate thread created when registered the callback function   
	while (1)
	{

		// Set destenition vJoy device
		id = (BYTE)DevID;
		iReport.bDevice = id;

		readResult = serialPort->Available();
		
		//ps2data[3 and 4] : digital buttons
		// 1 PSB_SELECT,	2 PSB_L3,	3 PSB_R3,	4 PSB_START,	5 PSB_PAD_UP,	6 PSB_PAD_RIGHT,	7 PSB_PAD_DOWN,		8 PSB_PAD_LEFT
		// 9 PSB_L2,		10 PSB_R2,	11 PSB_L1,	12 PSB_R1,		13 PSB_TRI,		14 PSB_CIRCLE,		15 PSB_CROSS,		16 PSB_SQUARE
		// 0x01,				0x02,		0x04,		0x08,			0x10,			0x20,				0x40,				0x80

		if (readResult >= 21)
		{
			serialPort->ReadData(ps2data, dataLength);

			if (joyType == XJOY) // XBOX 360
			{

				// Thumb sticks (L -> X/Y and R -> RX/RY)
				iReport.wAxisX = ( (unsigned char) ps2data[PSS_LX] ) << 7;
				iReport.wAxisY = ( (unsigned char) ps2data[PSS_LY] ) << 7;
				iReport.wAxisXRot = ( (unsigned char) ps2data[PSS_RX] ) << 7;
				iReport.wAxisYRot = ( (unsigned char) ps2data[PSS_RY] ) << 7;

				// Triggers (L2 and R2 -> Z and RZ)
				iReport.wAxisZ = ( (unsigned char) ps2data[PSAB_L2] ) << 8;
				iReport.wAxisZRot = ( (unsigned char) ps2data[PSAB_R2] ) << 8;

				///OLD LAYOUT
				/*
				unsigned char lower_buttons = (unsigned char) ~ps2data[3]; // 1-8
				unsigned char upper_buttons = (unsigned char) ~ps2data[4]; // 9-16

				//SELECT, START, L3, R3, L1, R1, TRI, CIRCLE, CROSS, SQUARE
				//unsigned long buttons = ( lower_buttons & 0x01 ) + ( ( lower_buttons & 0x08 ) >> 2 ) + ( ( lower_buttons & 0x06 ) << 1 ) + ( (long) ( upper_buttons & 0xFC ) << 2 );
				*/

				///NEW, actual 360 layout
				unsigned long lower_buttons = (unsigned char) ~ps2data[3]; // 1-8
				unsigned long upper_buttons = (unsigned char) ~ps2data[4]; // 9-16
				unsigned long buttons =
					( ( upper_buttons & 0x40 ) >> 6 ) + //CROSS(7) -> 1 (A)
					( ( upper_buttons & 0x20 ) >> 4 ) + //CIRCLE(6) -> 2 (B)
					( ( upper_buttons & 0x80 ) >> 5 ) +	//SQUARE(8) -> 3 (X)
					( ( upper_buttons & 0x10 ) >> 1 ) + //TRI(5) -> 4 (Y)
					( ( upper_buttons & 0x0C ) << 2 ) + //L1/R1
					( ( lower_buttons & 0x01 ) << 6 ) + //SELECT(1) -> 7
					( ( lower_buttons & 0x08 ) << 4 ) + //START(4) -> 8
					( ( upper_buttons & 0x03 ) << 8 ) +	//L2/R2
					( ( lower_buttons & 0x06 ) << 9 );	//L3/R3

				// D-Pad: 8 directional HAT switch
				signed char haty = ( ( lower_buttons & 0x10 ) > 0 ) ? 1 : 0 - ( ( lower_buttons & 0x40 ) > 0 ) ? -1 : 0;
				signed char hatx = ( ( lower_buttons & 0x20 ) > 0 ) ? 1 : 0 - ( ( lower_buttons & 0x80 ) > 0 ) ? -1 : 0;

				// HAT -1 is neutral, 0 to 35999 where 0 is north, 18000 is south and 9000 and 27000 are east and west respectively
				int hatValue = -1;
				if (hatx != 0 || haty != 0)
				{
					float angle = direction((float) haty, (float) hatx);
					hatValue = ( int(angle * ( 180 / PI )) % 360 ) * 100;
					while (hatValue < 0)
						hatValue += 36000; // normalize angle
				}
#if defined(_DEBUG) & defined(_VERBOSE)
				printf("lByte: %d\t uByte: %d\n", lower_buttons, upper_buttons);
#endif

				//unsigned long buttons = ( (unsigned char) ~ps2data[4] << 8 ) + (unsigned char) ~ps2data[3];
				iReport.lButtons = buttons;
				iReport.bHats = hatValue;
			}
			else if (joyType == XJOY_2AXIS) // DualAnalog Xbox, Need for speed style layout (combined brake/gas w/ steering axes)
			{
				// Thumb sticks Stering and something + combined gas/brake
				iReport.wAxisX = ( (unsigned char) ps2data[PSS_LX] ) << 7;
				iReport.wAxisY = ( (unsigned char) ps2data[PSS_LY] ) << 7;
				iReport.wAxisZ = ( (unsigned char) ps2data[PSS_RX] ) << 7;
				iReport.wAxisZRot = ( (unsigned char) ps2data[PSS_RY] ) << 7;

				unsigned long lower_buttons = (unsigned char) ~ps2data[3]; // 1-8
				unsigned long upper_buttons = (unsigned char) ~ps2data[4]; // 9-16

				// 1 PSB_SELECT,	2 PSB_L3,	3 PSB_R3,	4 PSB_START,	5 PSB_PAD_UP,	6 PSB_PAD_RIGHT,	7 PSB_PAD_DOWN,		8 PSB_PAD_LEFT
				// 9 PSB_L2,		10 PSB_R2,	11 PSB_L1,	12 PSB_R1,		13 PSB_TRI,		14 PSB_CIRCLE,		15 PSB_CROSS,		16 PSB_SQUARE

				// SELECT, START, L3, R3, L1, R1, TRI, CIRCLE, CROSS, SQUARE
				// CROSS, CIRCLE, SQUARE, TRI, L1, R1, SELECT, START, L2, R2, L3, R3
				//	   1,	   2,	   3,	4, 5,	6,		7,		8,	9,10, 11, 12
				unsigned long buttons =
					( ( upper_buttons & 0x40 ) >> 6 ) + //CROSS(7) -> 1 (A)
					( ( upper_buttons & 0x20 ) >> 4 ) + //CIRCLE(6) -> 2 (B)
					( ( upper_buttons & 0x80 ) >> 5 ) +	//SQUARE(8) -> 3 (X)
					( ( upper_buttons & 0x10 ) >> 1 ) + //TRI(5) -> 4 (Y)
					( ( upper_buttons & 0x0C ) << 2 ) + //L1/R1
					( ( lower_buttons & 0x01 ) << 6 ) + //SELECT(1) -> 7
					( ( lower_buttons & 0x08 ) << 4 ) + //START(4) -> 8
					( ( upper_buttons & 0x03 ) << 8 ) +	//L2/R2
					( ( lower_buttons & 0x06 ) << 9 );	//L3/R3

				// some buttons are bitshifted together, here's a list of them:
				//( ( upper_buttons & 0x04 ) << 2 ) + //L1(3) -> 5
				//( ( upper_buttons & 0x08 ) << 2 ) + //R1(4) -> 6
				//( ( upper_buttons & 0x01 ) << 8 ) + //L2(1) -> 9
				//( ( upper_buttons & 0x02 ) << 8 ) + //R2(2) -> 10
				//( ( lower_buttons & 0x02 ) << 9 ) + //L3(2) -> 11
				//( ( lower_buttons & 0x04 ) << 9 );  //R3(3) -> 12
					
				// D-Pad: 8 directional HAT switch
				signed char haty = ( ( lower_buttons & 0x10 ) > 0 ) ? 1 : 0 - ( ( lower_buttons & 0x40 ) > 0 ) ? -1 : 0;
				signed char hatx = ( ( lower_buttons & 0x20 ) > 0 ) ? 1 : 0 - ( ( lower_buttons & 0x80 ) > 0 ) ? -1 : 0;

				// HAT -1 is neutral, 0 to 35999 where 0 is north, 18000 is south and 9000 and 27000 are east and west respectively
				int hatValue = -1;
				if (hatx != 0 || haty != 0)
				{
					float angle = direction((float) haty, (float) hatx);
					hatValue = ( int(angle * ( 180 / PI )) % 360 ) * 100;
					while (hatValue < 0)
						hatValue += 36000; // normalize angle
				}
#if defined(_DEBUG) & defined(_VERBOSE)
				printf("lByte: %d\t uByte: %d\n", lower_buttons, upper_buttons);
#endif

				//unsigned long buttons = ( (unsigned char) ~ps2data[4] << 8 ) + (unsigned char) ~ps2data[3];
				iReport.lButtons = buttons;
				iReport.bHats = hatValue;
			}

#ifdef _DEBUG
			printf("Feeding vJoy buttons: %d\n", iReport.lButtons);
#endif


			// Send position data to vJoy device
			pPositionMessage = (PVOID) ( &iReport );
			if (!UpdateVJD(DevID, pPositionMessage))
			{
				printf("Feeding vJoy device number %d failed - try to enable device then press enter\n", DevID);
				getchar();
				AcquireVJD(DevID);
			}
		}

		serialPort->WriteData(returnData, 2);

		Sleep(2);
	}

Exit:
	RelinquishVJD(DevID);
	return 0;
}


// This function prints the raw FFB packet
void  Ffb_PrintRawData(PVOID data)
{
	FFB_DATA * FfbData = (FFB_DATA *)data;
	int size = FfbData->size;
	_tprintf("\nFFB Size %d\n", size);

	_tprintf("Cmd:%08.8X ", FfbData->cmd);
	_tprintf("ID:%02.2X ", FfbData->data[0]);
	_tprintf("Size:%02.2d ", static_cast<int>(FfbData->size - 8));
	_tprintf(" - ");
	for (UINT i = 0; i < FfbData->size - 8; i++)
		_tprintf(" %02.2X", (UINT)FfbData->data[i]);
	_tprintf("\n");
}


// This is THE callback function used to analyze FFB data packets
// It is registered once in the main thread by calling FfbRegisterGenCB()
// The fanction is called in the thread of the FFB source application for every FFN data packet
void CALLBACK FfbFunction1(PVOID data, PVOID userdata)
{
	// Packet Header
	_tprintf("\n ============= FFB Packet size Size %d =============\n", static_cast<int>(((FFB_DATA *)data)->size));

	/////// Packet Device ID, and Type Block Index (if exists)
#pragma region Packet Device ID, and Type Block Index
	int DeviceID, BlockIndex;
	FFBPType	Type;
	TCHAR	TypeStr[100];

	if (ERROR_SUCCESS == Ffb_h_DeviceID((FFB_DATA *)data, &DeviceID))
		_tprintf("\n > Device ID: %d", DeviceID);
	if (ERROR_SUCCESS == Ffb_h_Type((FFB_DATA *)data, &Type))
	{
		if (!PacketType2Str(Type, TypeStr))
			_tprintf("\n > Packet Type: %d", Type);
		else
			_tprintf("\n > Packet Type: %s", TypeStr);

	}
	if (ERROR_SUCCESS == Ffb_h_EBI((FFB_DATA *)data, &BlockIndex))
		_tprintf("\n > Effect Block Index: %d", BlockIndex);
#pragma endregion


	/////// Effect Report
#pragma region Effect Report
#pragma region Effect Report
#pragma warning( push )
#pragma warning( disable : 4996 )
	FFB_EFF_CONST Effect;
	if (ERROR_SUCCESS == Ffb_h_Eff_Report((FFB_DATA *)data, &Effect))
	{
		if (!EffectType2Str(Effect.EffectType, TypeStr))
			_tprintf("\n >> Effect Report: %02x", Effect.EffectType);
		else
			_tprintf("\n >> Effect Report: %s", TypeStr);
#pragma warning( push )

		if (Effect.Polar)
		{
			_tprintf("\n >> Direction: %d deg (%02x)", Polar2Deg(Effect.Direction), Effect.Direction);


		}
		else
		{
			_tprintf("\n >> X Direction: %02x", Effect.DirX);
			_tprintf("\n >> Y Direction: %02x", Effect.DirY);
		};

		if (Effect.Duration == 0xFFFF)
			_tprintf("\n >> Duration: Infinit");
		else
			_tprintf("\n >> Duration: %d MilliSec", static_cast<int>(Effect.Duration));

		if (Effect.TrigerRpt == 0xFFFF)
			_tprintf("\n >> Trigger Repeat: Infinit");
		else
			_tprintf("\n >> Trigger Repeat: %d", static_cast<int>(Effect.TrigerRpt));

		if (Effect.SamplePrd == 0xFFFF)
			_tprintf("\n >> Sample Period: Infinit");
		else
			_tprintf("\n >> Sample Period: %d", static_cast<int>(Effect.SamplePrd));


		_tprintf("\n >> Gain: %d%%", Byte2Percent(Effect.Gain));

	};
#pragma endregion
#pragma region PID Device Control
	FFB_CTRL	Control;
	TCHAR	CtrlStr[100];
	if (ERROR_SUCCESS == Ffb_h_DevCtrl((FFB_DATA *)data, &Control) && DevCtrl2Str(Control, CtrlStr))
		_tprintf("\n >> PID Device Control: %s", CtrlStr);

#pragma endregion
#pragma region Effect Operation
	FFB_EFF_OP	Operation;
	TCHAR	EffOpStr[100];
	if (ERROR_SUCCESS == Ffb_h_EffOp((FFB_DATA *)data, &Operation) && EffectOpStr(Operation.EffectOp, EffOpStr))
	{
		_tprintf("\n >> Effect Operation: %s", EffOpStr);
		if (Operation.LoopCount == 0xFF)
			_tprintf("\n >> Loop until stopped");
		else
			_tprintf("\n >> Loop %d times", static_cast<int>(Operation.LoopCount));

	};
#pragma endregion
#pragma region Global Device Gain
	BYTE Gain;
	if (ERROR_SUCCESS == Ffb_h_DevGain((FFB_DATA *)data, &Gain))
		_tprintf("\n >> Global Device Gain: %d", Byte2Percent(Gain));

#pragma endregion
#pragma region Condition
	FFB_EFF_COND Condition;
	if (ERROR_SUCCESS == Ffb_h_Eff_Cond((FFB_DATA *)data, &Condition))
	{
		if (Condition.isY)
			_tprintf("\n >> Y Axis");
		else
			_tprintf("\n >> X Axis");
		_tprintf("\n >> Center Point Offset: %d", TwosCompWord2Int((WORD)Condition.CenterPointOffset));
		_tprintf("\n >> Positive Coefficient: %d", TwosCompWord2Int((WORD)Condition.PosCoeff));
		_tprintf("\n >> Negative Coefficient: %d", TwosCompWord2Int((WORD)Condition.NegCoeff));
		_tprintf("\n >> Positive Saturation: %d", Condition.PosSatur);
		_tprintf("\n >> Negative Saturation: %d", Condition.NegSatur);
		_tprintf("\n >> Dead Band: %d", Condition.DeadBand);
	}
#pragma endregion
#pragma region Envelope
	FFB_EFF_ENVLP Envelope;
	if (ERROR_SUCCESS == Ffb_h_Eff_Envlp((FFB_DATA *)data, &Envelope))
	{
		_tprintf("\n >> Attack Level: %d", Envelope.AttackLevel);
		_tprintf("\n >> Fade Level: %d", Envelope.FadeLevel);
		_tprintf("\n >> Attack Time: %d", static_cast<int>(Envelope.AttackTime));
		_tprintf("\n >> Fade Time: %d", static_cast<int>(Envelope.FadeTime));
	};

#pragma endregion
#pragma region Periodic
	FFB_EFF_PERIOD EffPrd;
	if (ERROR_SUCCESS == Ffb_h_Eff_Period((FFB_DATA *)data, &EffPrd))
	{
		_tprintf("\n >> Magnitude: %d", EffPrd.Magnitude);
		_tprintf("\n >> Offset: %d", TwosCompWord2Int(static_cast<WORD>(EffPrd.Offset)));
		_tprintf("\n >> Phase: %d", EffPrd.Phase);
		_tprintf("\n >> Period: %d", static_cast<int>(EffPrd.Period));
	};
#pragma endregion

#pragma region Effect Type
	FFBEType EffectType;
	if (ERROR_SUCCESS == Ffb_h_EffNew((FFB_DATA *)data, &EffectType))
	{
		if (EffectType2Str(EffectType, TypeStr))
			_tprintf("\n >> Effect Type: %s", TypeStr);
		else
			_tprintf("\n >> Effect Type: Unknown");
	}

#pragma endregion

#pragma region Ramp Effect
	FFB_EFF_RAMP RampEffect;
	if (ERROR_SUCCESS == Ffb_h_Eff_Ramp((FFB_DATA *)data, &RampEffect))
	{
		_tprintf("\n >> Ramp Start: %d", TwosCompWord2Int((WORD)RampEffect.Start) /** 10000 / 127*/);
		_tprintf("\n >> Ramp End: %d", TwosCompWord2Int((WORD)RampEffect.End) /** 10000 / 127*/);
	};

#pragma endregion

	_tprintf("\n");
	Ffb_PrintRawData(data);
	_tprintf("\n ====================================================\n");

}


// FFB-related conversion helper functions

// Convert Packet type to String
BOOL PacketType2Str(FFBPType Type, LPTSTR OutStr)
{
	BOOL stat = TRUE;
	LPTSTR Str="";

	switch (Type)
	{
	case PT_EFFREP:
		Str = "Effect Report";
		break;
	case PT_ENVREP:
		Str = "Envelope Report";
		break;
	case PT_CONDREP:
		Str = "Condition Report";
		break;
	case PT_PRIDREP:
		Str = "Periodic Report";
		break;
	case PT_CONSTREP:
		Str = "Constant Force Report";
		break;
	case PT_RAMPREP:
		Str = "Ramp Force Report";
		break;
	case PT_CSTMREP:
		Str = "Custom Force Data Report";
		break;
	case PT_SMPLREP:
		Str = "Download Force Sample";
		break;
	case PT_EFOPREP:
		Str = "Effect Operation Report";
		break;
	case PT_BLKFRREP:
		Str = "PID Block Free Report";
		break;
	case PT_CTRLREP:
		Str = "PID Device Contro";
		break;
	case PT_GAINREP:
		Str = "Device Gain Report";
		break;
	case PT_SETCREP:
		Str = "Set Custom Force Report";
		break;
	case PT_NEWEFREP:
		Str = "Create New Effect Report";
		break;
	case PT_BLKLDREP:
		Str = "Block Load Report";
		break;
	case PT_POOLREP:
		Str = "PID Pool Report";
		break;
	default:
		stat = FALSE;
		break;
	}

	if (stat)
		_tcscpy_s(OutStr, 100, Str);

	return stat;
}

// Convert Effect type to String
BOOL EffectType2Str(FFBEType Type, LPTSTR OutStr)
{
	BOOL stat = TRUE;
	LPTSTR Str="";

	switch (Type)
	{
	case ET_NONE:
		stat = FALSE;
		break;
	case ET_CONST:
		Str="Constant Force";
		break;
	case ET_RAMP:
		Str="Ramp";
		break;
	case ET_SQR:
		Str="Square";
		break;
	case ET_SINE:
		Str="Sine";
		break;
	case ET_TRNGL:
		Str="Triangle";
		break;
	case ET_STUP:
		Str="Sawtooth Up";
		break;
	case ET_STDN:
		Str="Sawtooth Down";
		break;
	case ET_SPRNG:
		Str="Spring";
		break;
	case ET_DMPR:
		Str="Damper";
		break;
	case ET_INRT:
		Str="Inertia";
		break;
	case ET_FRCTN:
		Str="Friction";
		break;
	case ET_CSTM:
		Str="Custom Force";
		break;
	default:
		stat = FALSE;
		break;
	};

	if (stat)
		_tcscpy_s(OutStr, 100, Str);

	return stat;
}

// Convert PID Device Control to String
BOOL DevCtrl2Str(FFB_CTRL Ctrl, LPTSTR OutStr)
{
	BOOL stat = TRUE;
	LPTSTR Str="";

	switch (Ctrl)
	{
	case CTRL_ENACT:
		Str="Enable Actuators";
		break;
	case CTRL_DISACT:
		Str="Disable Actuators";
		break;
	case CTRL_STOPALL:
		Str="Stop All Effects";
		break;
	case CTRL_DEVRST:
		Str="Device Reset";
		break;
	case CTRL_DEVPAUSE:
		Str="Device Pause";
		break;
	case CTRL_DEVCONT:
		Str="Device Continue";
		break;
	default:
		stat = FALSE;
		break;
	}
	if (stat)
		_tcscpy_s(OutStr, 100, Str);

	return stat;
}

// Convert Effect operation to string
BOOL EffectOpStr(FFBOP Op, LPTSTR OutStr)
{
	BOOL stat = TRUE;
	LPTSTR Str="";

	switch (Op)
	{
	case EFF_START:
		Str="Effect Start";
		break;
	case EFF_SOLO:
		Str="Effect Solo Start";
		break;
	case EFF_STOP:
		Str="Effect Stop";
		break;
	default:
		stat = FALSE;
		break;
	}

	if (stat)
		_tcscpy_s(OutStr, 100, Str);

	return stat;
}

// Polar values (0x00-0xFF) to Degrees (0-360)
int Polar2Deg(BYTE Polar)
{
	return ((UINT)Polar*360)/255;
}

// Convert range 0x00-0xFF to 0%-100%
int Byte2Percent(BYTE InByte)
{
	return ((UINT)InByte*100)/255;
}

// Convert One-Byte 2's complement input to integer
int TwosCompByte2Int(BYTE in)
{
	int tmp;
	BYTE inv = ~in;
	BOOL isNeg = in>>7;
	if (isNeg)
	{
		tmp = (int)(inv);
		tmp = -1*tmp;
		return tmp;
	}
	else
		return (int)in;
}

// Convert One-Byte 2's complement input to integer
int TwosCompWord2Int(WORD in)
{
	int tmp;
	WORD inv = ~in;
	BOOL isNeg = in >> 15;
	if (isNeg)
	{
		tmp = (int)(inv);
		tmp = -1 * tmp;
		return tmp - 1;
	}
	else
		return (int)in;
}
