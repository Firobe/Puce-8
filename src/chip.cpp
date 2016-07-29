#include "chip.h"
#include <iostream>
#include <cstring>
//http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

using namespace std;

byte s0[5] = {0xF0, 0x90, 0x90, 0x90, 0xF0};
byte s1[5] = {0x20, 0x60, 0x20, 0x20, 0x70};
byte s2[5] = {0xF0, 0x10, 0xF0, 0x80, 0xF0};
byte s3[5] = {0xF0, 0x10, 0xF0, 0x10, 0xF0};
byte s4[5] = {0x90, 0x90, 0xF0, 0x10, 0x10};
byte s5[5] = {0xF0, 0x80, 0xF0, 0x10, 0xF0};
byte s6[5] = {0xF0, 0x80, 0xF0, 0x90, 0xF0};
byte s7[5] = {0xF0, 0x10, 0x20, 0x40, 0x40};
byte s8[5] = {0xF0, 0x90, 0xF0, 0x90, 0xF0};
byte s9[5] = {0xF0, 0x90, 0xF0, 0x10, 0xF0};
byte sA[5] = {0xF0, 0x90, 0xF0, 0x90, 0x90};
byte sB[5] = {0xE0, 0x90, 0xE0, 0x90, 0xE0};
byte sC[5] = {0xF0, 0x80, 0x80, 0x80, 0xF0};
byte sD[5] = {0xE0, 0x90, 0x90, 0x90, 0xE0};
byte sE[5] = {0xF0, 0x80, 0xF0, 0x80, 0xF0};
byte sF[5] = {0xF0, 0x80, 0xF0, 0x80, 0x80};
vector<int> key_map { GLFW_KEY_PAGE_DOWN, GLFW_KEY_KP_7, GLFW_KEY_KP_8,
	GLFW_KEY_KP_9, GLFW_KEY_KP_4, GLFW_KEY_KP_5, GLFW_KEY_KP_6,
	GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_PAGE_UP,
	GLFW_KEY_HOME, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_KP_ADD,
	GLFW_KEY_KP_ENTER, GLFW_KEY_END };


Chip::Chip() : _mem(MEM_SIZE), _regs(REG_NB), _stack(STACK_SIZE),
	_screen(DISP_WIDTH * DISP_HEIGHT), _pixelOn(glm::vec2(screen_width, screen_height)),
	_pixelOff(glm::vec2(screen_width, screen_height))
{
	_beep.loadBuffer("res/beep.ogg");
	_regSound = 0;
	_regDelay = 0;
	_regPC = 0x200;
	_mem[0x200] = 0x12;
	_mem[0x201] = 0x00;
	vector<byte> def {
		0x61, 0x11, 0x60, 0x00, 0x6A, 0x04, 0x6B, 0x02,
			0x00, 0xE0, 0xFA, 0x29, 0xD0, 0x15, 0xFB, 0x29, 
			0x8C, 0x00, 0x7C, 0x05, 0xDC, 0x15, 0x70, 0x01, 0x12, 0x08};
	loadProgram(def);
	_regSP = 0x0;
	//Font loading
	memcpy(&_mem[0] + 5 * 0, s0, 5);
	memcpy(&_mem[0] + 5 * 1, s1, 5);
	memcpy(&_mem[0] + 5 * 2, s2, 5);
	memcpy(&_mem[0] + 5 * 3, s3, 5);
	memcpy(&_mem[0] + 5 * 4, s4, 5);
	memcpy(&_mem[0] + 5 * 5, s5, 5);
	memcpy(&_mem[0] + 5 * 6, s6, 5);
	memcpy(&_mem[0] + 5 * 7, s7, 5);
	memcpy(&_mem[0] + 5 * 8, s8, 5);
	memcpy(&_mem[0] + 5 * 9, s9, 5);
	memcpy(&_mem[0] + 5 * 10, sA, 5);
	memcpy(&_mem[0] + 5 * 11, sB, 5);
	memcpy(&_mem[0] + 5 * 12, sC, 5);
	memcpy(&_mem[0] + 5 * 13, sD, 5);
	memcpy(&_mem[0] + 5 * 14, sE, 5);
	memcpy(&_mem[0] + 5 * 15, sF, 5);
	int pixelWidth = screen_width/DISP_WIDTH;
	int pixelHeight = screen_height/DISP_HEIGHT;
	_pixelOn.load("pixelon");
	_pixelOn.setSize(glm::vec2(pixelWidth, pixelHeight));
	_pixelOff.load("pixeloff");
	_pixelOff.setSize(glm::vec2(pixelWidth, pixelHeight));

}

