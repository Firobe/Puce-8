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

#define DISP_WIDTH 64
#define DISP_HEIGHT 32
#define MEM_SIZE 0xFFF
#define REG_NB 16
#define STACK_SIZE 16

class Chip{
	public:
		Chip();
		void timers(); //True if tone should be emitted
		void loadProgram(std::vector<unsigned char>&);
		void nextInstr(Video& video);
		void writeScreen(int, int, bool);
		bool readScreen(int, int);
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
		Audio _beep;
};

#endif
