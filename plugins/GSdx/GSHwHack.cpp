/*
 *	Copyright (C) 2007-2016 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSState.h"
#include "GSdx.h"

int s_crc_hack_level = 3;

// hacks
#define Aggresive (s_crc_hack_level > 3)
#define Dx_only   (s_crc_hack_level > 2)

struct GSFrameInfo
{
	uint32 FBP;
	uint32 FPSM;
	uint32 FBMSK;
	uint32 TBP0;
	uint32 TPSM;
	uint32 TZTST;
	bool TME;
};

typedef bool (*GetSkipCount)(const GSFrameInfo& fi, int& skip);
CRC::Region g_crc_region = CRC::NoRegion;

////////////////////////////////////////////////////////////////////////////////
// Broken on both DirectX and OpenGL
// (note: could potentially work with latest OpenGL)
////////////////////////////////////////////////////////////////////////////////

// Potentially partially dx only
bool GSC_DBZBT2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && /*fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 &&*/ (fi.TBP0 == 0x01c00 || fi.TBP0 == 0x02000) && fi.TPSM == PSM_PSMZ16)
		{
			if (Dx_only) // Feel like texture shuffle but not sure
				skip = 26; //27
		}
		else if(!fi.TME && (fi.FBP == 0x02a00 || fi.FBP == 0x03000) && fi.FPSM == PSM_PSMCT16)
		{
			skip = 10;
		}
	}

	return true;
}

// Potentially partially dx only
bool GSC_DBZBT3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMT8H)
		{
			//not needed anymore?
			//skip = 24; // blur
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8H)
		{
			if (Dx_only) { // Ought to be fine with blending accuracy (fbmask?)
				if(fi.FBMSK == 0x00000)
				{
					skip = 28; // outline
				}
				if(fi.FBMSK == 0x00FFFFFF)
				{
					skip = 1;
				}
			}
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT16 && fi.TPSM == PSM_PSMZ16)
		{
			// Texture shuffling must work on openGL
			if (Dx_only)
				skip = 5;
		}
		else if(fi.TME && fi.FPSM == fi.TPSM && fi.TBP0 == 0x03f00 && fi.TPSM == PSM_PSMCT32)
		{
			if (fi.FBP == 0x03400)
			{
				skip = 1;	//PAL
			}
			if(fi.FBP == 0x02e00)
			{
				skip = 3;	//NTSC
			}
		}
	}

    return true;
}

// Potentially partially dx only
bool GSC_ICO(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03d00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 3;
		}
		else if(fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02800 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 1;
		}
		else if( Aggresive && fi.TME && fi.FBP == 0x0800 && (fi.TBP0 == 0x2800 || fi.TBP0 ==0x2c00) && fi.TPSM ==0  && fi.FBMSK == 0)
		{
			skip = 1;
		}
	}
	else
	{
		if(fi.TME && fi.TBP0 == 0x00800 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_GT4(const GSFrameInfo& fi, int& skip)
{
	// Game requires to extract source from RT (block boundary) (texture cache limitation)
	if(skip == 0)
	{
		if(fi.TME && fi.FBP >= 0x02f00 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180 /*|| fi.TBP0 == 0x01a40*/) && fi.TPSM == PSM_PSMT8) //TBP0 0x1a40 progressive
		{
			skip = 770;	//ntsc, progressive 1540
		}
		if(g_crc_region == CRC::EU && fi.TME && fi.FBP >= 0x03400 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01400 ) && fi.TPSM == PSM_PSMT8)
		{
			skip = 880;	//pal
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01400) && fi.FPSM == PSM_PSMCT24 && fi.TBP0 >= 0x03420 && fi.TPSM == PSM_PSMT8)
		{
			// TODO: removes gfx from where it is not supposed to (garage)
			// skip = 58;
		}
	}

	return true;
}

bool GSC_GT3(const GSFrameInfo& fi, int& skip)
{
	// Same issue as GSC_GT4 ???
	if(skip == 0)
	{
		if(fi.TME && fi.FBP >= 0x02de0 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.TPSM == PSM_PSMT8)
		{
			skip = 770;
		}
	}

	return true;
}

bool GSC_GTConcept(const GSFrameInfo& fi, int& skip)
{
	// Same issue as GSC_GT4 ???
	if(skip == 0)
	{
		if(fi.TME && fi.FBP >= 0x03420 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01400) && fi.TPSM == PSM_PSMT8)
		{
			skip = 880;
		}
	}

	return true;
}

bool GSC_WildArms4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSM_PSMZ32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02a00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_WildArms5(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSM_PSMZ32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02a00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_Manhunt2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03c20 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x01400 && fi.TPSM == PSM_PSMT8)
		{
			skip = 640;
		}
	}

	return true;
}

bool GSC_CrashBandicootWoC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x00a00) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x008c0 || fi.TBP0 == 0x00a00) && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.FPSM == fi.TPSM)
		{
			return false; // allowed
		}

		if(fi.TME && (fi.FBP == 0x01e40 || fi.FBP == 0x02200)  && fi.FPSM == PSM_PSMZ24 && (fi.TBP0 == 0x01180 || fi.TBP0 == 0x01400) && fi.TPSM == PSM_PSMZ24)
		{
			skip = 42;
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x00a00) && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03c00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
		else if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x00a00))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_ResidentEvil4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 176;
		}
		else if(fi.TME && fi.FBP ==0x03100 && (fi.TBP0==0x2a00 ||fi.TBP0==0x3480) && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_SacredBlaze(const GSFrameInfo& fi, int& skip)
{
	//Fix Sacred Blaze rendering glitches
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP==0x0000 || fi.FBP==0x0e00) && (fi.TBP0==0x2880 || fi.TBP0==0x2a80 ) && fi.FPSM==fi.TPSM && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0x0)
		{
			skip = 1;
		}
	}
	return true;
}

bool GSC_Spartan(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(g_crc_region == CRC::EU &&fi.TME && fi.FBP == 0x02000 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 107;
		}
		if(g_crc_region == CRC::JP && fi.TME && fi.FBP == 0x02180 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x2180 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 3;
		}
		else
		{
				if(fi.TME)
				{
					// depth textures (bully, mgs3s1 intro, Front Mission 5)
					if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
						// General, often problematic post processing
						(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
					{
						skip = 1;
					}
				}
		}
	}

	return true;
}

bool GSC_AceCombat4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02a00 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x01600 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 71; // clouds (z, 16-bit)
		}
		else if(fi.TME && fi.FBP == 0x02900 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 28; // blur
		}
	}

	return true;
}

bool GSC_Tekken5(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x02d60 || fi.FBP == 0x02d80 || fi.FBP == 0x02ea0 || fi.FBP == 0x03620) && fi.FPSM == fi.TPSM && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 95;
		}
		else if(fi.TME && (fi.FBP == 0x02bc0 || fi.FBP == 0x02be0 || fi.FBP == 0x02d00) && fi.FPSM == fi.TPSM && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 2;
		}
		else if(fi.TME)
		{
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
				{
					skip = 24;
				}
		}
	}

	return true;
}

bool GSC_IkkiTousen(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00a80 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x01180 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 1000; // shadow (result is broken without depth copy, also includes 16 bit)
		}
		else if(fi.TME && fi.FBP == 0x00700 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x01180 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 11; // blur
		}
	}
	else if(skip > 7)
	{
		if(fi.TME && fi.FBP == 0x00700 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00700 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 7; // the last steps of shadow drawing
		}
	}

	return true;
}

