#ifndef MAIN__H
#define MAIN__H

#include <iostream>
#include <exception>

#include "video.h"
#include "keymanager.h"
#include "model2d.h"
#include "chip.h"

extern int screen_width;
extern int screen_height;
enum Axes { xAxis, yAxis, zAxis };

void gameLoop(Video&, int, char**);

#endif
