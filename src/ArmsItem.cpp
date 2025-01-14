// THIS IS DECOMPILED PROPRIETARY CODE - USE AT YOUR OWN RISK.
//
// The original code belongs to Daisuke "Pixel" Amaya.
//
// Modifications and custom code are under the MIT licence.
// See LICENCE.txt for details.

#include "ArmsItem.h"

#include <string.h>
#include <string>

#include "WindowsWrapper.h"

#include "CommonDefines.h"
#include "Draw.h"
#include "Escape.h"
#include "Game.h"
#include "KeyControl.h"
#include "Main.h"
#include "Pause.h"
#include "Shoot.h"
#include "Sound.h"
#include "TextScr.h"
#include "MyChar.h"

ARMS gArmsData[ARMS_MAX];
ITEM gItemData[ITEM_MAX];

int gSelectedArms;
int gSelectedItem;

int gCampTitleY;

/// True if we're in the items section of the inventory (not in the weapons section) (only relevant when the inventory is open)
BOOL gCampActive;

int gArmsEnergyX = 16;

void ClearArmsData(void)
{
#ifdef FIX_BUGS
	gSelectedArms = 0; // Should probably be done in order to avoid potential problems with the selected weapon being invalid (like is done in SubArmsData)
#endif
	gArmsEnergyX = 32;
	memset(gArmsData, 0, sizeof(gArmsData));
}

void ClearItemData(void)
{
	memset(gItemData, 0, sizeof(gItemData));
}

BOOL AddArmsData(long code, long max_num)
{
	// Search for code
	int i = 0;
	while (i < ARMS_MAX)
	{
		if (gArmsData[i].code == code)
			break;	// Found identical

		if (gArmsData[i].code == 0)
			break;	// Found free slot

		++i;
	}

	if (i == ARMS_MAX)
		return FALSE;	// No space left

	if (gArmsData[i].code == 0)
	{
		// Initialize new weapon
		memset(&gArmsData[i], 0, sizeof(ARMS));
		gArmsData[i].level = 1;
	}

	// Set weapon and ammo
	gArmsData[i].code = code;
	if (gArmsData[i].max_num < 9999)
		gArmsData[i].max_num += max_num;
	gArmsData[i].num += max_num;

	// Cap the amount of current ammo to the maximum amount of ammo
	if (gArmsData[i].num > gArmsData[i].max_num)
		gArmsData[i].num = gArmsData[i].max_num;

	return TRUE;
}

BOOL SubArmsData(long code)
{
	// Search for code
	int i;
	for (i = 0; i < ARMS_MAX; ++i)
		if (gArmsData[i].code == code)
			break;	// Found

#ifdef FIX_BUGS
	if (i == ARMS_MAX)
#else
	if (i == ITEM_MAX)	// Wrong
#endif
		return FALSE;	// Not found

	// Shift all arms from the right to the left
	for (++i; i < ARMS_MAX; ++i)
		gArmsData[i - 1] = gArmsData[i];

	// Clear farthest weapon and select first
	gArmsData[i - 1].code = 0;
	gSelectedArms = 0;

	return TRUE;
}

BOOL TradeArms(long code1, long code2, long max_num)
{
	// Search for code1
	int i = 0;
	while (i < ARMS_MAX)
	{
		if (gArmsData[i].code == code1)
			break;	// Found identical

		++i;
	}

	if (i == ARMS_MAX)
		return FALSE;	// Not found

	// Initialize new weapon replacing old one, but adding the maximum ammunition to that of the old weapon.
	gArmsData[i].level = 1;
	gArmsData[i].code = code2;
	gArmsData[i].max_num += max_num;
	gArmsData[i].num += max_num;
	gArmsData[i].exp = 0;

	return TRUE;
}

BOOL AddItemData(long code)
{
	// Search for code
	int i = 0;
	while (i < ITEM_MAX)
	{
		if (gItemData[i].code == code)
			break;	// Found identical

		if (gItemData[i].code == 0)
			break;	// Found free slot

		++i;
	}

	if (i == ITEM_MAX)
		return FALSE;	// Not found

	gItemData[i].code = code;

	return TRUE;
}

BOOL SubItemData(long code)
{
	// Search for code
	int i;
	for (i = 0; i < ITEM_MAX; ++i)
		if (gItemData[i].code == code)
			break;	// Found

	if (i == ITEM_MAX)
		return FALSE;	// Not found

	// Shift all items from the right to the left
	for (++i; i < ITEM_MAX; ++i)
		gItemData[i - 1] = gItemData[i];

	gItemData[i - 1].code = 0;
	gSelectedItem = 0;

	return TRUE;
}

