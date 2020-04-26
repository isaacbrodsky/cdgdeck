//***************************************************************************
//
//		File:			cdg.h
//
//		Author:			Isaac Brodsky
//
//		Project:		CD+G Deck
//
//		Version:		1.0.3
//
//		Date:			2012 AUGUST 6
//						2012 OCTOBER 16
//						2012 OCTOBER 29
//
//		COPYRIGHT 2012 Isaac Brodsky ALL RIGHTS RESERVED.
//
//		Compact disc subcode graphics decoder, definition.
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

#ifndef CDG_H
#define CDG_H

#include <iostream>

using namespace std;

#include <stdint.h>

//***************************************************************************
// Constants and compilation options.
//***************************************************************************

#define SHRINK_CDG					// use space saving memory layout.
									// MSVC seems to have trouble
									// without this. (but only sometimes?)

#define BYTES_PER_SECOND (96 * 75)	// number of bytes of subcode information
									// per second of audio.

#define CDG_NUM_COLORS 16
#define CDG_NUM_COLOR_CHANNELS 4

#define CDG_WIDTH 300				// size in pixels
#define CDG_HEIGHT 216				//

#define ROW_MULT 12					// Size of each discrete block ("font")
#define COL_MULT 6					// of CDG data.
									// Bit masks
#define LOWER_6_BITS 0x3F			// 63
#define LOWER_5_BITS 0x1F			// (LOWER_6_BITS >> 1)
#define LOWER_4_BITS 0x0F			// (LOWER_6_BITS >> 2)
#define LOWER_3_BITS 0x07			// (LOWER_6_BITS >> 3)
#define LOWER_2_BITS 0x03			// (LOWER_6_BITS >> 4)

//***************************************************************************
// Represents the mode a seek operation will use.
//
// S_ENHANCED will cause the seek system to process the file up to the seek
// location. This will result in technically correct seeking but is slower.
//
// S_DIRECT is faster and does not involve processing the file up to the
// given point (VLC uses this type of seeking.) The read pointer is simply
// adjusted to the new location. This may result in some temporary graphical
// corruption.
//***************************************************************************
enum SeekMode
{
	SEEK_DIRECT,
	SEEK_ENHANCED
};

//***************************************************************************
// Represents possible Subcode commands that this program can process
//
// C++11 features being used here
// http://en.wikipedia.org/wiki/C%2B%2B0x#Strongly_typed_enumerations
// would like to use "enum class" but not supported
//***************************************************************************
enum SubCode_Command : char
{
	SCCMD_NONE			= 0x00,
	SCCMD_CDG			= 0x09
};

//***************************************************************************
// This type represents possible CDG command codes.
//***************************************************************************
enum CDG_Instruction : char
{
	CDG_MEMORYPRESET	=  1,
	CDG_BORDERPRESET	=  2,
	CDG_TILEBLOCK		=  6,
	CDG_SCROLLPRESET	= 20,
	CDG_SCROLLCOPY		= 24,
	CDG_TRANSPARENT		= 28,
	CDG_LOADCTLOW		= 30,
	CDG_LOADCTHIGH		= 31,
	CDG_TILEBLOCKXOR	= 38
};

#pragma pack(push, 1)				//disable packing since we're reading these
									//structs in from disk.

//***************************************************************************
// Structure of the data field in MEMORYPRESET CDG commands.
//***************************************************************************
struct CDG_MemPreset
{
	char	color;
	char	repeat;					// ignored in this implementation
	char	padding[14];
};

//***************************************************************************
// Structure of the data field in BORDERPRESET CDG commands.
//***************************************************************************
struct CDG_BorderPreset
{
	char	color;
	char	padding[15];
};

//***************************************************************************
// Structure of the data field in FONT, and FONTXOR CDG commands.
//
// Semantics note: I use the terms TILE and FONT commands interchangably.
// Other sources online (probably as a result of the document on jbum.com)
// call these commands "TILE" commands, the spec says "FONT."
//***************************************************************************
struct CDG_Tile
{
	char	color0;					// spec also stores the channel number
	char	color1;					// in these two bytes
	char	row;
	char	column;
	char	tilePixels[12];
};

