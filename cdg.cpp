//***************************************************************************
//
//		File:			cdg.cpp
//
//		Author:			Isaac Brodsky
//
//		Project:		CD+G Deck
//
//		Version:		1.0.4
//
//		Date:			2012 AUGUST 6
//						2012 OCTOBER 16
//						2012 OCTOBER 29
//						2013 DECEMBER 14
//
//		Copyright 2012 Isaac Brodsky. All rights reserved.
//
//		Compact disc subcode graphics decoder, implementation.
//
//		Compact disc subcode graphics (CD + Graphics, CD+G, subcode graphics)
//		are simple graphics programs that can be embedded on audio CDs. Audio
//		CDs contain subcode information interspersed along with the audio
//		tracks. Normally only timing and seek information is stored in the
//		subcodes, but with the extensions to the standard in IEC 60908 (2ed)
//		graphics programs can be stored as well.
//
//		This is a decoder from CD+G subcodes to displayable graphics.
//
//		Information about this format can be found in IEC 60908 2ed.
//		Some (limited) information available online at:
//		http://jbum.com/cdg_revealed.html
//
//***************************************************************************

#include <iostream>						// stream input

using namespace std;

#include "cdg.h"

//***************************************************************************
// Constructs a blank CDG decoder, with all colors being {00, 00, 00}, all
// pixels set to the 0th color, and the PV and PH pointers at 0.
//***************************************************************************
CDG::CDG()
{
	for (int i = 0; i < CDG_NUM_COLORS; i++)
		for (int channel = 0; channel < CDG_NUM_COLOR_CHANNELS; channel++)
			colorTable[i][channel] = 0x00;

	for (int x = 0; x < CDG_WIDTH; x++)
		for (int y = 0; y < CDG_HEIGHT; y++)
			putPixel(x, y, 0);

	ph = pv = 0;
	border = 0;
}

//***************************************************************************
// Returns true if the state of the other CDG decoder is the same as this
// one. State includes graphic memory, PV and PH pointers, and the color
// table. State does not include how many subcode packets have been
// processed or any detail of how the state was changed.
//***************************************************************************
bool CDG::operator==(const CDG& other) const
{
	bool equal = true;

	for (int i = 0; i < CDG_NUM_COLORS; i++)
	{
		for (int channel = 0; channel < CDG_NUM_COLOR_CHANNELS; channel++)
		{
			if (colorTable[i][channel] != other.colorTable[i][channel])
			{
				equal = false;
				break;
			}
		}
		if (!equal)
			break;
	}

	equal &= (ph == other.ph);			// Don't bother testing if (equal)
	equal &= (pv == other.pv);			// it's only a few compares
	equal &= (border == other.border);	// Do it before checking the
										// screen array since that's
	if (equal)							// a heavier operation and if
	{									// we can skip it we should.
		for (int x = 0; x < CDG_WIDTH; x++)
		{
			for (int y = 0; y < CDG_HEIGHT; y++)
			{
				if (getPixel(x, y) != other.getPixel(x, y))
				{
					equal = false;
					break;
				}
			}
			if (!equal)
				break;
		}
	}

	return equal;
}

//***************************************************************************
// Reads the next SubCode structure in from the given input stream.
// 
// Returns true if the subcode was read successfully, false if any error
// occured (e.g. EOF)
//***************************************************************************
bool CDG::readNext(istream &in, SubCode &out)
{
	bool success = true;

	out.command = SubCode_Command::SCCMD_NONE;
	
	in.read((char*)&out, sizeof(SubCode));		// attempt read
	if (!in.good())								// the read failed
	{
		success = false;
	}
	
	return success;
}

