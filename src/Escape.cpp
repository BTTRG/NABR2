#include "WindowsWrapper.h"
#include "Draw.h"
#include "KeyControl.h"

int Call_Escape()
{
	RECT rc = {0, 128, 208, 144};
	
	while (Flip_SystemTask())
	{
		//Get pressed keys
		GetTrg();
		
		if (gKeyTrg & KEY_ESCAPE) //Escape is pressed, quit game
		{
			gKeyTrg = 0;
			return 0;
		}
		if (gKeyTrg & KEY_F1) //F1 is pressed, continue
		{
			gKeyTrg = 0;
			return 1;
		}
		if (gKeyTrg & KEY_F2) //F2 is pressed, reset
		{
			gKeyTrg = 0;
			return 2;
		}
		
		//Draw screen
		CortBox(&grcFull, 0x000000);
		PutBitmap3(&grcFull, 56, 112, &rc, 26);
		//PutFramePerSecound();
	}
	
	//Quit if window is closed
	gKeyTrg = 0;
	return 0;
}