bool GSC_Onimusha3(const GSFrameInfo& fi, int& skip)
{
	if(fi.TME /*&& (fi.FBP == 0x00000 || fi.FBP == 0x00700)*/ && (fi.TBP0 == 0x01180 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x01000 || fi.TBP0 == 0x01200) && (fi.TPSM == PSM_PSMCT32 || fi.TPSM == PSM_PSMCT24))
	{
		skip = 1;
	}

	return true;
}

bool GSC_TalesOfAbyss(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMT8) // copies the z buffer to the alpha channel of the fb
		{
			skip = 1000;
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && (fi.TBP0 == 0x03560 || fi.TBP0 == 0x038e0) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}
	else
	{
		if(fi.TME && fi.TPSM != PSM_PSMT8)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_Genji(const GSFrameInfo& fi, int& skip)
{
	if( !skip && fi.TME && (fi.FBP == 0x700 || fi.FBP == 0x0) && fi.TBP0 == 0x1500 && fi.TPSM )
		skip=1;

	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01500 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00e00 && fi.TPSM == PSM_PSMZ16)
		{
			// likely fixed in openGL (texture shuffle)
			if (Dx_only)
				skip = 6;
			else
				return false;
		}
		else if(fi.TPSM == PSM_PSMCT24 && fi.TME ==0x0001 && fi.TBP0==fi.FBP)
		{
			skip = 1;
		}
		else if(fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0)
		{
			skip = 1;
		}
	}
	else
	{
	}

	return true;
}

bool GSC_HauntingGround(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16S && fi.FBMSK == 0x03FFF)
		{
			if (Dx_only)
				skip = 1;
			else
				return false;
		}
		else if(fi.TME && fi.FBP == 0x3000 && fi.TBP0 == 0x3380)
		{
			skip = 1; // bloom
		}
		else if(fi.TME && (fi.FBP ==0x2200) && (fi.TBP0 ==0x3a80) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
		else if(fi.FBP ==0x2200 && fi.TBP0==0x3000 && fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0)
		{
			skip = 1;
		}
		else if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 1;
			}
		}
	}

	return true;
}

bool GSC_EvangelionJo(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.TBP0 == 0x2BC0 || (fi.FBP == 0 || fi.FBP == 0x1180) && (fi.FPSM | fi.TPSM) == 0)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_CaptainTsubasa(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x1C00 && !fi.FBMSK)
			{
				skip = 1;
			}
	}
	return true;
}

bool GSC_Oneechanbara2Special(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TPSM == PSM_PSMCT24 && fi.TME && fi.FBP == 0x01180)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_NarutimateAccel(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x3800 && fi.TBP0 == 0 && (fi.FPSM | fi.TPSM) == 0)
			{
				skip = 105;
			}
		else if(!fi.TME && fi.FBP == 0x3800 && fi.TBP0 == 0x1E00 && fi.FPSM == 0 && fi.TPSM == 49 && fi.FBMSK == 0xFF000000)
			{
				skip = 1;
			}
	}
	else
	{
		if(fi.FBP == 0 && fi.TBP0 == 0x3800 && fi.TME && (fi.FPSM | fi.TPSM) == 0)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_Naruto(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x3800 && fi.TBP0 == 0 && (fi.FPSM | fi.TPSM) == 0)
			{
				skip = 105;
			}
		else if(!fi.TME && fi.FBP == 0x3800 && fi.TBP0 == 0x1E00 && fi.FPSM == 0 && fi.TPSM == 49 && fi.FBMSK == 0xFF000000)
			{
				skip = 0;
			}
	}
	else
	{
		if(fi.FBP == 0 && fi.TBP0 == 0x3800 && fi.TME && (fi.FPSM | fi.TPSM) == 0)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_EternalPoison(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		// Texture shuffle ???
		if(fi.TPSM == PSM_PSMCT16S && fi.TBP0 == 0x3200)
		{
			skip = 1;
		}
	}
	return true;
}

bool GSC_SakuraTaisen(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && (fi.FBP == 0x0 || fi.FBP == 0x1180) && (fi.TBP0!=0x3fc0 && fi.TBP0!=0x3c9a && fi.TBP0 !=0x3dec /*fi.TBP0 ==0x38d0 || fi.TBP0==0x3912 ||fi.TBP0==0x3bdc ||fi.TBP0==0x3ab3 ||fi.TBP0<=0x3a92*/) && fi.FPSM == PSM_PSMCT32 && (fi.TPSM == PSM_PSMT8 || fi.TPSM == PSM_PSMT4) && (fi.FBMSK == 0x00FFFFFF || !fi.FBMSK))
		{
			skip = 0; //3dec 3fc0 3c9a
		}
		if(!fi.TME && (fi.FBP | fi.TBP0) !=0 && (fi.FBP | fi.TBP0) !=0x1180 && (fi.FBP | fi.TBP0) !=0x3be0 && (fi.FBP | fi.TBP0) !=0x3c80 && fi.TBP0!=0x3c9a  && (fi.FBP | fi.TBP0) !=0x3d80 && fi.TBP0 !=0x3dec&& fi.FPSM == PSM_PSMCT32 && (fi.FBMSK==0))
		{
			skip =0; //3dec 3fc0 3c9a
		}
		if(!fi.TME && (fi.FBP | fi.TBP0) !=0 && (fi.FBP | fi.TBP0) !=0x1180 && (fi.FBP | fi.TBP0) !=0x3be0 && (fi.FBP | fi.TBP0) !=0x3c80 && (fi.FBP | fi.TBP0) !=0x3d80 && fi.TBP0!=0x3c9a && fi.TBP0 !=0x3de && fi.FPSM == PSM_PSMCT32 && (fi.FBMSK==0))
		{
			skip =1; //3dec 3fc0 3c9a
		}
		else if(fi.TME && (fi.FBP == 0 || fi.FBP == 0x1180) && fi.TBP0 == 0x35B8 && fi.TPSM == PSM_PSMT4)
		{
			skip = 1;
		}
		else
		{
			if(!fi.TME && (fi.FBP | fi.TBP0) ==0x38d0 && fi.FPSM == PSM_PSMCT32 )
			{
				skip = 1; //3dec 3fc0 3c9a
			}
		}
	}

	return true;
}

