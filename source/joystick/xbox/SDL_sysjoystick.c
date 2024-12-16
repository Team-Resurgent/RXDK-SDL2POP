/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if defined(SDL_JOYSTICK_XBOX)

/* This is the dummy implementation of the SDL joystick API */

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#include "SDL_events.h"

#include <xtl.h>
#include <math.h>
#include <stdio.h>		/* For the definition of NULL */

#define XBINPUT_DEADZONE 0.24f
#define AXIS_MIN	-32768  /* minimum value for axis coordinate */
#define AXIS_MAX	32767   /* maximum value for axis coordinate */
#define MAX_AXES	4		/* each joystick can have up to 4 axes */
#define MAX_BUTTONS	12		/* and 12 buttons                      */
#define	MAX_HATS	2

extern BOOL g_bDevicesInitialized;

typedef struct GamePad
{
	// The following members are inherited from XINPUT_GAMEPAD:
	WORD    wButtons;
	BYTE    bAnalogButtons[8];
	SHORT   sThumbLX;
	SHORT   sThumbLY;
	SHORT   sThumbRX;
	SHORT   sThumbRY;

	// Thumb stick values converted to range [-1,+1]
	FLOAT      fX1;
	FLOAT      fY1;
	FLOAT      fX2;
	FLOAT      fY2;

	// State of buttons tracked since last poll
	WORD       wLastButtons;
	BOOL       bLastAnalogButtons[8];
	WORD       wPressedButtons;
	BOOL       bPressedAnalogButtons[8];

	// Rumble properties
	XINPUT_RUMBLE   Rumble;
	XINPUT_FEEDBACK Feedback;

	// Device properties
	XINPUT_CAPABILITIES caps;
	HANDLE     hDevice;

	// Flags for whether game pad was just inserted or removed
	BOOL       bInserted;
	BOOL       bRemoved;

	// Time to stop the rumble motors
	Uint32     rumble_end_time; // Added field
} XBGAMEPAD;

// Global instance of XInput polling parameters
XINPUT_POLLING_PARAMETERS g_PollingParameters =
{
	TRUE,
	TRUE,
	0,
	8,
	8,
	0,
};

// Global instance of input states
XINPUT_STATE g_InputStates[4];

// Global instance of custom gamepad devices
XBGAMEPAD g_Gamepads[4];

// The private structure used to keep track of a joystick
struct joystick_hwdata
{
	XBGAMEPAD	pGamepad;
	Uint8 index;

	/* values used to translate device-specific coordinates into
	   SDL-standard ranges */
	struct _transaxis {
		int offset;
		float scale;
	} transaxis[6];
};

float (xfabsf)(float x)
{
	if (x == 0)
		return x;
	else
		return (x < 0.0F ? -x : x);
}

VOID XBInput_RefreshDeviceList(XBGAMEPAD* pGamepads, int i)
{
	DWORD dwInsertions, dwRemovals, b;

	XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &dwInsertions, &dwRemovals);

	// Handle removed devices.
	pGamepads->bRemoved = (dwRemovals & (1 << i)) ? TRUE : FALSE;
	if (pGamepads->bRemoved)
	{
		// If the controller was removed after XGetDeviceChanges but before
		// XInputOpen, the device handle will be NULL
		if (pGamepads->hDevice)
			XInputClose(pGamepads->hDevice);
		pGamepads->hDevice = NULL;

		pGamepads->Feedback.Rumble.wLeftMotorSpeed = 0;
		pGamepads->Feedback.Rumble.wRightMotorSpeed = 0;
	}

	// Handle inserted devices
	pGamepads->bInserted = (dwInsertions & (1 << i)) ? TRUE : FALSE;
	if (pGamepads->bInserted)
	{
		// TCR C6-2 Device Types
		pGamepads->hDevice = XInputOpen(XDEVICE_TYPE_GAMEPAD, i,
			XDEVICE_NO_SLOT, &g_PollingParameters);

		// if the controller is removed after XGetDeviceChanges but before
		// XInputOpen, the device handle will be NULL
		if (pGamepads->hDevice)
		{
			XInputGetCapabilities(pGamepads->hDevice, &pGamepads->caps);

			// Initialize last pressed buttons
			XInputGetState(pGamepads->hDevice, &g_InputStates[i]);

			pGamepads->wLastButtons = g_InputStates[i].Gamepad.wButtons;

			for (b = 0; b < 8; b++)
			{
				pGamepads->bLastAnalogButtons[b] =
					// Turn the 8-bit polled value into a boolean value
					(g_InputStates[i].Gamepad.bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK);
			}
		}
	}
}