/// Update the inventory cursor
void MoveCampCursor(void)
{
	BOOL bChange;

	// Compute the current amount of weapons and items
	int arms_num = 0;
	int item_num = 0;
	while (gArmsData[arms_num].code != 0)
		++arms_num;
	while (gItemData[item_num].code != 0)
		++item_num;

	if (arms_num == 0 && item_num == 0)
		return;	// Empty inventory

	// True if we're currently changing cursor position
	bChange = FALSE;

	if (!gCampActive)
	{
		// Handle selected weapon
		if (gKeyTrg & gKeyLeft)
		{
			--gSelectedArms;
			bChange = TRUE;
		}

		if (gKeyTrg & gKeyRight)
		{
			++gSelectedArms;
			bChange = TRUE;
		}

		if (gKeyTrg & (gKeyUp | gKeyDown))
		{
			// If there are any items, we're changing to the items section, since the weapons section has only 1 row
			if (item_num != 0)
				gCampActive = TRUE;

			bChange = TRUE;
		}

		// Loop around gSelectedArms if needed
		if (gSelectedArms < 0)
			gSelectedArms = arms_num - 1;

		if (gSelectedArms > arms_num - 1)
			gSelectedArms = 0;
	}
	else
	{
		// Handle selected item
		if (gKeyTrg & gKeyLeft)
		{
			if (gSelectedItem % 6 == 0)
				gSelectedItem += 5;
			else
				gSelectedItem -= 1;

			bChange = TRUE;
		}

		if (gKeyTrg & gKeyRight)
		{
			if (gSelectedItem == item_num - 1)
				gSelectedItem = (gSelectedItem / 6) * 6;	// Round down to multiple of 6
			else if (gSelectedItem % 6 == 5)
				gSelectedItem -= 5;	// Loop around row
			else
				gSelectedItem += 1;

			bChange = TRUE;
		}

		if (gKeyTrg & gKeyUp)
		{
			if (gSelectedItem / 6 == 0)
				gCampActive = FALSE;	// We're on the first row, transition to weapons
			else
				gSelectedItem -= 6;

			bChange = TRUE;
		}

		if (gKeyTrg & gKeyDown)
		{
			if (gSelectedItem / 6 == (item_num - 1) / 6)
				gCampActive = FALSE;	// We're on the last row, transition to weapons
			else
				gSelectedItem += 6;

			bChange = TRUE;
		}

		if (gSelectedItem >= item_num)
			gSelectedItem = item_num - 1;	// Don't allow selecting a non-existing item

		if (gCampActive && gKeyTrg & gKeyOk)
			StartTextScript(6000 + gItemData[gSelectedItem].code);
	}

	if (bChange)
	{
		if (gCampActive == FALSE)
		{
			// Switch to a weapon
			PlaySoundObject(SND_SWITCH_WEAPON, SOUND_MODE_PLAY);

			if (arms_num != 0)
				StartTextScript(1000 + gArmsData[gSelectedArms].code);
			else
				StartTextScript(1000);
		}
		else
		{
			// Switch to an item
			PlaySoundObject(SND_YES_NO_CHANGE_CHOICE, SOUND_MODE_PLAY);

			if (item_num != 0)
				StartTextScript(5000 + gItemData[gSelectedItem].code);
			else
				StartTextScript(5000);
		}
	}
}

