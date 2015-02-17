
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <uv.h>

#include <libmissilelauncher/libmissilelauncher.h>

#include "tracker.h"
#include "launcher.h"

using std::ifstream;
using std::stringstream;
using std::string;

class OnlyDreamsNow
{

private:
	std::string haar_face_path;
	std::string haar_body_path;
	std::string face_csv_path;

	uv_loop_t *main_loop;

	long launcher_id = 0;
	long camera_id = 0;
	long diff = 5;
	long fps = 15;

	bool loaded = false;

	Tracker *odn_tracker = NULL;
	Launcher *odn_launcher = NULL;

public:
	OnlyDreamsNow();
	~OnlyDreamsNow();

	int Load(std::string config_path);
	int Run();
};