static int
XBOX_JoystickInit(void)
{
	SDL_Log("Initializing XBOX Joystick driver with Rumble support \n");
	return 0;
}

static int
XBOX_JoystickGetCount(void)
{
	return(4);
}

static void
XBOX_JoystickDetect(void)
{
}

static const char*
XBOX_JoystickGetDeviceName(int device_index)
{
	return("XBOX Gamepad Plugin\n");
}

static int
XBOX_JoystickGetDevicePlayerIndex(int device_index)
{
	return -1;
}

static SDL_JoystickGUID
XBOX_JoystickGetDeviceGUID(int device_index)
{
	SDL_JoystickGUID guid;
	SDL_zero(guid);
	return guid;
}

static SDL_JoystickID
XBOX_JoystickGetDeviceInstanceID(int device_index)
{
	return -1;
}

static int
XBOX_JoystickOpen(SDL_Joystick* joystick, int device_index)
{
	DWORD dwDeviceMask;

	if (!g_bDevicesInitialized) {
		XInitDevices(0, NULL);
		g_bDevicesInitialized = TRUE;
	}

	dwDeviceMask = XGetDevices(XDEVICE_TYPE_GAMEPAD);

	// Allocate joystick hardware data if not already allocated
	if (!joystick->hwdata) {
		joystick->hwdata = (struct joystick_hwdata*)malloc(sizeof(*joystick->hwdata));
		if (!joystick->hwdata) {
			SDL_Log("XBOX_JoystickOpen: Failed to allocate hardware data.\n");
			return -1;
		}
		ZeroMemory(joystick->hwdata, sizeof(*joystick->hwdata));
	}

	// Set joystick properties
	joystick->nbuttons = MAX_BUTTONS;
	joystick->naxes = MAX_AXES;
	joystick->nhats = MAX_HATS;
	joystick->is_game_controller = SDL_TRUE;
	joystick->name = "Xbox SDL Gamepad V0.02";

	// Store index
	joystick->hwdata->index = joystick->instance_id = device_index;

	// Check if the device is already open
	if (g_Gamepads[joystick->hwdata->index].hDevice != NULL) {
		SDL_Log("XBOX_JoystickOpen: Device %d already open. Reinitializing rumble.\n", joystick->hwdata->index);

		// Reuse the existing device handle
		joystick->hwdata->pGamepad.hDevice = g_Gamepads[joystick->hwdata->index].hDevice;

		// Reinitialize rumble motors to zero
		ZeroMemory(&joystick->hwdata->pGamepad.Feedback, sizeof(XINPUT_FEEDBACK));
		joystick->hwdata->pGamepad.Feedback.Header.dwStatus = ERROR_IO_PENDING;
		joystick->hwdata->pGamepad.Feedback.Rumble.wLeftMotorSpeed = 0;
		joystick->hwdata->pGamepad.Feedback.Rumble.wRightMotorSpeed = 0;

		if (XInputSetState(joystick->hwdata->pGamepad.hDevice, &joystick->hwdata->pGamepad.Feedback) != ERROR_IO_PENDING) {
			SDL_Log("XBOX_JoystickOpen: Failed to initialize rumble motors for gamepad %d\n", joystick->hwdata->index);
		}
		else {
			SDL_Log("XBOX_JoystickOpen: Rumble motors initialized successfully for gamepad %d\n", joystick->hwdata->index);
		}

		return 0; // Skip opening the device again
	}

	// Check if device exists
	if (dwDeviceMask & (1 << joystick->hwdata->index)) {
		joystick->hwdata->pGamepad.hDevice = XInputOpen(XDEVICE_TYPE_GAMEPAD, joystick->hwdata->index,
			XDEVICE_NO_SLOT, &g_PollingParameters);

		if (joystick->hwdata->pGamepad.hDevice) {
			SDL_Log("XInputOpen succeeded for device %d\n", joystick->hwdata->index);

			// Save the device handle to the global array
			g_Gamepads[joystick->hwdata->index].hDevice = joystick->hwdata->pGamepad.hDevice;

			// Retrieve capabilities
			if (XInputGetCapabilities(joystick->hwdata->pGamepad.hDevice, &joystick->hwdata->pGamepad.caps) == ERROR_SUCCESS) {
				SDL_Log("XBOX_JoystickOpen: Device capabilities retrieved successfully.\n");
			}
			else {
				SDL_Log("XBOX_JoystickOpen: Failed to retrieve device capabilities.\n");
				XInputClose(joystick->hwdata->pGamepad.hDevice);
				joystick->hwdata->pGamepad.hDevice = NULL;
				return -1;
			}

			// Initialize rumble motors to zero
			ZeroMemory(&joystick->hwdata->pGamepad.Feedback, sizeof(XINPUT_FEEDBACK));
			joystick->hwdata->pGamepad.Feedback.Header.dwStatus = ERROR_IO_PENDING;
			joystick->hwdata->pGamepad.Feedback.Rumble.wLeftMotorSpeed = 0;
			joystick->hwdata->pGamepad.Feedback.Rumble.wRightMotorSpeed = 0;

			if (XInputSetState(joystick->hwdata->pGamepad.hDevice, &joystick->hwdata->pGamepad.Feedback) != ERROR_IO_PENDING) {
				SDL_Log("XBOX_JoystickOpen: Failed to initialize rumble motors for gamepad %d\n", joystick->hwdata->index);
			}
			else {
				SDL_Log("XBOX_JoystickOpen: Rumble motors initialized successfully for gamepad %d\n", joystick->hwdata->index);
			}
		}
		else {
			SDL_Log("XBOX_JoystickOpen: Failed to open gamepad device %d\n", joystick->hwdata->index);
			return -1;
		}
	}
	else {
		SDL_Log("XBOX_JoystickOpen: Gamepad %d not found.\n", joystick->hwdata->index);
		return -1;
	}

	SDL_Log("XBOX_JoystickOpen: Gamepad %d opened successfully.\n", joystick->hwdata->index);
	return 0;
}