bool GSC_ShadowofRome(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.FBP && fi.TPSM == PSM_PSMT8H && ( fi.FBMSK ==0x00FFFFFF))
		{
			skip =1;
		}
		else if(fi.TME ==0x0001 && (fi.TBP0==0x1300 || fi.TBP0==0x0f00) && fi.FBMSK>=0xFFFFFF)
		{
			skip = 1;
		}
		else if(fi.TME && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 ==0x0160 ||fi.TBP0==0x01e0 || fi.TBP0<=0x0800) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
		else if(fi.TME && (fi.TBP0==0x0700) && (fi.TPSM == PSM_PSMCT32 || fi.TPSM == PSM_PSMCT24))
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_TimeSplitters2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0x0FF000000)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_LordOfTheRingsThirdAge(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && fi.FBP == 0x03000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4 && fi.FBMSK == 0xFF000000)
		{
			skip = 1000;	//shadows
		}
	}
	else
	{
		if (fi.TME && (fi.FBP == 0x0 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03000 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_RedDeadRevolver(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && (fi.FBP == 0x02420 || fi.FBP == 0x025e0) && fi.FPSM == PSM_PSMCT24)
		{
			skip = 1200;
		}
		else if(fi.TME && (fi.FBP == 0x00800 || fi.FBP == 0x009c0) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x01600 || fi.TBP0 == 0x017c0) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 2;	//filter
		}
		else if(fi.FBP == 0x03700 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 2;	//blur
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00800 || fi.FBP == 0x009c0) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_HeavyMetalThunder(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x02a00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_BleachBladeBattlers(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01180 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x03fc0 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_TombRaider(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01000 && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}
	return true;
}

bool GSC_TombRaiderLegend(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01000 && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32 && (fi.TBP0 == 0x2b60 ||fi.TBP0 == 0x2b80 || fi.TBP0 == 0x2E60 ||fi.TBP0 ==0x3020 ||fi.TBP0 == 0x3200 || fi.TBP0 == 0x3320))
		{
			skip = 1;
		}
		else if(fi.TPSM == PSM_PSMCT32 && (fi.TPSM | fi.FBP)==0x2fa0 && (fi.TBP0==0x2bc0 ) && fi.FBMSK ==0)
		{
			skip = 2;
		}


	}// ||fi.TBP0 ==0x2F00

	return true;
}

bool GSC_TombRaiderUnderWorld(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01000 && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32 && (fi.TBP0 == 0x2B60 /*|| fi.TBP0 == 0x2EFF || fi.TBP0 ==0x2F00 || fi.TBP0 == 0x3020*/ || fi.TBP0 >= 0x2C01 && fi.TBP0!=0x3029 && fi.TBP0!=0x302d))
		{
			skip = 1;
		}
		else if(fi.TPSM == PSM_PSMCT32 && (fi.TPSM | fi.FBP)==0x2c00 && (fi.TBP0 ==0x0ee0) && fi.FBMSK ==0)
		{
			skip = 2;
		}
		/*else if(fi.TPSM == PSM_PSMCT16 && (fi.TPSM | fi.FBP)>=0x0 && (fi.TBP0 >=0x0) && fi.FBMSK ==0)
		{
			skip = 600;
		}*/
	}

	return true;
}

bool GSC_FFVIIDoC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02c00 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 1;
		}
		if(!fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMCT24)
		{
			//skip = 1;
		}
	}

	return true;
}

bool GSC_DevilMayCry3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{

		if(Dx_only && fi.TME && fi.FBP == 0x01800 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x01000 && fi.TPSM == PSM_PSMZ16)
		{
			skip = 32;
		}
		if(fi.TME && fi.FBP == 0x01800 && fi.FPSM == PSM_PSMZ32 && fi.TBP0 == 0x0800 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 16;
		}
		if(fi.TME && fi.FBP == 0x01800 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x0 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 24;
		}
	}

	return true;
}

bool GSC_StarWarsForceUnleashed(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x038a0 || fi.FBP == 0x03ae0) && fi.FPSM == fi.TPSM && fi.TBP0 == 0x02300 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 1000;	//9, shadows
		}
	}
	else
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x034a0 || fi.TBP0 == 0x36e0) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2;
		}

	}

	return true;
}

bool GSC_StarWarsBattlefront(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP > 0x0 && fi.FBP < 0x01000) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 > 0x02000 && fi.TBP0 < 0x03000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_StarWarsBattlefront2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP > 0x01000 && fi.FBP < 0x02000) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 > 0x0 && fi.TBP0 < 0x01000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
		if(fi.TME && (fi.FBP > 0x01000 && fi.FBP < 0x02000) && fi.FPSM == PSM_PSMZ32 && (fi.TBP0 > 0x0 && fi.TBP0 < 0x01000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_BlackHawkDown(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(Dx_only && fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x01800 && fi.TPSM == PSM_PSMZ16)
		{
			skip = 2;	//wall of fog
		}
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8)
		{
			skip = 5;	//night filter
		}
	}

	return true;
}

bool GSC_Burnout(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x01dc0 || fi.FBP == 0x02200) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x01dc0 || fi.TBP0 == 0x02200) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 4;
		}
		else if(fi.TME && fi.FPSM == PSM_PSMCT16 && fi.TPSM == PSM_PSMZ16)	//fog
		{
			if (!Dx_only) return false;

			if(fi.FBP == 0x00a00 && fi.TBP0 == 0x01e00)
			{
				skip = 4; //pal
			}
			if(fi.FBP == 0x008c0 && fi.TBP0 == 0x01a40)
			{
				skip = 3; //ntsc
			}
		}
		else if (fi.TME && (fi.FBP == 0x02d60 || fi.FBP == 0x033a0) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x02d60 || fi.TBP0 == 0x033a0) && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0x0)
		{
			skip = 2; //impact screen
		}
	}

	return true;
}

bool GSC_MidnightClub3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP > 0x01d00 && fi.FBP <= 0x02a00) && fi.FPSM == PSM_PSMCT32 && (fi.FBP >= 0x01600 && fi.FBP < 0x03260) && fi.TPSM == PSM_PSMT8H)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_TalesOfLegendia(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x3f80 || fi.FBP == 0x03fa0) && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8)
		{
			skip = 3; //3, 9
		}
		if(fi.TME && fi.FBP == 0x3800 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 2;
		}
		if(fi.TME && fi.FBP && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x3d80)
		{
			skip = 1;
		}
		if(fi.TME && fi.FBP ==0x1c00 && (fi.TBP0==0x2e80 ||fi.TBP0==0x2d80) && fi.TPSM ==0  && fi.FBMSK == 0xff000000)
		{
			skip = 1;
		}
		if(!fi.TME && fi.FBP ==0x2a00 && (fi.TBP0==0x1C00 ) && fi.TPSM ==0  && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_NanoBreaker(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x0 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x03800 || fi.TBP0 == 0x03900) && fi.TPSM == PSM_PSMCT16S)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_Kunoichi(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && (fi.FBP == 0x0 || fi.FBP == 0x00700 || fi.FBP == 0x00800) && fi.FPSM == PSM_PSMCT32 && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 3;
		}
		if(fi.TME && (fi.FBP ==0x0700 || fi.FBP==0) && fi.TBP0==0x0e00 && fi.TPSM ==0  && fi.FBMSK == 0)
		{
			skip = 1;
		}
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 1;
			}
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x0e00) && fi.FPSM == PSM_PSMCT32 && fi.FBMSK == 0xFF000000)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_Yakuza(const GSFrameInfo& fi, int& skip)
{
	if(1
		&& !skip
		&& !fi.TME
		&& (0
			|| fi.FBP == 0x1c20 && fi.TBP0 == 0xe00		//ntsc (EU and US DVDs)
			|| fi.FBP == 0x1e20 && fi.TBP0 == 0x1000	//pal1
			|| fi.FBP == 0x1620 && fi.TBP0 == 0x800		//pal2
		)
		&& fi.TPSM == PSM_PSMZ24
		&& fi.FPSM == PSM_PSMCT32
		/*
		&& fi.FBMSK	==0xffffff
		&& fi.TZTST
		&& !GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)
		*/
	)
	{
		skip=3;
	}
	return true;
}

bool GSC_Yakuza2(const GSFrameInfo& fi, int& skip)
{
	if(1
		&& !skip
		&& !fi.TME
		&& (0
			|| fi.FBP == 0x1c20 && fi.TBP0 == 0xe00		//ntsc (EU DVD)
			|| fi.FBP == 0x1e20 && fi.TBP0 == 0x1000	//pal1
			|| fi.FBP == 0x1620 && fi.TBP0 == 0x800		//pal2
		)
		&& fi.TPSM == PSM_PSMZ24
		&& fi.FPSM == PSM_PSMCT32
		/*
		&& fi.FBMSK	==0xffffff
		&& fi.TZTST
		&& !GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)
		*/
	)
	{
		skip=17;
	}
	return true;
}

bool GSC_SkyGunner(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{

		if(!fi.TME && !(fi.FBP == 0x0 || fi.FBP == 0x00800 || fi.FBP == 0x008c0 || fi.FBP == 0x03e00) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x0 || fi.TBP0 == 0x01800) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1; //Huge Vram usage
		}
	}

	return true;
}