void Chip::loadProgram(vector<byte>& buffer){
	for(unsigned int i = 0 ; i < buffer.size() ; i++)
		_mem[0x200 + i] = buffer[i];
}

void Chip::timers(){
	if(_regSound > 0){
		_beep.playSource();
		_regSound--;
	}
	else _beep.stopSource();
	if(_regDelay > 0)
		_regDelay--;
}

void Chip::writeScreen(int x, int y, bool data){
	x %= DISP_WIDTH;
	y %= DISP_HEIGHT;
	_screen[y * DISP_WIDTH + x] = data;
}

bool Chip::readScreen(int x, int y){
	return _screen[(y%DISP_HEIGHT) * DISP_WIDTH + (x%=DISP_WIDTH)];
}

void Chip::nextInstr(Video& video){
	int pixelWidth = screen_width/DISP_WIDTH;
	int pixelHeight = screen_height/DISP_HEIGHT;
	bool error = false;
	unsigned short instr = (_mem[_regPC] << 8) + _mem[_regPC + 1];
	_regPC += 2;
	byte msn = instr >> 12;
	byte dsn = (instr >> 8) - (msn << 4);
	byte tsn = (instr >> 4) - (msn << 8) - (dsn << 4);
	byte lsn = instr - (msn << 12) - (dsn << 8) - (tsn << 4);
	switch(msn){
		case 0x0:
			if(instr == 0x00E0){ //CLS
				glClear(GL_COLOR_BUFFER_BIT);
				for(unsigned int i = 0 ; i < DISP_WIDTH*DISP_HEIGHT ; i++)
					_screen[i] = false;
			}
			else if(instr == 0x00EE){ //RET
				if(_regSP - 1 < 0)
					throw runtime_error("Unstacking empty stack");
				_regPC = _stack[_regSP - 1];
				_regSP--;
			}
			else error = true;
			break;
		case 0x1: //JP
			_regPC = instr - (msn << 12);
			break;
		case 0x2: //CALL
			_regSP++;
			if(_regSP > STACK_SIZE)
				throw runtime_error("Stacking full stack");
			_stack[_regSP-1] = _regPC;
			_regPC = instr - (msn << 12);
			break;
		case 0x3: //SE
			if(_regs[dsn] == lsn + (tsn << 4))
				_regPC += 2;
			break;
		case 0x4: //SNE
			if(_regs[dsn] != lsn + (tsn << 4))
				_regPC += 2;
			break;
		case 0x5: //SE
			if(lsn != 0x0) error = true;
			if(_regs[dsn] == _regs[tsn])
				_regPC += 2;
			break;
		case 0x6: //LD
			_regs[dsn] = lsn + (tsn << 4);
			break;
		case 0x7: //ADD
			_regs[dsn] += lsn + (tsn << 4);
			break;
		case 0x8:
			switch(lsn){
				case 0x0: //LD
					_regs[dsn] = _regs[tsn];
					break;
				case 0x1: //OR
					_regs[dsn] |= _regs[tsn];
					break;
				case 0x2: //AND
					_regs[dsn] &= _regs[tsn];
					break;
				case 0x3: //XOR
					_regs[dsn] ^= _regs[tsn];
					break;
				case 0x4: //ADD
					_regs[0xF] = (int)_regs[dsn] +
						(int)_regs[tsn] > 255;
					_regs[dsn] += _regs[tsn];
					break;
				case 0x5: //SUB
					_regs[0xF] = _regs[dsn] > _regs[tsn];
					_regs[dsn] -= _regs[tsn];
					break;
				case 0x6: //SHR
					_regs[0xF] = _regs[dsn] & 0x1;
					_regs[dsn] >>= 1;
					break;
				case 0x7: //SUBN
					_regs[0xF] = _regs[tsn] > _regs[dsn];
					_regs[dsn] = _regs[tsn] - _regs[dsn];
					break;
				case 0xE: //SHL
					_regs[0xF] = _regs[dsn] >> 7;
					_regs[dsn] <<= 1;
					break;
				default:
					error = true;
			}
			break;
		case 0x9: //SNE
			if(lsn != 0x0) error = true;
			if(_regs[dsn] != _regs[tsn])
				_regPC += 2;
			break;
		case 0xA: //LD
			_regI = instr - (msn << 12);
			break;
		case 0xB: //JP
			_regPC = _regs[0x0] + instr - (msn << 12);
			break;
		case 0xC: //RND
			_regs[dsn] = (rand() % 256) & (lsn + (tsn << 4));
			break;
		case 0xD:{//DRW
				 byte collision = 0;
				 int x = _regs[dsn];
				 int y = _regs[tsn];
				 for(int line = 0 ; line < lsn ; line++){
					 byte toD = _mem[_regI + line];
					 for(int bit = 7 ; bit >= 0 ; bit--){
						 bool oldPixel = readScreen(x + bit,
								 y + line);
						 if(oldPixel && (toD % 2))
							 collision = 1;
						 writeScreen(x+bit, y+line, (toD % 2) ^ oldPixel);
						 if((toD % 2) ^ oldPixel){
							 _pixelOn.place(glm::vec2(((x+bit) % DISP_WIDTH) * pixelWidth, (31-((y+line) % DISP_HEIGHT)) * pixelHeight));
							 _pixelOn.draw(video);
						 }
						 else{
							 _pixelOff.place(glm::vec2(((x+bit) % DISP_WIDTH) * pixelWidth, (31-((y+line) % DISP_HEIGHT))* pixelHeight));
							 _pixelOff.draw(video);
						 }
						 toD >>= 1;

					 }
				 }
				 _regs[0xF] = collision;
			 }
			 break;
		case 0xE:
			 if(lsn + (tsn << 4) == 0x9E){ //SKP
				 if(KeyManager::check(key_map[_regs[dsn]]))
					 _regPC += 2;
			 }
			 else if(lsn + (tsn << 4) == 0xA1){ //SKNP
				 if(!KeyManager::check(key_map[_regs[dsn]]))
					 _regPC += 2;
			 }
			 else error = true;
			 break;
		case 0xF:
			 switch(lsn + (tsn << 4)){
				 case 0x07: //LD
					 _regs[dsn] = _regDelay;
					 break;
				 case 0x0A: //LD
					 for(int i = 0 ;;i++){
						 glfwPollEvents();
						 if(KeyManager::check(key_map[i % 0x10])){
							 _regs[dsn] = i % 0x10;
							 break;
						 }
					 }
					 break;
				 case 0x15: //LD
					 _regDelay = _regs[dsn];
					 break;
				 case 0x18: //LD
					 _regSound = _regs[dsn];
					 break;
				 case 0x1E: //ADD
					 _regI += _regs[dsn];
					 break;
				 case 0x29: //LD
					 if(_regs[dsn] > 0xF)
						 throw runtime_error("Nonexistent hexadecimal sprite");
					 _regI = _regs[dsn] * 5;
					 break;
				 case 0x33: //LD
					 if(_regI < 0x200 or _regI + 2 >= MEM_SIZE)
						 throw runtime_error("Writing out of memory");
					 _mem[_regI + 2] = _regs[dsn] % 10;
					 _mem[_regI + 1] = (_regs[dsn] / 10) % 10;
					 _mem[_regI] = (_regs[dsn] / 100) % 10;
					 break;
				 case 0x55: //LD
					 if(_regI < 0x200 or _regI + dsn >= MEM_SIZE)
						 throw runtime_error("Writing out of memory");
					 for(byte n = 0 ; n <= dsn ; n++)
						 _mem[_regI + n] = _regs[n];
					 break;
				 case 0x65: //LD
					 if(_regI < 0x200 or _regI + dsn >= MEM_SIZE)
						 throw runtime_error("Reading out of memory");
					 for(byte n = 0 ; n <= dsn ; n++)
						 _regs[n] = _mem[_regI + n];

					 break;
				 default:
					 error = true;
			 }
			 break;
	}
	if(error)
		throw runtime_error("Unknown OPCODE");
}