//***************************************************************************
// Internal implementation of the LOADCLUT command. This function accepts
// a color index and a packed color (such as one from the data field
// of a LOADCLUT command.)
//
// The color is unpacked and stored in the given index in the color table.
//***************************************************************************
void CDG::loadColor(int idx, short col)
{
								// jbum and the spec both have nice
								// charts of the layout of col.
	uint8_t r, g, b, high, low;

	high = col & LOWER_6_BITS;
	low = (col >> 8) & LOWER_6_BITS;
	
	r = (high >> 2);
	g = (((high & LOWER_2_BITS) << 2) | (low >> 4));
	b = (low & LOWER_4_BITS);

	r = r << 4;
	g = g << 4;
	b = b << 4;

	if (idx >= 0 && idx < CDG_NUM_COLORS)
	{
		colorTable[idx][0] = r;
		colorTable[idx][1] = g;
		colorTable[idx][2] = b;	//don't change 3 - alpha
								// the standard doesn't say anything
	}							// about that.
}

//***************************************************************************
// Executes the given LOADCLUT command on this CDG decoder.
//
// LOADCLUT commands change a given half of the color palette.
//***************************************************************************
void CDG::execLoadct(const SubCode &subCode) 
{
	int offset = ((subCode.instruction & LOWER_6_BITS) == CDG_LOADCTHIGH) ? 8 : 0;

	for (int i = 0; i < 8; i++)
		loadColor(i + offset, subCode.data.clutDat.colorSpec[i]);
}

//***************************************************************************
// Executes the given TRANSPARENT command on this CDG decoder.
//
// TRANSPARENT commands set the alpha channel of all colors in the palette
// to the level given.
//***************************************************************************
void CDG::execTransparent(const SubCode &subCode)
{
	for (int i = 0; i < 16; i++)
	{
		colorTable[i][3] = 
			((subCode.data.transparentDat.alphaChannel[i] & LOWER_6_BITS)
			<< 2);
	}
}

//***************************************************************************
// Fills the pixels from (xs, ys) to (xe, ye) with the given color.
// Inclusive of (xs, ys) and exclusive of column xe and row ye.
//***************************************************************************
void CDG::fillPixels(int xs, int ys, int xe, int ye, uint8_t color)
{
	for (int x = xs; x < xe; x++)
	{
		for (int y = ys; y < ye; y++)
		{
			putPixel(x, y, color);
		}
	}
}

//***************************************************************************
// Sets the pixel at (x, y) to the given color, where color is an index
// to the color table.
//
// If xor is true, the new color is XORed with the previous value at the
// given location. This means the new index is XORed with the old index.
//***************************************************************************
void CDG::putPixel(int x, int y, uint8_t color, bool xor)
{
	if (x < CDG_WIDTH && x >= 0
		&& y < CDG_HEIGHT && y >= 0)
	{
#ifdef SHRINK_CDG
		//determine new color
		uint8_t newcolor;
		if (xor)
		{
			newcolor = getPixel(x, y);
			newcolor ^= color;
		}
		else
		{
			newcolor = color;
		}

		//commit new color to screen
		if (y % 2 == 1)
			screen[x][y / 2] = (screen[x][y / 2] & LOWER_4_BITS) | (newcolor << 4);
		else
			screen[x][y / 2] = (screen[x][y / 2] & ~LOWER_4_BITS) | newcolor;
#else
		if (xor)
			screen[x][y] ^= color;
		else
			screen[x][y] = color;
#endif
	}
}