bool GSC_JamesBondEverythingOrNothing(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{

		if(fi.TME && (fi.FBP < 0x02000 && !(fi.FBP == 0x0 || fi.FBP == 0x00e00)) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 > 0x01c00 && fi.TBP0 < 0x03000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1; //Huge Vram usage
		}
	}

	return true;
}

bool GSC_ZettaiZetsumeiToshi2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
			if(fi.TME  && fi.TPSM == PSM_PSMCT16S  && (fi.FBMSK >= 0x6FFFFFFF || fi.FBMSK ==0) )
			{
				skip = 1000;
			}
			else if(fi.TME  && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0xFF000000)
			{
				skip = 2;
 			}
			else if((fi.FBP | fi.TBP0)&& fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x3FFF)
			{
				// Note start of the effect (texture shuffle) is fixed in openGL but maybe not the extra draw
				// call....
				skip = 1000;
			}

	}
	else
	{
			if(!fi.TME && fi.TPSM == PSM_PSMCT32  && fi.FBP==0x1180 && fi.TBP0==0x1180 && (fi.FBMSK ==0))
			{
				skip = 0; //
			}
			if(fi.TME && fi.TPSM == PSM_PSMT4  && fi.FBP && (fi.TBP0!=0x3753))
			{
				skip = 0; //
			}
			if(fi.TME && fi.TPSM == PSM_PSMT8H && fi.FBP ==0x22e0 && fi.TBP0 ==0x36e0 )
			{
				skip = 0; //
			}
			if(!fi.TME  && fi.TPSM == PSM_PSMT8H && fi.FBP ==0x22e0 )
			{
				skip = 0; //
			}
			if(fi.TME  && fi.TPSM == PSM_PSMT8 && (fi.FBP==0x1180 || fi.FBP==0) && (fi.TBP0 !=0x3764 && fi.TBP0!=0x370f))
			{
				skip = 0; //
			}
			if(fi.TME && fi.TPSM == PSM_PSMCT16S && (fi.FBP==0x1180 ))
			{
				skip = 2; //
			}

	}

	return true;
}

bool GSC_ShinOnimusha(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{

		if(fi.TME && fi.FBP == 0x001000 && (fi.TBP0 ==0 || fi.TBP0 == 0x0800) && fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 0;
		}
		else if(fi.TPSM == PSM_PSMCT24 && fi.TME && fi.FBP == 0x01000) // || fi.FBP == 0x00000
		{
			skip = 28; //28 30 56 64
		}
		else if(fi.FBP && fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0xFFFFFF)
		{
			skip = 0; //24 33 40 9
		}
		else if(fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0xFF000000)
		{
			skip = 1;
		}
		else if(fi.TME && (fi.TBP0 ==0x1400 || fi.TBP0 ==0x1000 ||fi.TBP0 == 0x1200) && (fi.TPSM == PSM_PSMCT32 || fi.TPSM == PSM_PSMCT24))
		{
			skip = 1;
		}

	}

	return true;
}

bool GSC_GetaWay(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if((fi.FBP ==0 || fi.FBP ==0x1180)&& fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_SakuraWarsSoLongMyLove(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME==0 && fi.FBP != fi.TBP0 && fi.TBP0 && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 3;
		}
		else if(fi.TME==0 && fi.FBP == fi.TBP0 && (fi.TBP0 ==0x1200 ||fi.TBP0 ==0x1180 ||fi.TBP0 ==0) && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 3;
		}
		else if(fi.TME && (fi.FBP ==0 || fi.FBP ==0x1180) && fi.FPSM == PSM_PSMCT32 && fi.TBP0 ==0x3F3F && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_FightingBeautyWulong(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.TBP0 ==0x0700 || fi.TBP0 ==0x0a80) && (fi.TPSM == PSM_PSMCT32 || fi.TPSM == PSM_PSMCT24))
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_TouristTrophy(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP >= 0x02f00 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.TPSM == PSM_PSMT8)
		{
			skip = 770;
		}
		if(fi.TME && fi.FBP >= 0x02de0 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 ==0 || fi.TBP0==0x1a40 ||fi.TBP0 ==0x2300) && fi.TPSM == PSM_PSMT8)
		{
			skip = 770; //480P
		}
	}

	return true;
}

bool GSC_GTASanAndreas(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP ==0x0a00 || fi.FBP ==0x08c0) && (fi.TBP0 ==0x1b80 || fi.TBP0 ==0x1a40) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_FrontMission5(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0)
		{
			skip = 1;
		}
		if(fi.TME && (fi.FBP ==0x1000) && (fi.TBP0 ==0x2e00 || fi.TBP0 ==0x3200) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1; //fi.TBP0 ==0x1f00
		}
	}

	return true;
}

bool GSC_GodHand(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP ==0x0) && (fi.TBP0 ==0x2800) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_KnightsOfTheTemple2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TPSM == PSM_PSMT8H && fi.FBMSK == 0)
		{
			skip = 1;
		}
		else if(fi.TPSM ==0x00000 && PSM_PSMCT24 && fi.TME && (fi.FBP ==0x3400 ||fi.FBP==0x3a00))
		{
			skip = 1 ;
		}
	}

	return true;
}

bool GSC_UltramanFightingEvolution(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP==0x2a00 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x1c00 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 5; // blur
		}
	}

	return true;
}

bool GSC_DeathByDegreesTekkenNinaWilliams(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP ==0 ) && fi.TBP0==0x34a0 && (fi.TPSM == PSM_PSMCT32))
		{
			skip = 1;
		}
		else if((fi.FBP ==0x3500)&& fi.TPSM == PSM_PSMT8 && fi.FBMSK == 0xFFFF00FF)
		{
			skip = 4;
		}
	}
	if(fi.TME)
		{
			if((fi.FBP | fi.TBP0 | fi.FPSM | fi.TPSM) && (fi.FBMSK == 0x00FFFFFF ))
			{
				skip = 1;
			}
		}
	return true;
}

bool GSC_AlpineRacer3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && fi.FBP == 0 && (fi.FBMSK ==0x0001 ||fi.FBMSK == 0x00FFFFFF))
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_HummerBadlands(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP ==0x0a00) && (fi.TBP0 ==0x03200 || fi.TBP0==0x3700) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_SengokuBasara(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME  && (fi.TBP0==0x1800 ) && fi.FBMSK==0xFF000000)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_FinalFightStreetwise(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && (fi.FBP == 0 || fi.FBP == 0x08c0) && fi.FPSM == PSM_PSMCT32 && (fi.TPSM == PSM_PSMT8 || fi.TPSM == PSM_PSMT4) && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_TalesofSymphonia(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x2bc0 || fi.TBP0 <= 0x0200) && (fi.FBMSK==0xFF000000 ||fi.FBMSK==0x00FFFFFF))
		{
			skip = 1; //fi.FBMSK==0
		}
		if(fi.TME  && (fi.TBP0==0x1180 || fi.TBP0==0x1a40 || fi.TBP0==0x2300) && fi.FBMSK>=0xFF000000)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_SoulCalibur2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 2;
			}
		}
	}

	return true;
}

bool GSC_SoulCalibur3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 2;
			}
		}
	}

	return true;
}

bool GSC_Simple2000Vol114(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME==0 && (fi.FBP==0x1500) && (fi.TBP0==0x2c97 || fi.TBP0==0x2ace || fi.TBP0==0x03d0 || fi.TBP0==0x2448) && (fi.FBMSK == 0x0000))
		{
			skip = 1;
		}
		if(fi.TME && (fi.FBP==0x0e00) && (fi.TBP0==0x1000) && (fi.FBMSK == 0x0000))
		{
			skip = 1;
		}
	}
	return true;
}

