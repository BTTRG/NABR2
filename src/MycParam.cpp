#include "MycParam.h"

#include <stddef.h>
#include <stdio.h>
#include <string>

#include "WindowsWrapper.h"

#include "ArmsItem.h"
#include "CommonDefines.h"
#include "Caret.h"
#include "Draw.h"
#include "File.h"
#include "Game.h"
#include "Main.h"
#include "MyChar.h"
#include "NpChar.h"
#include "Sound.h"
#include "TextScr.h"
#include "ValueView.h"

ARMS_LEVEL gArmsLevelTable[14] =
{
	{{0,  0,  100}},
	{{30, 40, 16}},
	{{10, 20, 10}},
	{{10, 20, 20}},
	{{30, 40, 10}},
	{{10, 20, 10}},
	{{10, 20, 30}},
	{{10, 20, 5}},
	{{10, 20, 100}},
	{{30, 60, 0}},
	{{30, 60, 10}},
	{{10, 20, 100}},
	{{1,  1,  1}},
	{{40, 60, 200}}
};


void SetDestroyMyChar(int x, int y, int w, int num)
{
	int i;
	int offset_x;
	int offset_y;

	// Create smoke
	w /= 0x200;
	for (i = 0; i < num; ++i)
	{
		offset_x = Random(-w, w) * 0x200;
		offset_y = Random(-w, w) * 0x200;
		SetNpChar(4, x + offset_x, y + offset_y, 0, 0, 1, NULL, 0x100);//SetNpChar(NPC_SMOKE, x + offset_x, y + offset_y, 0, 0, 0, NULL, 0x100);
	}

	// Flash effect
	SetCaret(x, y, 12, 0);
}
void AddExpMyChar(int x)
{
	/*int lv = gArmsData[gSelectedArms].level - 1;
	int arms_code = gArmsData[gSelectedArms].code;

	gArmsData[gSelectedArms].exp += x;

	if (lv == 2)
	{
		if (gArmsData[gSelectedArms].exp >= gArmsLevelTable[arms_code].exp[lv])
		{
			gArmsData[gSelectedArms].exp = gArmsLevelTable[arms_code].exp[lv];

			if (gMC.equip & EQUIP_WHIMSICAL_STAR)
			{
				if (gMC.star < 3)
					++gMC.star;
			}
		}
	}
	else
	{
		for (; lv < 2; ++lv)
		{
			if (gArmsData[gSelectedArms].exp >= gArmsLevelTable[arms_code].exp[lv])
			{
				++gArmsData[gSelectedArms].level;
				gArmsData[gSelectedArms].exp = 0;

				if (gArmsData[gSelectedArms].code != 13)
				{
					PlaySoundObject(27, SOUND_MODE_PLAY);
					SetCaret(gMC.x, gMC.y, 10, 0);
				}
			}
		}
		if (gArmsData[gSelectedArms].code != 13)
		{
			gMC.exp_count += x;
			gMC.exp_wait = 30;
		}
		else
		{
			gMC.exp_wait = 10;
			gArmsData[gSelectedArms].level = 1;
			gArmsData[gSelectedArms].exp = 0;
		}
	}*/
	if (cion < 9999) // Cap Cion
		cion += x;
	/*if (x == 1) {
		SetCaret(gMC.x, gMC.y, 21, 0);
	}
	else if (x == 3) {
		SetCaret(gMC.x, gMC.y, 22, 0);
	}
	else if (x == 8) {
		SetCaret(gMC.x, gMC.y, 23, 0);
	}*/
}

void ZeroExpMyChar(void)
{
	gArmsData[gSelectedArms].level = 1;
	gArmsData[gSelectedArms].exp = 0;
}

BOOL IsMaxExpMyChar(void)
{
	int arms_code;

	if (gArmsData[gSelectedArms].level == 3)
	{
		arms_code = gArmsData[gSelectedArms].code;

		if (gArmsData[gSelectedArms].exp >= gArmsLevelTable[arms_code].exp[2])
			return TRUE;
	}

	return FALSE;
}

