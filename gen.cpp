#include "cdg.h"
#include <fstream>
#include <vector>
#include "gen.h"

using namespace std;

short packColor(uint8_t r, uint8_t g, uint8_t b);
void pushEmpty(vector<SubCode>& t, int repeat = 1);

void renderBob(vector<SubCode>& track);

//generates test pattern to out.cdg
int genmain()
{
	vector<SubCode> track;

	//color table
	SubCode ct0, ct1, tr;
	ct0.command = ct1.command = tr.command = SCCMD_CDG;
	ct0.instruction = CDG_Instruction::CDG_LOADCTLOW;
	ct1.instruction = CDG_Instruction::CDG_LOADCTHIGH;
	tr.instruction = CDG_Instruction::CDG_TRANSPARENT;
	
#define L0 0
#define L1 1
#define L2 ((L1 << 1) | L1)
#define L3 ((L2 << 1) | L2)
#define L4 ((L3 << 1) | L3)
	
	ct0.data.clutDat.colorSpec[0] = packColor(L0, L0, L0);
	ct0.data.clutDat.colorSpec[1] = packColor(L1, L1, L1);
	ct0.data.clutDat.colorSpec[2] = packColor(L2, L2, L2);
	ct0.data.clutDat.colorSpec[3] = packColor(L3, L3, L3);
	ct0.data.clutDat.colorSpec[4] = packColor(L4, L4, L4);
	ct0.data.clutDat.colorSpec[5] = packColor(L4, L0, L0);
	ct0.data.clutDat.colorSpec[6] = packColor(L0, L4, L0);
	ct0.data.clutDat.colorSpec[7] = packColor(L0, L0, L4);
	ct1.data.clutDat.colorSpec[0] = packColor(L0, L4, L4);
	ct1.data.clutDat.colorSpec[1] = packColor(L4, L4, L0);
	ct1.data.clutDat.colorSpec[2] = packColor(L4, L0, L4);
	ct1.data.clutDat.colorSpec[3] = packColor(L1, L0, L0);
	ct1.data.clutDat.colorSpec[4] = packColor(L2, L0, L0);
	ct1.data.clutDat.colorSpec[5] = packColor(L3, L0, L0);
	ct1.data.clutDat.colorSpec[6] = packColor(L0, L1, L0);
	ct1.data.clutDat.colorSpec[7] = packColor(L0, L2, L0);
	int ttemp = LOWER_6_BITS;
	tr.data.transparentDat.alphaChannel[0] = ttemp--;
	tr.data.transparentDat.alphaChannel[1] = ttemp--;
	tr.data.transparentDat.alphaChannel[2] = ttemp--;
	tr.data.transparentDat.alphaChannel[3] = ttemp--;
	ttemp = LOWER_4_BITS;
	tr.data.transparentDat.alphaChannel[4] = ttemp--;
	tr.data.transparentDat.alphaChannel[5] = ttemp--;
	tr.data.transparentDat.alphaChannel[6] = ttemp--;
	tr.data.transparentDat.alphaChannel[7] = ttemp--;
	ttemp = LOWER_2_BITS;
	tr.data.transparentDat.alphaChannel[8] = ttemp--;
	tr.data.transparentDat.alphaChannel[9] = ttemp--;
	tr.data.transparentDat.alphaChannel[10] = ttemp--;
	tr.data.transparentDat.alphaChannel[11] = ttemp--;
	tr.data.transparentDat.alphaChannel[12] = ttemp--;
	tr.data.transparentDat.alphaChannel[13] = ttemp--;
	tr.data.transparentDat.alphaChannel[14] = ttemp--;
	tr.data.transparentDat.alphaChannel[15] = ttemp--;

	//commit ct0 ct1
	track.push_back(ct0);
	track.push_back(ct1);
	pushEmpty(track, 2);
	pushEmpty(track, 160);
	
	for (int x = 0; x < CDG_WIDTH / COL_MULT; x++)
	{
		for (int y = 0; y < CDG_HEIGHT / ROW_MULT; y++)
		{
			SubCode p;
			p.command = SCCMD_CDG;
			p.instruction = CDG_Instruction::CDG_TILEBLOCK;
			uint8_t col = x + y;
			while (col > 15)
				col -= 16;
			p.data.tileDat.color0 = col;
			p.data.tileDat.row = y;
			p.data.tileDat.column = x;
			for (int i = 0; i < 12; i++)
				p.data.tileDat.tilePixels[i] = 0;
			
			//commit p
			track.push_back(p);
			pushEmpty(track, 3);
		}
	}
	
	for (int i = 0; i < CDG_WIDTH / COL_MULT; i++)
	{
		SubCode x;
		x.command = SCCMD_CDG;
		x.instruction = CDG_Instruction::CDG_TILEBLOCKXOR;
		x.data.tileDat.color0 = 6;
		x.data.tileDat.color1 = 9;
		x.data.tileDat.row = i % (CDG_HEIGHT / ROW_MULT);
		x.data.tileDat.column = i;
		for (int j = 0; j < 12; j++)
			x.data.tileDat.tilePixels[j] = ((0xFF << j) >> i);

		track.push_back(x);
		pushEmpty(track, 3);
	}
	
	//others
	for (int i = 0; i < COL_MULT; i++)
	{
		SubCode s;
		s.command = SCCMD_CDG;
		s.instruction = CDG_Instruction::CDG_SCROLLCOPY;
		s.data.scrollDat.hScroll = i;
		s.data.scrollDat.vScroll = 0;
		track.push_back(s);
		pushEmpty(track, 3 + (4 * 16));
	}
	SubCode s2;
	s2.command = SCCMD_CDG;
	s2.instruction = CDG_Instruction::CDG_SCROLLCOPY;
	s2.data.scrollDat.hScroll = 2 << 4;
	s2.data.scrollDat.vScroll = 0;
	track.push_back(s2);
	
	pushEmpty(track, 320);

	pushEmpty(track, 3 + 4);
	for (int i = 0; i < ROW_MULT; i++)
	{
		SubCode s;
		s.command = SCCMD_CDG;
		s.instruction = CDG_Instruction::CDG_SCROLLCOPY;
		s.data.scrollDat.vScroll = i;
		s.data.scrollDat.hScroll = 0;
		track.push_back(s);
		pushEmpty(track, 3 + (4 * 16));
	}
	SubCode s3;
	s3.command = SCCMD_CDG;
	s3.instruction = CDG_Instruction::CDG_SCROLLCOPY;
	s3.data.scrollDat.vScroll = 2 << 4;
	s3.data.scrollDat.hScroll = 0;
	track.push_back(s3);

	pushEmpty(track, 320);
	
	
	SubCode ct2, ct3;
	ct2.command = ct3.command = SCCMD_CDG;
	ct2.instruction = CDG_Instruction::CDG_LOADCTLOW;
	ct3.instruction = CDG_Instruction::CDG_LOADCTHIGH;
	
	ct2.data.clutDat.colorSpec[0] = packColor(L0, L0, L0);
	ct2.data.clutDat.colorSpec[1] = packColor(L4, L4, L0);
	ct2.data.clutDat.colorSpec[2] = packColor(L3, L3, L0);
	ct2.data.clutDat.colorSpec[3] = packColor(L2, L2, L0);
	ct2.data.clutDat.colorSpec[4] = packColor(L1, L1, L0);
	ct2.data.clutDat.colorSpec[5] = packColor(L0, L4, L4);
	ct2.data.clutDat.colorSpec[6] = packColor(L0, L3, L3);
	ct2.data.clutDat.colorSpec[7] = packColor(L0, L2, L2);
	ct3.data.clutDat.colorSpec[0] = packColor(L0, L1, L1);
	ct3.data.clutDat.colorSpec[1] = packColor(L4, L0, L4);
	ct3.data.clutDat.colorSpec[2] = packColor(L3, L0, L3);
	ct3.data.clutDat.colorSpec[3] = packColor(L2, L0, L2);
	ct3.data.clutDat.colorSpec[4] = packColor(L1, L0, L1);
	ct3.data.clutDat.colorSpec[5] = packColor(L4, L4, L4);
	ct3.data.clutDat.colorSpec[6] = packColor(L4, L3, L2);
	ct3.data.clutDat.colorSpec[7] = packColor(L2, L3, L4);
	
	track.push_back(ct2);
	track.push_back(ct3);
	pushEmpty(track, 320 + 2);
	
	SubCode s4;
	s4.command = SCCMD_CDG;
	s4.instruction = CDG_Instruction::CDG_SCROLLPRESET;
	s4.data.scrollDat.color = 1;
	s4.data.scrollDat.hScroll = 1 << 4;
	s4.data.scrollDat.vScroll = 0;
	SubCode s5;
	s5.command = SCCMD_CDG;
	s5.instruction = CDG_Instruction::CDG_SCROLLPRESET;
	s5.data.scrollDat.color = 2;
	s5.data.scrollDat.hScroll = 0;
	s5.data.scrollDat.vScroll = 1 << 4;
	track.push_back(s4);
	track.push_back(s5);
	pushEmpty(track, 320 + 2);

	for (int x = 0; x < 3; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			SubCode p;
			p.command = SCCMD_CDG;
			p.instruction = CDG_Instruction::CDG_TILEBLOCK;
			uint8_t col = 1 + (x * 4) + y;
			p.data.tileDat.color0 = col;
			p.data.tileDat.row = y + 1;
			p.data.tileDat.column = x + 1;
			for (int i = 0; i < 12; i++)
				p.data.tileDat.tilePixels[i] = 0;
			
			//commit p
			track.push_back(p);
			pushEmpty(track, 3);
		}
	}
	
	pushEmpty(track, 320);

	SubCode m;
	m.command = SCCMD_CDG;
	m.instruction = CDG_Instruction::CDG_MEMORYPRESET;
	m.data.memDat.color = 0;
	m.data.memDat.repeat = 0;

	track.push_back(m);
	pushEmpty(track, 3);

	for (int x = 0; x < 3; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			SubCode p;
			p.command = SCCMD_CDG;
			p.instruction = CDG_Instruction::CDG_TILEBLOCK;
			uint8_t col = 1 + (x * 4) + y;
			p.data.tileDat.color0 = col;
			p.data.tileDat.row = y + 1;
			p.data.tileDat.column = x + 4;
			for (int i = 0; i < 12; i++)
				p.data.tileDat.tilePixels[i] = 0;
			
			//commit p
			track.push_back(p);
			pushEmpty(track, 3);
		}
	}

	renderBob(track);

	pushEmpty(track, 320);
	SubCode ct4, ct5;
	ct4.command = ct5.command = SCCMD_CDG;
	ct4.instruction = CDG_Instruction::CDG_LOADCTLOW;
	ct5.instruction = CDG_Instruction::CDG_LOADCTHIGH;
	
	for (int i = 0; i < 8; i++)
	{
		ct4.data.clutDat.colorSpec[i] = packColor(i, 0, 0);
		ct5.data.clutDat.colorSpec[i] = packColor(i + 8, 0, 0);
	}
	
	track.push_back(ct4);
	track.push_back(ct5);
	track.push_back(m);
	pushEmpty(track);

	for (int i = 0; i < 16; i++)
	{
		SubCode x;
		x.command = SCCMD_CDG;
		x.instruction = CDG_Instruction::CDG_TILEBLOCKXOR;
		x.data.tileDat.color0 = 0;
		x.data.tileDat.color1 = i;
		x.data.tileDat.row = 6;
		x.data.tileDat.column = i + 4;
		for (int j = 0; j < 12; j++)
			x.data.tileDat.tilePixels[j] = char(int(int(0xFF) << j) >> i);

		track.push_back(x);
		pushEmpty(track, 3);
	}
	
	pushEmpty(track, 320);

	for (int i = 0; i < 8; i++)
	{
		ct4.data.clutDat.colorSpec[i] = packColor(0, i, 0);
		ct5.data.clutDat.colorSpec[i] = packColor(0, i + 8, 0);
	}
	
	track.push_back(ct4);
	track.push_back(ct5);
	pushEmpty(track, 320);
	
	for (int i = 0; i < 8; i++)
	{
		ct4.data.clutDat.colorSpec[i] = packColor(0, 0, i);
		ct5.data.clutDat.colorSpec[i] = packColor(0, 0, i + 8);
	}
	
	track.push_back(ct4);
	track.push_back(ct5);
	pushEmpty(track, 320);
	
	for (int i = 0; i < 8; i++)
	{
		ct4.data.clutDat.colorSpec[i] = packColor(i, i, i);
		ct5.data.clutDat.colorSpec[i] = packColor(i + 8, i + 8, i + 8);
	}

	track.push_back(ct4);
	track.push_back(ct5);
	pushEmpty(track, 320);

	//make transparent test
	track.push_back(tr);
	
	//burst write mode
	for (int x = 0; x < CDG_WIDTH / COL_MULT; x++)
	{
		for (int y = 0; y < CDG_HEIGHT / ROW_MULT; y++)
		{
			SubCode p;
			p.command = SCCMD_CDG;
			p.instruction = CDG_Instruction::CDG_TILEBLOCK;
			uint8_t col = x + y;
			while (col > 15)
				col -= 16;
			p.data.tileDat.color0 = col;
			p.data.tileDat.row = y;
			p.data.tileDat.column = x;
			for (int i = 0; i < 12; i++)
				p.data.tileDat.tilePixels[i] = 0;
			
			//commit p
			track.push_back(p);
		}
	}

	pushEmpty(track, 640);

	while (track.size() % 4 != 0)
		pushEmpty(track, 1);

	//dump track to disk
	ofstream out("out.cdg", ios::binary);
	for (size_t i = 0; i < track.size(); i++)
	{
		out.write((const char*)&track[i], sizeof(SubCode));
	}
	out.close();

	return 0;
}