bool GSC_UrbanReign(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP==0x0000 && fi.TBP0==0x3980 && fi.FPSM==fi.TPSM && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0x0)
		{
			skip = 1;
		}
	}
	return true;
}

bool GSC_SteambotChronicles(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		// Author: miseru99 on forums.pcsx2.net
		if(fi.TME && fi.TPSM == PSM_PSMCT16S)
		{
			if(fi.FBP == 0x1180)
			{
				skip=1;//1 deletes some of the glitched effects
			}
			else if(fi.FBP == 0)
			{
				skip=100;//deletes most others(too high deletes the buggy sea completely;c, too low causes glitches to be visible)
			}
			else if(Aggresive && fi.FBP != 0)//Agressive CRC
			{
				skip=19;//"speedhack", makes the game very light, vaporized water can disappear when not looked at directly, possibly some interface still, other value to try: 6 breaks menu background, possibly nothing(?) during gameplay, but it's slower, hence not much of a speedhack anymore
			}
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Correctly emulated on OpenGL but can be used as potential speed hack
////////////////////////////////////////////////////////////////////////////////

bool GSC_Okami(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03800 && fi.TPSM == PSM_PSMT4)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_MetalGearSolid3(const GSFrameInfo& fi, int& skip)
{
	// Game requires sub RT support (texture cache limitation)
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02000 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT24)
		{
			skip = 1000; // 76, 79
		}
		else if(fi.TME && fi.FBP == 0x02800 && fi.FPSM == PSM_PSMCT24 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1000; // 69
		}
	}
	else
	{
		if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
		else if(!fi.TME && fi.FBP == fi.TBP0 && fi.TBP0 == 0x2000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMCT24)
		{
			if(g_crc_region == CRC::US || g_crc_region == CRC::JP || g_crc_region == CRC::KO)
			{
				skip = 119;	//ntsc
			}
			else
			{
				skip = 136;	//pal
			}
		}
	}

	return true;
}

bool GSC_SFEX3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00500 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00f00 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2; // blur
		}
	}

	return true;
}

bool GSC_Bully(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.FPSM == fi.TPSM)
		{
			return false; // allowed
		}

		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && fi.FPSM == PSM_PSMCT16S && fi.TBP0 == 0x02300 && fi.TPSM == PSM_PSMZ16S)
		{
			skip = 6;
		}
	}
	else
	{
		if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_BullyCC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.FPSM == fi.TPSM)
		{
			return false; // allowed
		}

		if(!fi.TME && fi.FBP == 0x02800 && fi.FPSM == PSM_PSMCT24)
		{
			skip = 9;
		}
	}

	return true;
}

bool GSC_OnePieceGrandAdventure(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02d00 && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x00f00) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 4;
		}
	}

	return true;
}

bool GSC_OnePieceGrandBattle(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02d00 && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00f00) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 4;
		}
	}

	return true;
}

bool GSC_GodOfWar(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 1000;
		}
		else if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0xff000000)
		{
			skip = 1; // blur
		}
		else if(fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8 && ((fi.TZTST == 2 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 1 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 3 && fi.FBMSK == 0xFF000000)))
		{
			skip = 1; // wall of fog
		}
		else if (fi.TME && (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S))
		{
			// Equivalent to the UserHacks_AutoSkipDrawDepth hack but enabled by default
			// http://forums.pcsx2.net/Thread-God-of-War-Red-line-rendering-explained
			skip = 1;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_GodOfWar2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME)
		{
			if( fi.FBP == 0x00100 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00100 && fi.TPSM == PSM_PSMCT16 // ntsc
				|| fi.FBP == 0x02100 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x02100 && fi.TPSM == PSM_PSMCT16) // pal
			{
				skip = 1000; // shadows
			}
			if((fi.FBP == 0x00100 || fi.FBP == 0x02100) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 & 0x03000) == 0x03000
				&& (fi.TPSM == PSM_PSMT8 || fi.TPSM == PSM_PSMT4)
				&& ((fi.TZTST == 2 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 1 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 3 && fi.FBMSK == 0xFF000000)))
			{
					skip = 1; // wall of fog
			}
			else if(Aggresive && fi.TPSM == PSM_PSMCT24 && fi.TME && (fi.FBP ==0x1300 ) && (fi.TBP0 ==0x0F00 || fi.TBP0 ==0x1300 || fi.TBP0==0x2b00)) // || fi.FBP == 0x0100
			{
				skip = 1; // global haze/halo
			}
			else if(Aggresive && fi.TPSM == PSM_PSMCT24 && fi.TME && (fi.FBP ==0x0100 ) && (fi.TBP0==0x2b00 || fi.TBP0==0x2e80)) //480P 2e80
			{
				skip = 1; // water effect and water vertical lines
			}
			else if (fi.TME && (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S))
			{
				// Equivalent to the UserHacks_AutoSkipDrawDepth hack but enabled by default
				// http://forums.pcsx2.net/Thread-God-of-War-Red-line-rendering-explained
				skip = 1;
			}
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00100 || fi.FBP == 0x02100) && fi.FPSM == PSM_PSMCT16)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_GiTS(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01400 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x02e40 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1315;
		}
	}
	else
	{
	}

	return true;
}

bool GSC_SonicUnleashed(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FPSM == PSM_PSMCT16S && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000; // shadow
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 && fi.TPSM == PSM_PSMCT16S)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_SimpsonsGame(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x03000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x03000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_StarOcean3(const GSFrameInfo& fi, int& skip)
{
	// The game emulate a stencil buffer with the alpha channel of the RT
	// The operation of the stencil is selected with the palette
	// For example -1 wrap will be [240, 16, 32, 48 ....]
	// i.e. p[A>>4] = (A - 16) % 256
	//
	// The fastest and accurate solution will be to replace this pseudo stencil
	// by a dedicated GPU draw call
	// 1/ Use future GPU capabilities to do a "kind" of SW blending
	// 2/ Use a real stencil/atomic image, and then compute the RT alpha value
	//
	// Both of those solutions will increase code complexity (and only avoid upscaling
	// glitches)

	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH)
		{
			skip = 1000; //
		}
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_ValkyrieProfile2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		/*if(fi.TME && (fi.FBP == 0x018c0 || fi.FBP == 0x02180) && fi.FPSM == fi.TPSM && fi.TBP0 >= 0x03200 && fi.TPSM == PSM_PSMCT32)	//NTSC only, !(fi.TBP0 == 0x03580 || fi.TBP0 == 0x03960)
		{
			skip = 1;	//red garbage in lost forest, removes other effects...
		}
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 1; // //garbage in cutscenes, doesn't remove completely, better use "Alpha Hack"
        }*/
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH)
		{
			// GH: Hack is quite similar to GSC_StarOcean3. It is potentially the same issue.
			skip = 1000; //
		}
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_RadiataStories(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
        {
			skip = 1;
        }
		else if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH)
		{
			// GH: Hack is quite similar to GSC_StarOcean3. It is potentially the same issue.
			// Fixed on openGL
			skip = 1000;
		}
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_SuikodenTactics(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if( !fi.TME && fi.TPSM == PSM_PSMT8H && fi.FPSM == 0 &&
			fi.FBMSK == 0x0FF000000 && fi.TBP0 == 0 && GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM))
		{
			skip = 4;
		}
	}

	return true;
}