/// Draw the inventory
void PutCampObject(void)
{
	static unsigned int flash;

	int i;
	int j;
	int k;
	// I hate this with every bit of my soul
	// But hey it works

	/// Rect for the current weapon
	RECT rcArms;

	/// Rect for the current item
	RECT rcItem;

	/// Probably the rect for the slash
	RECT rcPer = {72, 48, 80, 56};

	/// Rect for when there is no ammo (double dashes)
	RECT rcNone = {80, 48, 96, 56};

	/// Rect for the "Lv" text!
	RECT rcLv = {80, 80, 96, 88};

	/// Final rect drawn on the screen
	RECT rcView = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

	/// Cursor rect
	RECT rcCur = {112, 88, 128, 104};

	RECT rcTitle1 = {80, 48, 144, 56};
	RECT rcTitle2 = {80, 56, 144, 64};
	RECT rcBoxTop = {0, 0, 244, 8};
	RECT rcBoxBody = {0, 8, 244, 16};
	RECT rcBoxBottom = {0, 16, 244, 24};

	RECT rcBoxLeft = {0, 0, 48, 24};
	RECT rcBoxRight = {232, 0, 244, 24};

	RECT rcBoxCharBox = {0, 88, 48, 128};
	RECT rcMyChar = {0, 16, 16, 32};
	RECT rcCion = {128, 112, 152, 120};

	// Draw item box
	PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 112 + 78), &rcBoxTop, SURFACE_ID_TEXT_BOX);
	for (i = 1; i < 8; ++i)
		PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord(((WINDOW_HEIGHT / 2) - 120) + ((i + 1) * 8) + 78), &rcBoxBody, SURFACE_ID_TEXT_BOX);
	PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord(((WINDOW_HEIGHT / 2) - 120) + ((i + 1) * 8) + 78), &rcBoxBottom, SURFACE_ID_TEXT_BOX);

	// Draw weapon box
	PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 112 + 54), &rcBoxTop, SURFACE_ID_TEXT_BOX);
	for (j = 1; j < 2 ; ++j)
		PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord(((WINDOW_HEIGHT / 2) - 120) + ((j + 1) * 8) + 54), &rcBoxBody, SURFACE_ID_TEXT_BOX);
	PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord(((WINDOW_HEIGHT / 2) - 120) + ((j + 1) * 8) + 54), &rcBoxBottom, SURFACE_ID_TEXT_BOX);

	// Draw MyChar box
	//PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 112 + 8), &rcBoxCharBox, SURFACE_ID_TEXT_BOX);
	PutBitmap3(&rcView, PixelToScreenCoord(38), PixelToScreenCoord(16), &rcBoxCharBox, SURFACE_ID_TEXT_BOX);

	// Draw stats boxes
	PutBitmap3(&rcView, PixelToScreenCoord(86), PixelToScreenCoord(8), &rcBoxLeft, SURFACE_ID_TEXT_BOX);
	PutBitmap3(&rcView, PixelToScreenCoord(134), PixelToScreenCoord(8), &rcBoxRight, SURFACE_ID_TEXT_BOX);

	PutBitmap3(&rcView, PixelToScreenCoord(86), PixelToScreenCoord(32), &rcBoxLeft, SURFACE_ID_TEXT_BOX);
	PutBitmap3(&rcView, PixelToScreenCoord(134), PixelToScreenCoord(32), &rcBoxRight, SURFACE_ID_TEXT_BOX);

	// Draw stats
	PutBitmap3(&grcGame, PixelToScreenCoord(120), PixelToScreenCoord(15), &rcCion, SURFACE_ID_TEXT_BOX);
	PutNumber4(88, 15, cion, FALSE);
	
	RECT rcLife[2] = {
		{120, 112, 128, 120},
		{112, 112, 120, 120},
	};
	for (int i = 0; i < gMC.max_life - 1; i++) // For every 1 in max life, add 1 to i, and run the following code
	{
		// Put a heart
		PutBitmap3(
			&grcGame, // Target
			// On the next two lines TT7 forgot PixelToScreenCoord, which is needed for the PutBitmap to work on resolutions other than 1x 
			PixelToScreenCoord(96 + (8 * i)), // X position, offset by 8 for every 1 in i
			PixelToScreenCoord(40), // Y position,
			& rcLife[gMC.life - 1 > i], // Which rect to use, 'gMC.life - 1 > i' checks if  the current heart that is being drawn is full or not, returns 0 if it's not and 1 if it is
			SURFACE_ID_TEXT_BOX); // Surface
	}

	// Move titles
	/*if (gCampTitleY > (WINDOW_HEIGHT / 2) - 104)
		--gCampTitleY;

	// Draw titles
	PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 112), PixelToScreenCoord(gCampTitleY + 32), &rcTitle1, SURFACE_ID_TEXT_BOX);
	PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 112), PixelToScreenCoord(gCampTitleY + 76), &rcTitle2, SURFACE_ID_TEXT_BOX);*/

	++flash;

	// Draw weapons
	for (i = 0; i < ARMS_MAX; ++i)
	{
		if (gArmsData[i].code == 0)
			break;	// Invalid weapon

		// Get icon rect for next weapon
		rcArms.left = (gArmsData[i].code % 32) * 32;
		rcArms.right = rcArms.left + 32;
		rcArms.top = ((gArmsData[i].code) / 32) * 32;
		rcArms.bottom = rcArms.top + 16;

		// Draw the icon, slash and "Lv"
		PutBitmap3(&rcView, PixelToScreenCoord((i * 40) + (WINDOW_WIDTH / 2) - 112), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 96 + 40), &rcArms, SURFACE_ID_ARMS_IMAGE);
		//PutBitmap3(&rcView, PixelToScreenCoord((i * 40) + (WINDOW_WIDTH / 2) - 112), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 64), &rcPer, SURFACE_ID_TEXT_BOX);
		//PutBitmap3(&rcView, PixelToScreenCoord((i * 40) + (WINDOW_WIDTH / 2) - 112), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 80), &rcLv, SURFACE_ID_TEXT_BOX);
		//PutNumber4((i * 40) + (WINDOW_WIDTH / 2) - 112, (WINDOW_HEIGHT / 2) - 80, gArmsData[i].level, FALSE);

		// Draw ammo
		if (gArmsData[i].max_num)
		{
			PutNumber4((i * 40) + (WINDOW_WIDTH / 2) - 112, (WINDOW_HEIGHT / 2) - 88 + 40, gArmsData[i].num, FALSE);
			//PutNumber4((i * 40) + (WINDOW_WIDTH / 2) - 112, (WINDOW_HEIGHT / 2) - 64, gArmsData[i].max_num, FALSE);
		}
		else
		{
			// Weapon doesn't use ammunition
			PutBitmap3(&rcView, PixelToScreenCoord((i * 40) + (WINDOW_WIDTH - 192) / 2), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 72), &rcNone, SURFACE_ID_TEXT_BOX);
			PutBitmap3(&rcView, PixelToScreenCoord((i * 40) + (WINDOW_WIDTH - 192) / 2), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 64), &rcNone, SURFACE_ID_TEXT_BOX);
		}
	}

	for (i = 0; i < ITEM_MAX; ++i)
	{
		if (gItemData[i].code == 0)
			break;	// Invalid item

		// Get rect for next item
		rcItem.left = (gItemData[i].code % 8) * 32;
		rcItem.right = rcItem.left + 32;
		rcItem.top = (gItemData[i].code / 8) * 16;
		rcItem.bottom = rcItem.top + 16;

		PutBitmap3(&rcView, PixelToScreenCoord(((i % 6) * 32) + (WINDOW_WIDTH / 2) - 112), PixelToScreenCoord(((i / 6) * 16) + (WINDOW_HEIGHT / 2) - 28), &rcItem, SURFACE_ID_ITEM_IMAGE);
	}

	if (gCampActive == TRUE)
		// Draw items cursor
		PutBitmap3(&rcView, PixelToScreenCoord(((gSelectedItem % 6) * 32) + (WINDOW_WIDTH / 2) - 120), PixelToScreenCoord(((gSelectedItem / 6) * 16) + (WINDOW_HEIGHT / 2) - 28), &rcCur, SURFACE_ID_TEXT_BOX);
	else
		// Draw arms cursor
		PutBitmap3(&rcView, PixelToScreenCoord((gSelectedArms * 40) + (WINDOW_WIDTH / 2) - 120), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 56), &rcCur, SURFACE_ID_TEXT_BOX);

	// Draw MyChar
	PutBitmap3(&rcView, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122 + 14), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 112 + 20), &rcMyChar, SURFACE_ID_MY_CHAR);
}

