#include <iostream>
#include "clpingpong.h"

int main(int argc, char* argv[]) {
	if (argc > 3) {
		std::cerr << "Usage: " << argv[0] << "[platform_no] [device_no]" << std::endl;
		return 1;
	}
	const int platform_id = argc>1 ? atoi(argv[1]) : -1;
	const int device_id = argc>2 ? atoi(argv[2]) : -1;
	clPingPongApp app{platform_id, device_id};
	app.run();

	return 0;
}