//***************************************************************************
// Structure of the data field in SCROLLCOPY and SCROLLPRESET CDG commands.
//***************************************************************************
struct CDG_Scroll
{
	char	color;
	char	hScroll;				// these contain additional data
	char	vScroll;				// embedded in the higher bits
	char	padding[13];
};

//***************************************************************************
// Structure of the data field in TRANSPARENCY CDG commands.
//
// The transparency feature seems to be a little used one, I've never seen
// a disk with it, and the spec doesn't mention if transparency should be
// reset when LOADCLUT commands are executed.
//
// (The lack of nicer typesetting for the TRANSPARENT diagram in the spec
// is another indicator that this is a little cared for feature.)
//***************************************************************************
struct CDG_Transparent
{
	char	alphaChannel[16];
};

//***************************************************************************
// Structure of the data field in LOADCLUTHIGH and LOADCLUTLOW CDG commands.
//***************************************************************************
struct CDG_LoadCLUT
{
	short colorSpec[8];
};

//***************************************************************************
// Structure of a single CDG command. These are stored four in a packet on
// disk. This subcode data is stored in the R-W subcode channels of digital
// audio CDs.
//
// Only the lower six bits of any byte in this structure should be used, the
// high two bits (if not stripped and zeroed) are the P and Q subcode
// channels which are used for other purposes.
// 
// Application should use the appropriate part of the data union based on
// the command and instruction fields.
//***************************************************************************
struct SubCode
{
	SubCode_Command		command;
	CDG_Instruction		instruction;
	char				parityQ[2];
	union
	{
		char				data[16];
		CDG_MemPreset		memDat;
		CDG_BorderPreset	borderDat;
		CDG_Tile			tileDat;
		CDG_Scroll			scrollDat;
		CDG_Transparent		transparentDat;
		CDG_LoadCLUT		clutDat;
	} data;
	char				parityP[4];
}; //1 pack, 4 PACKs per PACKET

#pragma pack(pop)					// allow default for the classes;
									// compiler knows what it's doing.

//***************************************************************************
// Represents a CD+G decoder.
//***************************************************************************
class CDG
{
private:
	uint8_t colorTable[CDG_NUM_COLORS][CDG_NUM_COLOR_CHANNELS]; //rgba
#ifdef SHRINK_CDG
	uint8_t screen[CDG_WIDTH][CDG_HEIGHT/2];	// use 4 bits per byte per pixel
#else
#error "Non shrunken CDG not a supported feature."
	uint8_t screen[CDG_WIDTH][CDG_HEIGHT];	// use all 8 bits per pixel, even though
											// each only takes 4
#endif

	uint8_t ph, pv;
	uint8_t border;

	//private functions
	static bool readNext(istream &in, SubCode &out);
	
	void swapPixels(int x1, int y1, int x2, int y2);
	void rotateV(int cmd);
	void rotateH(int cmd);
	void loadColor(int idx, short col);

	void fillPixels(int x1, int y1, int x2, int y2, uint8_t color);
	void putPixel(int x, int y, uint8_t color, bool xor = false);
	
	void execLoadct(const SubCode&);
	void execMemoryPreset(const SubCode&);
	void execBorderPreset(const SubCode&);
	void execScroll(const SubCode&);
	void execTransparent(const SubCode&);
	void execTile(const SubCode&);
public:

	CDG();

	bool operator==(const CDG&) const;
	
	void getColor(uint8_t code, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const;
	uint8_t getPixel(int x, int y) const;
	void getPointers(uint8_t &v, uint8_t &h) const;
	uint8_t getBorderColor() const;

	void execNext(const SubCode&);
	void execNext(const SubCode&, int&);
	int execCount(istream &in, int count, int &dirty);
	void seekTo(istream &in, int loc, SeekMode mode);
	
	static int sizeToSeconds(int sz);
	static int percentToSecond(double percent, int total);
};

#endif