//***************************************************************************
// Retrieves the 32 bit (8 bit each RGBA) representation of the given color
// code.
//
// All outputs are set to 0 if the given code is invalid.
//***************************************************************************
void CDG::getColor(uint8_t code, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const
{
	if (code >= CDG_NUM_COLORS || code < 0)
	{
		r = g = b = a = 0;
	}
	else
	{
		r = colorTable[code][0];
		g = colorTable[code][1];
		b = colorTable[code][2];
		a = colorTable[code][3];
	}
}

//***************************************************************************
// Returns the color code (the index to the color table) stored at the given
// location, or 0 if the location is out of bounds.
//***************************************************************************
uint8_t CDG::getPixel(int x, int y) const
{
	uint8_t ret;

	if (x < CDG_WIDTH && x >= 0
		&& y < CDG_HEIGHT && y >= 0)
	{
#ifdef SHRINK_CDG
		if (y % 2 == 1)
			ret = screen[x][y / 2] >> 4;
		else
			ret = screen[x][y / 2] & LOWER_4_BITS;
#else
		ret = screen[x][y];
#endif
	}
	else
	{
		ret = 0;
	}

	return ret;
}

//***************************************************************************
// Retrieves the PV and PH values of this decoder. These are the offsets
// renderers should offset the data by (to the left for H or up for V.)
// 
// So with a PH of 1, the pixel at (100, 100) would be displayed instead at
// (99, 100)
//***************************************************************************
void CDG::getPointers(uint8_t &v, uint8_t &h) const
{
	v = pv;
	h = ph;
}

//***************************************************************************
// Returns the index of the color to be used in masking the border area
// of the display.
//***************************************************************************
uint8_t CDG::getBorderColor() const
{
	return border;
}

//***************************************************************************
// Executes the given MEMORYPRESET command on this CDG decoder.
//
// MEMORYPRESET commands clear the screen (and the PV/PH offsets.) The screen
// is filled with the given color. The specification has a repeat field
// which should be used to prevent contiously clearing the screen, that
// field is ignored in this implementation.
//***************************************************************************
void CDG::execMemoryPreset(const SubCode &subCode)
{
	int color = subCode.data.memDat.color & LOWER_4_BITS;

	fillPixels(0, 0, CDG_WIDTH, CDG_HEIGHT, color);
	ph = pv = 0;
}

//***************************************************************************
// Executes the given BORDERPRESET command on this CDG decoder.
//
// Sets the color to be used in masking the border area.
//***************************************************************************
void CDG::execBorderPreset(const SubCode &subCode)
{
	// I'm not entirely clear on the spec. for BORDERPRESET.
        // What exactly the "border" part it should clear isn't, well, clear.
	// I understand vintage decoders had a "border area" which displayed
        // just a solid color.
	//
	// VLC implements it by cleaing that part of graphics RAM, and the
	// jbum.com document is ambiguous as well.

		//BORDERPRESET
		//6,12,294,204
		
		//WRONG - do not use
		//fillPixels(0, 0, CDG_WIDTH, ROW_MULT, color);
		//fillPixels(0, CDG_HEIGHT - ROW_MULT, CDG_WIDTH, CDG_HEIGHT, color);
		//fillPixels(0, 0, COL_MULT, CDG_HEIGHT, color);
		//fillPixels(CDG_WIDTH - COL_MULT, 0, CDG_WIDTH, CDG_HEIGHT, color);

	// Jimi Hendrix - Smash Hits proves that the code for BORDPRESET
	// above is wrong. The disk does not play properly with that code.

	border = subCode.data.borderDat.color & LOWER_4_BITS;
}

//***************************************************************************
// Executes the given TILE or TILEXOR command on this CDG decoder.
//
// TILE (or FONT) commands are used to draw a block of data to the screen.
//
// Note: the spec. has a "channel" field packed into the TILE data,
// most disks do not use this and I do not have a disk to test this feature
// with or a reference decoder that supports it, so I have not implemented
// it.
//***************************************************************************
void CDG::execTile(const SubCode &subCode)
{
	uint8_t color[2];
	uint8_t channel;			// I believe Lou Reed's "New York"
							// uses channels but I do not
							// have this disk to test this
							// feature.
	bool useXor = ((subCode.instruction & LOWER_6_BITS) == CDG_TILEBLOCKXOR);
	int point;

	int row = (subCode.data.tileDat.row & LOWER_5_BITS) * ROW_MULT;
	int col = (subCode.data.tileDat.column & LOWER_6_BITS) * COL_MULT;

	color[0] = subCode.data.tileDat.color0 & LOWER_4_BITS;
	color[1] = subCode.data.tileDat.color1 & LOWER_4_BITS;
	
	channel = (subCode.data.tileDat.color0) >> 4;
	channel = (channel << 2) | (subCode.data.tileDat.color1 >> 4);

	if (row < CDG_HEIGHT || col < CDG_WIDTH)	//not offscreen
	{
		for (int y = 0; y < ROW_MULT; y++)
		{
			for (int x = 0; x < COL_MULT; x++)
			{
				point = (subCode.data.tileDat.tilePixels[y] >> (5 - x))
							& 1;
				
				putPixel((col+x), (row+y), color[point], useXor);
			}
		}
	}
}

//***************************************************************************
// Swaps the given pixels at (x1, y1) and (x2, y2)
//***************************************************************************
void CDG::swapPixels(int x1, int y1, int x2, int y2)
{
	uint8_t p1 = getPixel(x1, y1);
	uint8_t p2 = getPixel(x2, y2);
	putPixel(x2, y2, p1);
	putPixel(x1, y1, p2);
}

//***************************************************************************
// Internal implementation of SCROLL commands. Accepts the COPV part of
// the SCROLL data field and shifts the graphics array appropriately.
//***************************************************************************
void CDG::rotateV(int cmd)
{
	//For the std algorithm, see
	//http://www.cplusplus.com/reference/algorithm/rotate/
	//(not used here)
	int next;

	if (cmd == 2)
	{
		for (int y = 0; y < CDG_HEIGHT - ROW_MULT; y++)
		{
			next = y - ROW_MULT;
			if (next < 0)
				next += CDG_HEIGHT;

			for (int x = 0; x < CDG_WIDTH; x++)
				swapPixels(x, y, x, next);
		}
	}
	else if (cmd == 1)
	{
		for (int y = CDG_HEIGHT - 1; y >= ROW_MULT; y--)
		{
			next = y + ROW_MULT;
			if (next >= CDG_HEIGHT)
				next -= CDG_HEIGHT;

			for (int x = 0; x < CDG_WIDTH; x++)
				swapPixels(x, y, x, next);
		}
	}
}

//***************************************************************************
// Internal implementation of SCROLL commands. Accepts the COPH part of
// the SCROLL data field and shifts the graphics array appropriately.
//***************************************************************************
void CDG::rotateH(int cmd)
{
	//For the std algorithm, see
	//http://www.cplusplus.com/reference/algorithm/rotate/
	//(not used here)
	int next;

	if (cmd == 2)
	{
		for (int x = 0; x < CDG_WIDTH - COL_MULT; x++)
		{
			next = x - COL_MULT;
			if (next < 0)
				next += CDG_WIDTH;

			for (int y = 0; y < CDG_HEIGHT; y++)
				swapPixels(x, y, next, y);
		}
	}
	else if (cmd == 1)
	{
		for (int x = CDG_WIDTH - 1; x >= COL_MULT; x--)
		{
			next = x + COL_MULT;
			if (next >= CDG_WIDTH)
				next -= CDG_WIDTH;

			for (int y = 0; y < CDG_HEIGHT; y++)
				swapPixels(x, y, next, y);
		}
	}
}

//***************************************************************************
// Executes the given SCROLLPRESET or SCROLLCOPY command on this CDG decoder.
//
// SCROLL commands can be used to simulate animation, by panning new
// graphics into view in a relatively smooth manner. This uses the PH and PV
// pointers (accessible through getPointers(int&, int&)) the offset where
// the screen will be rendered to.
//
// SCROLL commands can also trigger larger "copy" shift operations which
// shift the entire screen contents.
//***************************************************************************
void CDG::execScroll(const SubCode &subCode)
{
	uint8_t color = subCode.data.scrollDat.color & LOWER_4_BITS;
	uint8_t scrollH = subCode.data.scrollDat.hScroll & LOWER_6_BITS,
		scrollV = subCode.data.scrollDat.vScroll & LOWER_6_BITS;
	uint8_t cmdH = (scrollH & 0x30) >> 4;
	uint8_t offsetH = (scrollH & 0x07);
	uint8_t cmdV = (scrollV & 0x30) >> 4;
	uint8_t offsetV = (scrollV & 0x0F);

	ph = offsetH;
	pv = offsetV;

	if (cmdH)
		rotateH(cmdH);
	if (cmdV)
		rotateV(cmdV);

	if ((subCode.instruction & LOWER_6_BITS) == CDG_SCROLLPRESET)
	{
		if (cmdH == 1)
			fillPixels(0, 0, COL_MULT, CDG_HEIGHT, color);
		if (cmdH == 2)
			fillPixels(CDG_WIDTH - COL_MULT, 0, COL_MULT, CDG_HEIGHT, color);
		if (cmdV == 1)
			fillPixels(0, 0, CDG_WIDTH, ROW_MULT, color);
		if (cmdV == 2)
			fillPixels(0, CDG_HEIGHT - ROW_MULT, CDG_WIDTH, ROW_MULT, color);
	}									// else CDG_SCROLLCOPY, which has
										// no extra steps.
}

//***************************************************************************
// Reads and executes the given number of subcodes from the input stream.
// Dirty is incremented by the number of CDG commands that have been
// executed.
//***************************************************************************
int CDG::execCount(istream &in, int count, int &dirty)
{
	bool success = true;
	SubCode code;

	for (int i = 0; i < count; i++)
	{
		if (readNext(in, code))
		{
			execNext(code, dirty);
		}
		else
		{
			success = false;
			break;
		}
	}

	return success;
}

//***************************************************************************
// Wraps to execNext but discards the dirty output parameter.
//***************************************************************************
void CDG::execNext(const SubCode &subCode)
{
	int i = 0;
	execNext(subCode, i);
}

//***************************************************************************
// Executes the given subcode. If the subcode was a CDG command
// the parameter dirty is incremented.
//***************************************************************************
void CDG::execNext(const SubCode &subCode, int &dirty)
{
	//seems like a nice place to use function pointers
	//but let's not do that
	if ((subCode.command & LOWER_6_BITS) == SubCode_Command::SCCMD_CDG)
	{
		switch (subCode.instruction & LOWER_6_BITS)
		{
		case CDG_MEMORYPRESET:
			execMemoryPreset(subCode);
			break;
		case CDG_BORDERPRESET:
			execBorderPreset(subCode);
			break;
		case CDG_TILEBLOCK:
			execTile(subCode);
			break;
		case CDG_SCROLLPRESET:
			execScroll(subCode);
			break;
		case CDG_SCROLLCOPY:
			execScroll(subCode);
			break;
		case CDG_TRANSPARENT:
			execTransparent(subCode);
			break;
		case CDG_LOADCTLOW:
			execLoadct(subCode);
			break;
		case CDG_LOADCTHIGH:
			execLoadct(subCode);
			break;
		case CDG_TILEBLOCKXOR:
			execTile(subCode);
			break;
		default:
								// unknown
			break;
		}
		dirty++;
	}
}

//***************************************************************************
// Has this decoder seek through the given input stream (using the given
// SeekMode) to the byte location loc.
//***************************************************************************
void CDG::seekTo(istream &in, int loc, SeekMode mode)
{
	int dirty = 0;
	switch (mode)
	{
	case SEEK_ENHANCED:
		in.seekg(0, ios::beg);
		pv = ph = 0;			//reset
		execCount(in, (loc / sizeof(SubCode)), dirty);
		break;
	case SEEK_DIRECT:
	default:
		in.seekg(loc, ios::beg);
		break;
	}
}

//***************************************************************************
// Calculates the length, in seconds, of the given number of bytes of CDG
// data.
//***************************************************************************
int CDG::sizeToSeconds(int f)
{
	f /= 96; // 96 bytes per sector
	f /= 75; // 75 sectors per second
	return f;
}

//***************************************************************************
// Converts the given percent of total (where total is the size in bytes of
// a stream of CDG data) to a seconds position in time.
//***************************************************************************
int CDG::percentToSecond(double percent, int total)
{
	int max = int(percent * total);
	int ret = 0;

	while (ret < max)
	{
		ret += BYTES_PER_SECOND;
	}

	return ret - BYTES_PER_SECOND;
}