bool GSC_Tenchu(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.TPSM == PSM_PSMZ16 && fi.FPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_Sly3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00700 || fi.FBP == 0x00a80 || fi.FBP == 0x00e00) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00700 || fi.TBP0 == 0x00a80 || fi.TBP0 == 0x00e00) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_Sly2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME &&  (fi.FBP == 0x00000 || fi.FBP == 0x00700 || fi.FBP == 0x00800) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_DemonStone(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01400 && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_BigMuthaTruckers(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_LordOfTheRingsTwoTowers(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x01180 || fi.FBP == 0x01400) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000;//shadows
		}
		else if(fi.TME && fi.TPSM == PSM_PSMZ16 && fi.TBP0 == 0x01400 && fi.FPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3;	//wall of fog
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && (fi.TBP0 == 0x01180 || fi.TBP0 == 0x01400) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_Castlevania(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		// This hack removes the shadows and globally darker image
		// I think there are 2 issues on GSdx
		//
		// 1/ potential not correctly supported colclip.
		//
		// 2/ use of a 32 bits format to emulate a 16 bit formats
		// For example, if you blend 64 time the value 4 on a dark destination pixels
		//
		// FMT32: 4*64 = 256 <= white pixels
		//
		// FMT16: output of blending will always be 0 because the 3 lsb of color is dropped.
		//		  Therefore the pixel remains dark !!!
		if(fi.TME && fi.FBP == 0 && fi.TBP0 && fi.TPSM == 10 && fi.FBMSK == 0xFFFFFF)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_Black(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		// Note: the first part of the hack must be fixed in openGL (texture shuffle). Remains the 2nd part (HasSharedBits)
		if(fi.TME /*&& (fi.FBP == 0x00000 || fi.FBP == 0x008c0)*/ && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x01a40 || fi.TBP0 == 0x01b80 || fi.TBP0 == 0x030c0) && fi.TPSM == PSM_PSMZ16 || (GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)))
		{
			skip = 5;
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x0a00 ) && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4)
		{
			skip = 0;
		}
		else if(!fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_CrashNburn(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 1;
			}
		}
	}

	return true;
}

bool GSC_SpyroNewBeginning(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x034a0 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_SpyroEternalNight(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x034a0 ||fi.TBP0 == 0x035a0 || fi.TBP0 == 0x036e0) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_XE3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TPSM == PSM_PSMT8H && fi.FBMSK >= 0xEFFFFFFF)
		{
			skip = 73;
		}
		else if(fi.TME && fi.FBP ==0x03800 && fi.TBP0 && fi.TPSM ==0  && fi.FBMSK == 0)
		{
			skip = 1;
		}
		/*else if(fi.TPSM ==0x00000 && PSM_PSMCT24 && fi.TME && fi.FBP == 0x03800)
		{
			skip = 1 ;
		}*/
		/*else if(fi.TME ==0  && (fi.FBP ==0 ) && fi.FPSM == PSM_PSMCT32 && ( fi.TPSM == PSM_PSMT8 || fi.TPSM == PSM_PSMT4) && (fi.FBMSK == 0x00FFFFFF || fi.FBMSK == 0xFF000000))
		{
			skip = 1;
		}*/
		else
		{
				if(fi.TME)
				{
					// depth textures (bully, mgs3s1 intro, Front Mission 5)
					if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
						// General, often problematic post processing
						(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
					{
						skip = 1;
					}
				}
		}
	}
	return true;
}

bool GSC_Grandia3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP ==0x0 || fi.FBP ==0x0e00) && (fi.TBP0 ==0x2a00 ||fi.TBP0==0x0e00 ||fi.TBP0==0) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}


	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Aggresive only hack
////////////////////////////////////////////////////////////////////////////////

template<uptr state_addr>
bool GSC_SMTNocturneDDS(const GSFrameInfo& fi, int& skip)
{
	// stop the motion blur on the main character and
	// smudge filter from being drawn on USA versions of
	// Nocturne, Digital Devil Saga 1 and Digital Devil Saga 2

	if(Aggresive && g_crc_region == CRC::US && skip == 0 && fi.TBP0 == 0xE00 && fi.TME)
	{
		// Note: it will crash if the core doesn't allocate the EE mem in 0x2000_0000 (unlikely but possible)
		// Aggresive hacks are evil anyway

		// Nocturne:
		// -0x5900($gp), ref at 0x100740
		const int state = *(int*)(state_addr);
		if (state == 23 || state == 24 || state == 25)
			skip = 1;
	}
	return true;
}

bool GSC_LegoBatman(const GSFrameInfo& fi, int& skip)
{
	if(Aggresive && skip == 0)
	{
		if(fi.TME && fi.TPSM == PSM_PSMZ16 && fi.FPSM == PSM_PSMCT16 && fi.FBMSK == 0x00000)
		{
			skip = 3;
		}
	}
	return true;
}

bool GSC_SoTC(const GSFrameInfo& fi, int& skip)
{
            // Not needed anymore? What did it fix anyway? (rama)
    if(skip == 0)
    {
            if(Aggresive && fi.TME /*&& fi.FBP == 0x03d80*/ && fi.FPSM == 0 && fi.TBP0 == 0x03fc0 && fi.TPSM == 1)
            {
                    skip = 48;	//removes sky bloom
            }
            /*
            if(fi.TME && fi.FBP == 0x02b80 && fi.FPSM == PSM_PSMCT24 && fi.TBP0 == 0x01e80 && fi.TPSM == PSM_PSMCT24)
            {
                    skip = 9;
            }
            else if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03800 && fi.TPSM == PSM_PSMCT32)
            {
                    skip = 8;
            }
            else if(fi.TME && fi.FBP == 0x01e80 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03880 && fi.TPSM == PSM_PSMCT32)
            {
                    skip = 8;
            }*/
    }





	return true;
}

bool GSC_FFXII(const GSFrameInfo& fi, int& skip)
{
	if(Aggresive && skip == 0)
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 1;
			}
		}
	}
	return true;
}

bool GSC_FFX2(const GSFrameInfo& fi, int& skip)
{
	if(Aggresive && skip == 0)
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 1;
			}
		}
	}
	return true;
}

bool GSC_FFX(const GSFrameInfo& fi, int& skip)
{
	if(Aggresive && skip == 0)
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 1;
			}
		}
	}
	return true;
}