short packColor(uint8_t r, uint8_t g, uint8_t b)
{
//    [---high byte---]   [---low byte----]
//     7 6 5 4 3 2 1 0     7 6 5 4 3 2 1 0
//     X X r r r r g g     X X g g b b b b
	uint8_t high, low;

	//r = r >> 4;
	//g = g >> 4;
	//b = b >> 4;
	
	high = r << 2;
	high |= g >> 2;
	low = b;
	low |= (g & LOWER_2_BITS) << 4;
	
	return ((low << 8) | high);
}

void pushEmpty(vector<SubCode>& t, int repeat)
{
	for (int i = 0; i < repeat; i++)
	{
		SubCode e;
		e.command = SubCode_Command::SCCMD_NONE;
		t.push_back(e);
	}
}

void renderBob(vector<SubCode>& track)
{
	SubCode bob0, bob1, bob2, bob3;
	bob0.command = bob1.command = bob2.command = bob3.command = SCCMD_CDG;
	bob0.instruction = bob2.instruction = CDG_Instruction::CDG_TILEBLOCK;
	bob1.instruction = bob3.instruction = CDG_Instruction::CDG_TILEBLOCKXOR;
	bob0.data.tileDat.row = bob1.data.tileDat.row = bob0.data.tileDat.column = bob1.data.tileDat.column = 2;
	bob2.data.tileDat.column = bob3.data.tileDat.column = 3;
	bob2.data.tileDat.row = bob3.data.tileDat.row = 2;
	bob0.data.tileDat.color0 = bob2.data.tileDat.color0 = 0;
	bob0.data.tileDat.color1 = bob2.data.tileDat.color1 = 14;
	bob1.data.tileDat.color0 = bob3.data.tileDat.color0 = 0;
	bob1.data.tileDat.color1 = bob3.data.tileDat.color1 = 15;
	
	for (int i = 0; i < 12; i++)
	{
		bob0.data.tileDat.tilePixels[i] = 0x20;
		bob2.data.tileDat.tilePixels[i] = 0x01;
	}
	bob0.data.tileDat.tilePixels[0] = 0x1F;
	bob0.data.tileDat.tilePixels[2] = 0x2C;
	bob0.data.tileDat.tilePixels[3] = 0x32;
	bob0.data.tileDat.tilePixels[4] = 0x32;
	bob0.data.tileDat.tilePixels[5] = 0x2C;
	bob0.data.tileDat.tilePixels[8] = 0x28;
	bob0.data.tileDat.tilePixels[9] = 0x2F;
	bob0.data.tileDat.tilePixels[11] = 0x1F;
	bob2.data.tileDat.tilePixels[0] = 0x3E;
	bob2.data.tileDat.tilePixels[2] = 0x0D;
	bob2.data.tileDat.tilePixels[3] = 0x13;
	bob2.data.tileDat.tilePixels[4] = 0x13;
	bob2.data.tileDat.tilePixels[5] = 0x0D;
	bob2.data.tileDat.tilePixels[8] = 0x05;
	bob2.data.tileDat.tilePixels[9] = 0x3D;
	bob2.data.tileDat.tilePixels[11] = 0x3E;
	for (int i = 0; i < 12; i++)
	{
		bob1.data.tileDat.tilePixels[i] = 0;
		bob3.data.tileDat.tilePixels[i] = 0;
	}
	bob1.data.tileDat.tilePixels[3] = 0x0C;
	bob1.data.tileDat.tilePixels[4] = 0x0C;
	bob3.data.tileDat.tilePixels[3] = 0x0C;
	bob3.data.tileDat.tilePixels[4] = 0x0C;
	
	track.push_back(bob0);
	track.push_back(bob1);
	track.push_back(bob2);
	track.push_back(bob3);
}