int CampLoop(void)
{
	std::string old_script_path;

	RECT rcView = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

	// Save the current script path (to restore it when we get out of the inventory)
	old_script_path = GetTextScriptPath();

	// Load the inventory script
	LoadTextScript2("ArmsItem.tsc");

	gCampTitleY = (WINDOW_HEIGHT / 2) - 96;

	// Put the cursor on the first weapon
	gCampActive = FALSE;
	gSelectedItem = 0;

	// Compute current amount of weapons
	int arms_num = 0;
	while (gArmsData[arms_num].code != 0)
		++arms_num;

	if (arms_num != 0)
		StartTextScript(1000 + gArmsData[gSelectedArms].code);
	else
		StartTextScript(5000 + gItemData[gSelectedItem].code);

	for (;;)
	{
		GetTrg();

		/*if (gKeyTrg & KEY_PAUSE)
		{
			switch (Call_Pause())
			{
				case enum_ESCRETURN_exit:
					return enum_ESCRETURN_exit;	// Quit game

				case enum_ESCRETURN_restart:
					return enum_ESCRETURN_restart;	// Go to game intro
			}
		}*/

		// Handle ESC
		if (gKeyTrg & KEY_ESCAPE)
		{
			switch (Call_Escape())
			{
				case enum_ESCRETURN_exit:
					return enum_ESCRETURN_exit;	// Quit game

				case enum_ESCRETURN_restart:
					return enum_ESCRETURN_restart;	// Go to game intro
			}
		}

		if (g_GameFlags & GAME_FLAG_IS_CONTROL_ENABLED)
			MoveCampCursor();

		switch (TextScriptProc())
		{
			case enum_ESCRETURN_exit:
				return enum_ESCRETURN_exit;	// Quit game

			case enum_ESCRETURN_restart:
				return enum_ESCRETURN_restart;	// Go to game intro
		}

		// Get currently displayed image
		PutBitmap4(&rcView, 0, 0, &rcView, SURFACE_ID_SCREEN_GRAB);
		PutCampObject();
		PutTextScript();
		PutFramePerSecound();

		// Check whether we're getting out of the loop
		if (gCampActive)
		{
			if (g_GameFlags & GAME_FLAG_IS_CONTROL_ENABLED && gKeyTrg & (gKeyCancel | gKeyItem))
			{
				StopTextScript();
				break;
			}
		}
		else
		{
			if (gKeyTrg & (gKeyOk | gKeyCancel | gKeyItem))
			{
				StopTextScript();
				break;
			}
		}

		if (!Flip_SystemTask())
			return enum_ESCRETURN_exit;	// Quit game
	}

	// Resume original script
	LoadTextScript_Stage(old_script_path.c_str());
	gArmsEnergyX = 32; // Displays weapon rotation animation in case the weapon was changed
	return enum_ESCRETURN_continue;	// Go to game
}