bool GSC_SSX3(const GSFrameInfo& fi, int& skip)
{
	if(Aggresive && skip == 0)
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = 1;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

#undef Agressive

#ifdef ENABLE_DYNAMIC_CRC_HACK

#include <sys/stat.h>
/***************************************************************************
	AutoReloadLibrary : Automatically reloads a dll if the file was modified.
		Uses a temporary copy of the watched dll such that the original
		can be modified while the copy is loaded and used.

	NOTE: The API is not platform specific, but current implementation is Win32.
***************************************************************************/
class AutoReloadLibrary
{
private:
	string	m_dllPath, m_loadedDllPath;
	DWORD	m_minMsBetweenProbes;
	time_t	m_lastFileModification;
	DWORD	m_lastProbe;
	HMODULE	m_library;

	string	GetTempName()
	{
		string result = m_loadedDllPath + ".tmp"; //default name
		TCHAR tmpPath[MAX_PATH], tmpName[MAX_PATH];
		DWORD ret = GetTempPath(MAX_PATH, tmpPath);
		if(ret && ret <= MAX_PATH && GetTempFileName(tmpPath, TEXT("GSdx"), 0, tmpName))
			result = tmpName;

		return result;
	};

	void	UnloadLib()
	{
		if( !m_library )
			return;

		FreeLibrary( m_library );
		m_library = NULL;

		// If can't delete (might happen when GSdx closes), schedule delete on reboot
		if(!DeleteFile( m_loadedDllPath.c_str() ) )
			MoveFileEx( m_loadedDllPath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT );
	}

public:
	AutoReloadLibrary( const string dllPath, const int minMsBetweenProbes=100 )
		: m_minMsBetweenProbes( minMsBetweenProbes )
		, m_dllPath( dllPath )
		, m_lastFileModification( 0 )
		, m_lastProbe( 0 )
		, m_library( 0 )
	{};

	~AutoReloadLibrary(){ UnloadLib();	};

	// If timeout has ellapsed, probe the dll for change, and reload if it was changed.
	// If it returns true, then the dll was freed/reloaded, and any symbol addresse previously obtained is now invalid and needs to be re-obtained.
	// Overhead is very low when when probe timeout has not ellapsed, and especially if current timestamp is supplied as argument.
	// Note: there's no relation between the file modification date and currentMs value, so it need'nt neccessarily be an actual timestamp.
	// Note: isChanged is guarenteed to return true at least once
	//       (even if the file doesn't exist, at which case the following GetSymbolAddress will return NULL)
	bool isChanged( const DWORD currentMs=0 )
	{
		DWORD current = currentMs? currentMs : GetTickCount();
		if( current >= m_lastProbe && ( current - m_lastProbe ) < m_minMsBetweenProbes )
			return false;

		bool firstTime = !m_lastProbe;
		m_lastProbe = current;

		struct stat s;
		if( stat( m_dllPath.c_str(), &s ) )
		{
			// File doesn't exist or other error, unload dll
			bool wasLoaded = m_library?true:false;
			UnloadLib();
			return firstTime || wasLoaded;	// Changed if previously loaded or the first time accessing this method (and file doesn't exist)
		}

		if( m_lastFileModification == s.st_mtime )
			return false;
		m_lastFileModification = s.st_mtime;

		// File modified, reload
		UnloadLib();

		if( !CopyFile( m_dllPath.c_str(), ( m_loadedDllPath = GetTempName() ).c_str(), false ) )
			return true;

		m_library = LoadLibrary( m_loadedDllPath.c_str() );
		return true;
	};

	// Return value is NULL if the dll isn't loaded (failure or doesn't exist) or if the symbol isn't found.
	void* GetSymbolAddress( const char* name ){ return m_library? GetProcAddress( m_library, name ) : NULL; };
};


// Use DynamicCrcHack function from a dll which can be modified while GSdx/PCSX2 is running.
// return value is true if the call succeeded or false otherwise (If the hack could not be invoked: no dll/function/etc).
// result contains the result of the hack call.

typedef uint32 (__cdecl* DynaHackType)(uint32, uint32, uint32, uint32, uint32, uint32, uint32, int32*, uint32, int32);
typedef uint32 (__cdecl* DynaHackType2)(uint32, uint32, uint32, uint32, uint32, uint32, uint32, int32*, uint32, int32, uint32); // Also accept CRC

bool IsInvokedDynamicCrcHack( GSFrameInfo &fi, int& skip, int region, bool &result, uint32 crc )
{
	static AutoReloadLibrary dll( DYNA_DLL_PATH );
	static DynaHackType dllFunc = NULL;
	static DynaHackType2 dllFunc2 = NULL;

	if( dll.isChanged() )
	{
		dllFunc  = (DynaHackType)dll.GetSymbolAddress( "DynamicCrcHack" );
		dllFunc2 = (DynaHackType2)dll.GetSymbolAddress( "DynamicCrcHack2" );
		printf( "GSdx: Dynamic CRC-hacks%s: %s\n",
			((dllFunc && !dllFunc2)?" [Old dynaDLL - No CRC support]":""),
			dllFunc? "Loaded OK        (-> overriding internal hacks)" :
					 "Not available    (-> using internal hacks)");
	}

	if( !dllFunc2 && !dllFunc )
		return false;

	int32	skip32 = skip;
	bool	hasSharedBits = GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM);
	if(dllFunc2)
		result	= dllFunc2( fi.FBP, fi.FPSM, fi.FBMSK, fi.TBP0, fi.TPSM, fi.TZTST, (uint32)fi.TME, &skip32, (uint32)region, (uint32)(hasSharedBits?1:0), crc )?true:false;
	else
		result	= dllFunc( fi.FBP, fi.FPSM, fi.FBMSK, fi.TBP0, fi.TPSM, fi.TZTST, (uint32)fi.TME, &skip32, (uint32)region, (uint32)(hasSharedBits?1:0) )?true:false;
	skip	= skip32;

	return true;
}

#endif