void DamageMyChar(int damage)
{
#ifdef FIX_BUGS
	if (!(g_GameFlags & 2))
#else
	// I'm preeeetty sure this is a typo. The Linux port optimised it out.
	if (!(g_GameFlags | 2))
#endif
		return;

	if (gMC.shock)
		return;

	// Damage player
	PlaySoundObject(16, SOUND_MODE_PLAY);
	gMC.cond &= ~1;
	gMC.shock = 128;

	if (gMC.unit == 1)
	{
		// Another weird case where there *has* to be an empty 'if' here to produce the same assembly.
		// Chances are there used to be some commented-out code here.
	}
	else
	{
		gMC.ym = -0x400;
	}

	gMC.life -= (short)damage;

	// Lose a whimsical star
	if (gMC.equip & EQUIP_WHIMSICAL_STAR && gMC.star > 0)
		gMC.star = (short)gMC.star - 1;	// For some reason, this does a cast to short. Might not be accurate to the original source code (possibly, Pixel was just being careful about int size/conversion, or this is from some weird macro)

	// Lose experience
	if (gMC.equip & EQUIP_ARMS_BARRIER)
		gArmsData[gSelectedArms].exp -= damage;
	else
		gArmsData[gSelectedArms].exp -= damage * 2;

	while (gArmsData[gSelectedArms].exp < 0)
	{
		if (gArmsData[gSelectedArms].level > 1)
		{
			--gArmsData[gSelectedArms].level;

			int lv = gArmsData[gSelectedArms].level - 1;
			int arms_code = gArmsData[gSelectedArms].code;

			gArmsData[gSelectedArms].exp = gArmsLevelTable[arms_code].exp[lv] + gArmsData[gSelectedArms].exp;

			if (gMC.life > 0 && gArmsData[gSelectedArms].code != 13)
				SetCaret(gMC.x, gMC.y, 10, 2);
		}
		else
		{
			gArmsData[gSelectedArms].exp = 0;
		}
	}

	// Tell player how much damage was taken
	SetValueView(&gMC.x, &gMC.y, -damage);

	// Death
	if (gMC.life <= 0)
	{
		PlaySoundObject(17, SOUND_MODE_PLAY);
		gMC.cond = 0;
		SetDestroyMyChar(gMC.x, gMC.y, 0x1400, 0x40);
		StartTextScript(40);
	}
}

void ZeroArmsEnergy_All(void)
{
	int a;

	for (a = 0; a < ARMS_MAX; a++)
	{
		gArmsData[a].level = 1;
		gArmsData[a].exp = 0;
	}
}

void AddBulletMyChar(int no, int val)
{
	int a;

	// Missile Launcher
	a = 0;
	while (a < ARMS_MAX && gArmsData[a].code != 5)
		++a;

	if (a == ARMS_MAX)
	{
		// Super Missile Launcher
		a = 0;
		while (a < ARMS_MAX && gArmsData[a].code != 10)
			++a;

		if (a == ARMS_MAX)
			return;
	}

	gArmsData[a].num += val;
	if (gArmsData[a].num > gArmsData[a].max_num)
		gArmsData[a].num = gArmsData[a].max_num;
}

void AddLifeMyChar(int x)
{
	gMC.life += (short)x;
	if (gMC.life > gMC.max_life)
		gMC.life = gMC.max_life;
	gMC.lifeBr = gMC.life;
}

void AddMaxLifeMyChar(int val)
{
	gMC.max_life += (short)val;
	if (gMC.max_life > 232)
		gMC.max_life = 232;
	gMC.life += (short)val;
	gMC.lifeBr = gMC.life;
}