BOOL CheckItem(long a)
{
	int i;

	for (i = 0; i < ITEM_MAX; ++i)
		if (gItemData[i].code == a)
			return TRUE;	// Found

	return FALSE;	// Not found
}

BOOL CheckArms(long a)
{
	int i;

	for (i = 0; i < ARMS_MAX; ++i)
		if (gArmsData[i].code == a)
			return TRUE;	// Found

	return FALSE;	// Not found
}

BOOL UseArmsEnergy(long num)
{
	if (gArmsData[gSelectedArms].max_num == 0)
		return TRUE;	// No ammo needed
	if (gArmsData[gSelectedArms].num == 0)
		return FALSE;	// No ammo left

	gArmsData[gSelectedArms].num -= num;

	if (gArmsData[gSelectedArms].num < 0)
		gArmsData[gSelectedArms].num = 0;

	return TRUE;	// Was able to spend ammo
}

BOOL ChargeArmsEnergy(long num)
{
	gArmsData[gSelectedArms].num += num;

	// Cap the ammo to the maximum ammunition
	if (gArmsData[gSelectedArms].num > gArmsData[gSelectedArms].max_num)
		gArmsData[gSelectedArms].num = gArmsData[gSelectedArms].max_num;

	return TRUE;	// Always successfull
}

void FullArmsEnergy(void)
{
	int a;

	for (a = 0; a < ARMS_MAX; ++a)
	{
		if (gArmsData[a].code == 0)
			continue;	// Don't change empty weapons

		gArmsData[a].num = gArmsData[a].max_num;
	}
}

int RotationArms(void)
{
	// Get amount of weapons
	int arms_num = 0;
	while (gArmsData[arms_num].code != 0)
		++arms_num;

	if (arms_num == 0)
		return 0;

	ResetSpurCharge();

	// Select next valid weapon
	++gSelectedArms;

	while (gSelectedArms < arms_num)
	{
		if (gArmsData[gSelectedArms].code)
			break;

		++gSelectedArms;
	}

	if (gSelectedArms == arms_num)
		gSelectedArms = 0;

	gArmsEnergyX = 32;
	PlaySoundObject(SND_SWITCH_WEAPON, SOUND_MODE_PLAY);

	return gArmsData[gSelectedArms].code;
}

int RotationArmsRev(void)
{
	// Get amount of weapons
	int arms_num = 0;
	while (gArmsData[arms_num].code != 0)
		++arms_num;

	if (arms_num == 0)
		return 0;

	ResetSpurCharge();

	// Select previous valid weapon
	--gSelectedArms;

	if (gSelectedArms < 0)
		gSelectedArms = arms_num - 1;

	while (gSelectedArms < arms_num)
	{
		if (gArmsData[gSelectedArms].code)
			break;

		--gSelectedArms;
	}

	gArmsEnergyX = 0;
	PlaySoundObject(SND_SWITCH_WEAPON, SOUND_MODE_PLAY);

	return gArmsData[gSelectedArms].code;
}

void ChangeToFirstArms(void)
{
	gSelectedArms = 0;
	gArmsEnergyX = 32;
	PlaySoundObject(SND_SWITCH_WEAPON, SOUND_MODE_PLAY);
}