static int
XBOX_JoystickRumble(SDL_Joystick* joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms)
{
	if (!joystick || !joystick->hwdata) {
		SDL_Log("XBOX_JoystickRumble: Invalid joystick or hardware data.\n");
		return -1;
	}

	if (!joystick->hwdata->pGamepad.hDevice) {
		SDL_Log("XBOX_JoystickRumble: Device handle is NULL.\n");
		return -1;
	}

	SDL_Log("XBOX_JoystickRumble: Starting rumble...\n");

	// Static/global feedback structure
	static XINPUT_FEEDBACK feedback;
	ZeroMemory(&feedback, sizeof(XINPUT_FEEDBACK));

	// Set rumble motor speeds
	feedback.Rumble.wLeftMotorSpeed = low_frequency_rumble;
	feedback.Rumble.wRightMotorSpeed = high_frequency_rumble;

	// Send the rumble command
	DWORD result = XInputSetState(joystick->hwdata->pGamepad.hDevice, &feedback);
	if (result != ERROR_SUCCESS && result != ERROR_IO_PENDING) {
		SDL_Log("XBOX_JoystickRumble: XInputSetState failed with error %lu.\n", result);
		return -1;
	}

	// Log success
	SDL_Log("XBOX_JoystickRumble: Rumble command sent successfully.\n");

	// Set the time to stop the rumble in joystick->hwdata
	joystick->hwdata->pGamepad.rumble_end_time = SDL_GetTicks() + duration_ms;

	return 0;
}