void PutActiveArmsList(void)
{
	RECT rect = {0, 0, 0, 16};

	// Draw icon
	rect.left = gArmsData[gSelectedArms].code * 32;
	rect.right = rect.left + 32;
	// Put your X and Y values here

	PutBitmap3(&grcGame, PixelToScreenCoord(WINDOW_WIDTH - 40), PixelToScreenCoord(16), &rect, SURFACE_ID_ARMS_IMAGE);
	PutNumber4(WINDOW_WIDTH - 40, 24, gArmsData[gSelectedArms].num, FALSE);

}
void PutArmsEnergy(BOOL flash)
{
	static unsigned char add_flash;

	RECT rcPer = { 72, 48, 80, 56 };
	RECT rcView = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	RECT rcNone = { 80, 48, 96, 56 };

	// Draw max ammo
	if (gArmsData[gSelectedArms].max_num)
	{
		//PutNumber4(32, 24, gArmsData[gSelectedArms].num, FALSE);
	}
	else
	{
		PutBitmap3(&rcView, PixelToScreenCoord(gArmsEnergyX + 48), PixelToScreenCoord(16), &rcNone, SURFACE_ID_TEXT_BOX);
		PutBitmap3(&rcView, PixelToScreenCoord(gArmsEnergyX + 48), PixelToScreenCoord(24), &rcNone, SURFACE_ID_TEXT_BOX);
	}

	// Draw experience and ammo
	if (flash == TRUE && (gMC.shock / 2) % 2)
		return;

	int lv = gArmsData[gSelectedArms].level - 1;

#ifdef FIX_BUGS
	// When the player has no weapons, the default level is 0, which becomes -1.
	// Catch it, and set it to 0 instead, so the following array-accesses aren't
	// out-of-bounds.
	if (lv < 0)
		lv = 0;
#endif
}
void PutMyLife(BOOL flash)
{
	static unsigned int add_flash;

	RECT rcLife[2] = {
		{120, 112, 128, 120},
		{112, 112, 120, 120},
	};
	if (flash == TRUE && (gMC.shock / 2) % 2 && gMC.life != 1)
		return;
	if (flash == TRUE && gMC.life == 1 && ((add_flash++ / 2) % 2))
		return;
	for (int i = 0; i < gMC.max_life - 1; i++) // For every 1 in max life, add 1 to i, and run the following code
	{
		// Put a heart
		PutBitmap3(
			&grcGame, // Target
			// On the next two lines TT7 forgot PixelToScreenCoord, which is needed for the PutBitmap to work on resolutions other than 1x 
			PixelToScreenCoord(16 + (8 * i)), // X position, offset by 8 for every 1 in i
			PixelToScreenCoord(225), // Y position,
			& rcLife[gMC.life - 1 > i], // Which rect to use, 'gMC.life - 1 > i' checks if  the current heart that is being drawn is full or not, returns 0 if it's not and 1 if it is
			SURFACE_ID_TEXT_BOX); // Surface
	}
}

void PutCion()
{
	RECT rcCion = {128, 112, 152, 120};
	int i;
	if (cion > 999)
		i = 8;
	else
		i = 0;
	PutBitmap3(&grcGame, PixelToScreenCoord(32 + i), PixelToScreenCoord(16), &rcCion, SURFACE_ID_TEXT_BOX);
	PutNumber4(0 + i, 16, cion, FALSE);
}

void PutMyAir(int x, int y)
{
	RECT rcAir[2] = {
		{112, 72, 144, 80},
		{112, 80, 144, 88},
	};

	if (gMC.no_splash_or_air_limit_underwater)
		return;

	if (gMC.equip & EQUIP_AIR_TANK)
		return;

	if (gMC.air_get != 0)
	{
		// Draw how much air is left
		if (gMC.air_get % 6 < 4)
			PutNumber4(x + 32, y, gMC.air / 10, FALSE);

		// Draw "AIR" text
		if (gMC.air % 30 > 10)
			PutBitmap3(&grcGame, PixelToScreenCoord(x), PixelToScreenCoord(y), &rcAir[0], SURFACE_ID_TEXT_BOX);
		else
			PutBitmap3(&grcGame, PixelToScreenCoord(x), PixelToScreenCoord(y), &rcAir[1], SURFACE_ID_TEXT_BOX);
	}
}

static int time_count;

void PutTimeCounter(int x, int y)
{
	RECT rcTime[3] = {
		{112, 104, 120, 112},
		{120, 104, 128, 112},
		{128, 104, 160, 112},
	};

	if (gMC.equip & EQUIP_NIKUMARU_COUNTER)
	{
		// Draw clock and increase time
		if (g_GameFlags & 2)
		{
			if (time_count < 100 * 60 * (gb60fps ? 60 : 50))	// 100 minutes
				++time_count;

			if (time_count % 30 > 10)
				PutBitmap3(&grcGame, PixelToScreenCoord(x), PixelToScreenCoord(y), &rcTime[0], SURFACE_ID_TEXT_BOX);
			else
				PutBitmap3(&grcGame, PixelToScreenCoord(x), PixelToScreenCoord(y), &rcTime[1], SURFACE_ID_TEXT_BOX);
		}
		else
		{
			PutBitmap3(&grcGame, PixelToScreenCoord(x), PixelToScreenCoord(y), &rcTime[0], SURFACE_ID_TEXT_BOX);
		}

		// Draw time
		if (gb60fps)
		{
			PutNumber4(x,		y, time_count / (60 * 60),	FALSE);
			PutNumber4(x + 20,	y, time_count / 60 % 60,	TRUE);
			PutNumber4(x + 32,	y, time_count / 6 % 10,		FALSE);
		}
		else
		{
			PutNumber4(x,		y, time_count / (60 * 50),	FALSE);
			PutNumber4(x + 20,	y, time_count / 50 % 60,	TRUE);
			PutNumber4(x + 32,	y, time_count / 5 % 10,		FALSE);
		}
		PutBitmap3(&grcGame, PixelToScreenCoord(x + 30), PixelToScreenCoord(y), &rcTime[2], SURFACE_ID_TEXT_BOX);
	}
	else
	{
		time_count = 0;
	}
}

