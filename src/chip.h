#ifndef CHIP__H
#define CHIP__H

#include <vector>
#include <stdexcept>
#include "model2d.h"
#include "video.h"
#include "audio.h"

using byte = unsigned char;
extern int screen_width;
extern int screen_height;

#define CLOCK_FREQUENCY 700
#define DISP_FREQUENCY 60

#define DISP_WIDTH 64
#define DISP_HEIGHT 32
#define EXT_DISP_WIDTH 128
#define EXT_DISP_HEIGHT 64
#define SCREEN_WRAP true
#define SCREEN_REDRAW false

#define MEM_SIZE 0xFFF
#define REG_NB 16
#define RPLREG_NB 8
#define STACK_SIZE 16

class Chip{
	public:
		Chip();
		void timers(); //True if tone should be emitted
		void loadProgram(std::vector<unsigned char>&);
		void nextInstr(Video& video);
		void writeScreen(int, int, bool);
		bool readScreen(int, int);
		void reDraw(Video& video);
	private:
		std::vector<byte> _mem;
		std::vector<byte> _regs;
		unsigned short _regI;
		byte _regDelay;
		byte _regSound;
		unsigned short _regPC;
		byte _regSP;
		std::vector<unsigned short> _stack;
		std::vector<bool> _screen;
		Model2D _pixelOn, _pixelOff;
		unsigned int _dWidth, _dHeight;
		Audio _beep;

		bool _extended;
		std::vector<byte> _regsRPL;
};

#endif