static void
XBOX_JoystickUpdate(SDL_Joystick* joystick)
{
	static int prev_buttons[4] = { 0 };
	static Sint16 nX = 0, nY = 0;
	static Sint16 nXR = 0, nYR = 0;

	DWORD b = 0;
	FLOAT fX1 = 0;
	FLOAT fY1 = 0;
	FLOAT fX2 = 0;
	FLOAT fY2 = 0;

	int hat = 0, changed = 0;

	// TCR C6-7 Controller Discovery
	// Get status about gamepad insertions and removals.
	XBInput_RefreshDeviceList(&joystick->hwdata->pGamepad, joystick->hwdata->index);

	// If the device isn't valid, bail now
	if (joystick->hwdata->pGamepad.hDevice == NULL) {
		return;
	}

	// Read the input state
	XInputGetState(joystick->hwdata->pGamepad.hDevice, &g_InputStates[joystick->hwdata->index]);

	// Copy gamepad to local structure
	memcpy(&joystick->hwdata->pGamepad, &g_InputStates[joystick->hwdata->index].Gamepad, sizeof(XINPUT_GAMEPAD));

	// Handle Rumble
	Uint32 current_time = SDL_GetTicks();

	// Check if rumble should stop
	if (joystick->hwdata->pGamepad.rumble_end_time > 0 && current_time >= joystick->hwdata->pGamepad.rumble_end_time) {
		SDL_Log("XBOX_JoystickUpdate: Stopping rumble motors...\n");

		static XINPUT_FEEDBACK feedback;
		ZeroMemory(&feedback, sizeof(XINPUT_FEEDBACK));
		feedback.Rumble.wLeftMotorSpeed = 0;
		feedback.Rumble.wRightMotorSpeed = 0;

		XInputSetState(joystick->hwdata->pGamepad.hDevice, &feedback);

		joystick->hwdata->pGamepad.rumble_end_time = 0; // Reset the rumble timer
		SDL_Log("XBOX_JoystickUpdate: Rumble motors stopped.\n");
	}

	// Put Xbox device input for the gamepad into our custom format
	fX1 = (joystick->hwdata->pGamepad.sThumbLX + 0.5f) / 32767.5f;
	joystick->hwdata->pGamepad.fX1 = (fX1 >= 0.0f ? 1.0f : -1.0f) *
		max(0.0f, (xfabsf(fX1) - XBINPUT_DEADZONE) / (1.0f - XBINPUT_DEADZONE));

	fY1 = (joystick->hwdata->pGamepad.sThumbLY + 0.5f) / 32767.5f;
	joystick->hwdata->pGamepad.fY1 = (fY1 >= 0.0f ? 1.0f : -1.0f) *
		max(0.0f, (xfabsf(fY1) - XBINPUT_DEADZONE) / (1.0f - XBINPUT_DEADZONE));

	fX2 = (joystick->hwdata->pGamepad.sThumbRX + 0.5f) / 32767.5f;
	joystick->hwdata->pGamepad.fX2 = (fX2 >= 0.0f ? 1.0f : -1.0f) *
		max(0.0f, (xfabsf(fX2) - XBINPUT_DEADZONE) / (1.0f - XBINPUT_DEADZONE));

	fY2 = (joystick->hwdata->pGamepad.sThumbRY + 0.5f) / 32767.5f;
	joystick->hwdata->pGamepad.fY2 = (fY2 >= 0.0f ? 1.0f : -1.0f) *
		max(0.0f, (xfabsf(fY2) - XBINPUT_DEADZONE) / (1.0f - XBINPUT_DEADZONE));

	// Get the boolean buttons that have been pressed since the last
	// call. Each button is represented by one bit.
	joystick->hwdata->pGamepad.wPressedButtons = (joystick->hwdata->pGamepad.wLastButtons ^ joystick->hwdata->pGamepad.wButtons) & joystick->hwdata->pGamepad.wButtons;
	joystick->hwdata->pGamepad.wLastButtons = joystick->hwdata->pGamepad.wButtons;

	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_START)
	{
		if (!joystick->buttons[8])
			SDL_PrivateJoystickButton(joystick, (Uint8)8, SDL_PRESSED);
	}
	else
	{
		if (joystick->buttons[8])
			SDL_PrivateJoystickButton(joystick, (Uint8)8, SDL_RELEASED);
	}

	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_BACK)
	{
		if (!joystick->buttons[9])
			SDL_PrivateJoystickButton(joystick, (Uint8)9, SDL_PRESSED);
	}
	else
	{
		if (joystick->buttons[9])
			SDL_PrivateJoystickButton(joystick, (Uint8)9, SDL_RELEASED);
	}

	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
	{
		if (!joystick->buttons[10])
			SDL_PrivateJoystickButton(joystick, (Uint8)10, SDL_PRESSED);
	}
	else
	{
		if (joystick->buttons[10])
			SDL_PrivateJoystickButton(joystick, (Uint8)10, SDL_RELEASED);
	}

	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
	{
		if (!joystick->buttons[11])
			SDL_PrivateJoystickButton(joystick, (Uint8)11, SDL_PRESSED);
	}
	else
	{
		if (joystick->buttons[11])
			SDL_PrivateJoystickButton(joystick, (Uint8)11, SDL_RELEASED);
	}

	// Get the analog buttons that have been pressed or released since
	// the last call.
	for (b = 0; b < 8; b++)
	{
		// Turn the 8-bit polled value into a boolean value
		BOOL bPressed = (joystick->hwdata->pGamepad.bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK);

		if (bPressed)
			joystick->hwdata->pGamepad.bAnalogButtons[b] = !joystick->hwdata->pGamepad.bLastAnalogButtons[b];
		else
			joystick->hwdata->pGamepad.bAnalogButtons[b] = FALSE;

		// Store the current state for the next time
		joystick->hwdata->pGamepad.bLastAnalogButtons[b] = bPressed;

		if (bPressed) {
			if (!joystick->buttons[b]) {
				SDL_PrivateJoystickButton(joystick, (Uint8)b, SDL_PRESSED);
			}
		}
		else {
			if (joystick->buttons[b]) {
				SDL_PrivateJoystickButton(joystick, (Uint8)b, SDL_RELEASED);
			}
		}
	}

	// do the HATS baby

	hat = SDL_HAT_CENTERED;
	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
		hat |= SDL_HAT_DOWN;
	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
		hat |= SDL_HAT_UP;
	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
		hat |= SDL_HAT_LEFT;
	if (joystick->hwdata->pGamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
		hat |= SDL_HAT_RIGHT;

	changed = hat ^ prev_buttons[joystick->hwdata->index];

	if (changed) {
		SDL_PrivateJoystickHat(joystick, 0, hat);
	}

	prev_buttons[joystick->hwdata->index] = hat;

	// Axis - LStick

	if ((joystick->hwdata->pGamepad.sThumbLX <= -10000) ||
		(joystick->hwdata->pGamepad.sThumbLX >= 10000))
	{
		if (joystick->hwdata->pGamepad.sThumbLX < 0)
			joystick->hwdata->pGamepad.sThumbLX++;
		nX = ((Sint16)joystick->hwdata->pGamepad.sThumbLX);
	}
	else
		nX = 0;

	if (nX != joystick->axes[0].value)
		SDL_PrivateJoystickAxis(joystick, (Uint8)0, (Sint16)nX);

	if ((joystick->hwdata->pGamepad.sThumbLY <= -10000) ||
		(joystick->hwdata->pGamepad.sThumbLY >= 10000))
	{
		if (joystick->hwdata->pGamepad.sThumbLY < 0)
			joystick->hwdata->pGamepad.sThumbLY++;
		nY = -((Sint16)(joystick->hwdata->pGamepad.sThumbLY));
	}
	else
		nY = 0;

	if (nY != joystick->axes[1].value)
		SDL_PrivateJoystickAxis(joystick, (Uint8)1, (Sint16)nY);

	// Axis - RStick

	if ((joystick->hwdata->pGamepad.sThumbRX <= -10000) ||
		(joystick->hwdata->pGamepad.sThumbRX >= 10000))
	{
		if (joystick->hwdata->pGamepad.sThumbRX < 0)
			joystick->hwdata->pGamepad.sThumbRX++;
		nXR = ((Sint16)joystick->hwdata->pGamepad.sThumbRX);
	}
	else
		nXR = 0;

	if (nXR != joystick->axes[2].value)
		SDL_PrivateJoystickAxis(joystick, (Uint8)2, (Sint16)nXR);

	if ((joystick->hwdata->pGamepad.sThumbRY <= -10000) ||
		(joystick->hwdata->pGamepad.sThumbRY >= 10000))
	{
		if (joystick->hwdata->pGamepad.sThumbRY < 0)
			joystick->hwdata->pGamepad.sThumbRY++;
		nYR = -((Sint16)joystick->hwdata->pGamepad.sThumbRY);
	}
	else
		nYR = 0;

	if (nYR != joystick->axes[3].value)
		SDL_PrivateJoystickAxis(joystick, (Uint8)3, (Sint16)nYR);
}

static void
XBOX_JoystickClose(SDL_Joystick* joystick)
{
	if (joystick->hwdata != NULL) {
		/* free system specific hardware data */
		free(joystick->hwdata);
	}
}

static void
XBOX_JoystickQuit(void)
{
}

SDL_JoystickDriver SDL_XBOX_JoystickDriver =
{
	XBOX_JoystickInit,
	XBOX_JoystickGetCount,
	XBOX_JoystickDetect,
	XBOX_JoystickGetDeviceName,
	XBOX_JoystickGetDevicePlayerIndex,
	XBOX_JoystickGetDeviceGUID,
	XBOX_JoystickGetDeviceInstanceID,
	XBOX_JoystickOpen,
	XBOX_JoystickRumble,
	XBOX_JoystickUpdate,
	XBOX_JoystickClose,
	XBOX_JoystickQuit,
};

#endif /* SDL_JOYSTICK_XBOX */

/* vi: set ts=4 sw=4 expandtab: */