BOOL SaveTimeCounter(void)
{
	int i;
	unsigned char p[4];
	REC rec;
	FILE *fp;
	std::string path;

	// Quit if player doesn't have the Nikumaru Counter
	if (!(gMC.equip & EQUIP_NIKUMARU_COUNTER))
		return TRUE;

	// Get last time
	path = gModulePath + "/290.rec";

	fp = fopen(path.c_str(), "rb");
	if (fp != NULL)
	{
		// Read data
		rec.counter[0] = File_ReadLE32(fp);
		rec.counter[1] = File_ReadLE32(fp);
		rec.counter[2] = File_ReadLE32(fp);
		rec.counter[3] = File_ReadLE32(fp);
		rec.random[0] = fgetc(fp);
		rec.random[1] = fgetc(fp);
		rec.random[2] = fgetc(fp);
		rec.random[3] = fgetc(fp);
		fclose(fp);

		p[0] = (unsigned char)(rec.counter[0] - rec.random[0]);
		p[1] = (unsigned char)((rec.counter[0] >> 8) - rec.random[0]);
		p[2] = (unsigned char)((rec.counter[0] >> 16) - rec.random[0]);
		p[3] = (unsigned char)((rec.counter[0] >> 24) - rec.random[0] / 2);

		rec.counter[0] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);

		// If this is faster than our new time, quit
		if (rec.counter[0] < time_count)
			return TRUE;
	}

	// Save new time
	for (i = 0; i < 4; ++i)
	{
		rec.counter[i] = time_count;
		rec.random[i] = Random(0, 250) + i;

		p[0] = (unsigned char)(rec.counter[i] + rec.random[i]);
		p[1] = (unsigned char)((rec.counter[i] >> 8) + rec.random[i]);
		p[2] = (unsigned char)((rec.counter[i] >> 16) + rec.random[i]);
		p[3] = (unsigned char)((rec.counter[i] >> 24) + rec.random[i] / 2);

		rec.counter[i] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	}

	fp = fopen(path.c_str(), "wb");
	if (fp == NULL)
		return FALSE;

	File_WriteLE32(rec.counter[0], fp);
	File_WriteLE32(rec.counter[1], fp);
	File_WriteLE32(rec.counter[2], fp);
	File_WriteLE32(rec.counter[3], fp);
	fputc(rec.random[0], fp);
	fputc(rec.random[1], fp);
	fputc(rec.random[2], fp);
	fputc(rec.random[3], fp);

	fclose(fp);
	return TRUE;
}

int LoadTimeCounter(void)
{
	int i;
	unsigned char p[4];
	REC rec;
	FILE *fp;
	std::string path;

	// Open file
	path = gModulePath + "/290.rec";

	fp = fopen(path.c_str(), "rb");
	if (fp == NULL)
		return 0;

	// Read data
	rec.counter[0] = File_ReadLE32(fp);
	rec.counter[1] = File_ReadLE32(fp);
	rec.counter[2] = File_ReadLE32(fp);
	rec.counter[3] = File_ReadLE32(fp);
	rec.random[0] = fgetc(fp);
	rec.random[1] = fgetc(fp);
	rec.random[2] = fgetc(fp);
	rec.random[3] = fgetc(fp);
	fclose(fp);

	// Decode from checksum
	for (i = 0; i < 4; ++i)
	{
		p[0] = (unsigned char)(rec.counter[i] - rec.random[i]);
		p[1] = (unsigned char)((rec.counter[i] >> 8) - rec.random[i]);
		p[2] = (unsigned char)((rec.counter[i] >> 16) - rec.random[i]);
		p[3] = (unsigned char)((rec.counter[i] >> 24) - rec.random[i] / 2);

		rec.counter[i] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	}

	// Verify checksum's result
	if (rec.counter[0] != rec.counter[1] || rec.counter[0] != rec.counter[2])
	{
		time_count = 0;
		return 0;
	}

	time_count = rec.counter[0];
	return time_count;
}
