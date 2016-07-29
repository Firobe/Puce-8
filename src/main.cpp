#include "main.h"
#include <fstream>



using namespace std;

int main(int argc, char** argv) {
	try {
		///////////////INIT////////////
		//Init Audio/Video/Text
		Video video("2d.vert", "2d.frag");
		gameLoop(video, argc, argv);
	}
	catch (exception const& ex) {
		cout << "ERROR : " << ex.what() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void gameLoop(Video& video, int argc, char** argv) {
	double oldTime = glfwGetTime();
	double dispTime = oldTime;
	////////////MAIN LOOP/////////
	cout << "Start !" << endl;
	srand(0);
	Chip chip;
	if(argc > 1){
		std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<byte> buffer(size);
		if (file.read((char*)buffer.data(), size))
			chip.loadProgram(buffer);
	}
	while (!glfwWindowShouldClose(video.win())) {
		while(glfwGetTime() - oldTime < 1./CLOCK_FREQUENCY);
		oldTime = glfwGetTime();
		glfwPollEvents();
		chip.nextInstr(video);
		if(glfwGetTime() - dispTime >= 1./DISP_FREQUENCY){
			chip.timers();
			video.refresh();
			dispTime = glfwGetTime();
		}
	}
}