bool GSState::IsBadFrame(int& skip, int UserHacks_SkipDraw)
{
	GSFrameInfo fi;

	fi.FBP = m_context->FRAME.Block();
	fi.FPSM = m_context->FRAME.PSM;
	fi.FBMSK = m_context->FRAME.FBMSK;
	fi.TME = PRIM->TME;
	fi.TBP0 = m_context->TEX0.TBP0;
	fi.TPSM = m_context->TEX0.PSM;
	fi.TZTST = m_context->TEST.ZTST;

	static GetSkipCount map[CRC::TitleCount];

	if (!m_crcinited)
	{
		m_crcinited = true;

		s_crc_hack_level = theApp.GetConfig("crc_hack_level", 3);

		memset(map, 0, sizeof(map));

		if (s_crc_hack_level > 1) {
			map[CRC::AceCombat4] = GSC_AceCombat4;
			map[CRC::AlpineRacer3] = GSC_AlpineRacer3;
			map[CRC::BlackHawkDown] = GSC_BlackHawkDown;
			map[CRC::BleachBladeBattlers] = GSC_BleachBladeBattlers;
			map[CRC::BurnoutDominator] = GSC_Burnout;
			map[CRC::BurnoutRevenge] = GSC_Burnout;
			map[CRC::BurnoutTakedown] = GSC_Burnout;
			map[CRC::CaptainTsubasa] = GSC_CaptainTsubasa;
			map[CRC::CrashBandicootWoC] = GSC_CrashBandicootWoC;
			map[CRC::DBZBT2] = GSC_DBZBT2;
			map[CRC::DBZBT3] = GSC_DBZBT3;
			map[CRC::DeathByDegreesTekkenNinaWilliams] = GSC_DeathByDegreesTekkenNinaWilliams;
			map[CRC::DevilMayCry3] = GSC_DevilMayCry3;
			map[CRC::EternalPoison] = GSC_EternalPoison;
			map[CRC::EvangelionJo] = GSC_EvangelionJo;
			map[CRC::FFVIIDoC] = GSC_FFVIIDoC;
			map[CRC::FightingBeautyWulong] = GSC_FightingBeautyWulong;
			map[CRC::FinalFightStreetwise] = GSC_FinalFightStreetwise;
			map[CRC::FrontMission5] = GSC_FrontMission5;
			map[CRC::Genji] = GSC_Genji;
			map[CRC::GetaWayBlackMonday] = GSC_GetaWay;
			map[CRC::GetaWay] = GSC_GetaWay;
			map[CRC::GodHand] = GSC_GodHand;
			map[CRC::GT3] = GSC_GT3;
			map[CRC::GT4] = GSC_GT4;
			map[CRC::GTASanAndreas] = GSC_GTASanAndreas;
			map[CRC::GTConcept] = GSC_GTConcept;
			map[CRC::HauntingGround] = GSC_HauntingGround;
			map[CRC::HeavyMetalThunder] = GSC_HeavyMetalThunder;
			map[CRC::HummerBadlands] = GSC_HummerBadlands;
			map[CRC::ICO] = GSC_ICO;
			map[CRC::IkkiTousen] = GSC_IkkiTousen;
			map[CRC::JamesBondEverythingOrNothing] = GSC_JamesBondEverythingOrNothing;
			map[CRC::KnightsOfTheTemple2] = GSC_KnightsOfTheTemple2;
			map[CRC::Kunoichi] = GSC_Kunoichi;
			map[CRC::LordOfTheRingsThirdAge] = GSC_LordOfTheRingsThirdAge;
			map[CRC::Manhunt2] = GSC_Manhunt2;
			map[CRC::MidnightClub3] = GSC_MidnightClub3;
			map[CRC::NanoBreaker] = GSC_NanoBreaker;
			map[CRC::NarutimateAccel] = GSC_NarutimateAccel;
			map[CRC::Naruto] = GSC_Naruto;
			map[CRC::Oneechanbara2Special] = GSC_Oneechanbara2Special;
			map[CRC::Onimusha3] = GSC_Onimusha3;
			map[CRC::RedDeadRevolver] = GSC_RedDeadRevolver;
			map[CRC::ResidentEvil4] = GSC_ResidentEvil4;
			map[CRC::SacredBlaze] = GSC_SacredBlaze;
			map[CRC::SakuraTaisen] = GSC_SakuraTaisen;
			map[CRC::SakuraWarsSoLongMyLove] = GSC_SakuraWarsSoLongMyLove;
			map[CRC::SengokuBasara] = GSC_SengokuBasara;
			map[CRC::ShadowofRome] = GSC_ShadowofRome;
			map[CRC::ShinOnimusha] = GSC_ShinOnimusha;
			map[CRC::Simple2000Vol114] = GSC_Simple2000Vol114;
			map[CRC::SkyGunner] = GSC_SkyGunner;
			map[CRC::SoulCalibur2] = GSC_SoulCalibur2;
			map[CRC::SoulCalibur3] = GSC_SoulCalibur3;
			map[CRC::Spartan] = GSC_Spartan;
			map[CRC::StarWarsBattlefront2] = GSC_StarWarsBattlefront2;
			map[CRC::StarWarsBattlefront] = GSC_StarWarsBattlefront;
			map[CRC::StarWarsForceUnleashed] = GSC_StarWarsForceUnleashed;
			map[CRC::SteambotChronicles] = GSC_SteambotChronicles;
			map[CRC::TalesOfAbyss] = GSC_TalesOfAbyss;
			map[CRC::TalesOfLegendia] = GSC_TalesOfLegendia;
			map[CRC::TalesofSymphonia] = GSC_TalesofSymphonia;
			map[CRC::Tekken5] = GSC_Tekken5;
			map[CRC::TimeSplitters2] = GSC_TimeSplitters2;
			map[CRC::TombRaiderAnniversary] = GSC_TombRaider;
			map[CRC::TombRaiderLegend] = GSC_TombRaiderLegend;
			map[CRC::TombRaiderUnderworld] = GSC_TombRaiderUnderWorld;
			map[CRC::TouristTrophy] = GSC_TouristTrophy;
			map[CRC::UltramanFightingEvolution] = GSC_UltramanFightingEvolution;
			map[CRC::UrbanReign] = GSC_UrbanReign;
			map[CRC::WildArms4] = GSC_WildArms4;
			map[CRC::WildArms5] = GSC_WildArms5;
			map[CRC::Yakuza2] = GSC_Yakuza2;
			map[CRC::Yakuza] = GSC_Yakuza;
			map[CRC::ZettaiZetsumeiToshi2] = GSC_ZettaiZetsumeiToshi2;
			// Only Aggresive
			map[CRC::FFX2] = GSC_FFX2;
			map[CRC::FFX] = GSC_FFX;
			map[CRC::FFXII] = GSC_FFXII;
			map[CRC::SMTDDS1] = GSC_SMTNocturneDDS<0x203BA820>;
			map[CRC::SMTDDS2] = GSC_SMTNocturneDDS<0x20435BF0>;
			map[CRC::SMTNocturne] = GSC_SMTNocturneDDS<0x2054E870>;
			map[CRC::SoTC] = GSC_SoTC;
			map[CRC::SSX3] = GSC_SSX3;
		}

		// Hack that were fixed on openGL
		if (Dx_only) {
			map[CRC::Bully] = GSC_Bully;
			map[CRC::BullyCC] = GSC_BullyCC;
			map[CRC::GodOfWar2] = GSC_GodOfWar2;
			map[CRC::LordOfTheRingsTwoTowers] = GSC_LordOfTheRingsTwoTowers;
			map[CRC::Okami] = GSC_Okami;
			map[CRC::SimpsonsGame] = GSC_SimpsonsGame;
			map[CRC::SuikodenTactics] = GSC_SuikodenTactics;
			map[CRC::XE3] = GSC_XE3;

			// Not tested but must be fixed with texture shuffle
			map[CRC::BigMuthaTruckers] = GSC_BigMuthaTruckers;
			map[CRC::DemonStone] = GSC_DemonStone;
			map[CRC::CrashNburn] = GSC_CrashNburn; // seem to be a basic depth effect
			map[CRC::GiTS] = GSC_GiTS;
			map[CRC::LegoBatman] = GSC_LegoBatman;
			map[CRC::OnePieceGrandAdventure] = GSC_OnePieceGrandAdventure;
			map[CRC::OnePieceGrandBattle] = GSC_OnePieceGrandBattle;
			map[CRC::SFEX3] = GSC_SFEX3;
			map[CRC::SpyroEternalNight] = GSC_SpyroEternalNight;
			map[CRC::SpyroNewBeginning] = GSC_SpyroNewBeginning;
			map[CRC::SonicUnleashed] = GSC_SonicUnleashed;
			map[CRC::TenchuFS] = GSC_Tenchu;
			map[CRC::TenchuWoH] = GSC_Tenchu;

			// Basic Accurate blending + channel effect
			map[CRC::MetalGearSolid3] = GSC_MetalGearSolid3;

			// Those games might requires accurate fbmask
			map[CRC::Sly2] = GSC_Sly2;
			map[CRC::Sly3] = GSC_Sly3;

			// Those games require accurate_colclip (perf)
			map[CRC::CastlevaniaCoD] = GSC_Castlevania;
			map[CRC::CastlevaniaLoI] = GSC_Castlevania;
			map[CRC::GodOfWar] = GSC_GodOfWar;

			// Those games emulate a stencil buffer with the alpha channel of the RT (Slow)
			map[CRC::RadiataStories] = GSC_RadiataStories;
			map[CRC::StarOcean3] = GSC_StarOcean3;
			map[CRC::ValkyrieProfile2] = GSC_ValkyrieProfile2;

			// Deprecated hack could be removed (Cutie)
			map[CRC::Grandia3] = GSC_Grandia3;

			// At least a part of the CRC is fixed with texture shuffle.
			// The status of post-processing effect is unknown
			map[CRC::Black] = GSC_Black;
		}
	}

	// TODO: just set gsc in SetGameCRC once

	GetSkipCount gsc = map[m_game.title];
	g_crc_region = m_game.region;

#ifdef ENABLE_DYNAMIC_CRC_HACK
	bool res=false; if(IsInvokedDynamicCrcHack(fi, skip, g_crc_region, res, m_crc)){ if( !res ) return false;	} else
#endif
	if(gsc && !gsc(fi, skip))
	{
		return false;
	}

	if(skip == 0 && (UserHacks_SkipDraw > 0) )
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = UserHacks_SkipDraw;
			}
		}
	}
#ifdef ENABLE_OGL_DEBUG
	else if (fi.TME) {
			if(fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S)
				GL_INS("!!! Depth Texture 0x%x!!!", fi.TPSM);
	}
#endif

	if(skip > 0)
	{
		skip--;

		return true;
	}

	return false;
}
