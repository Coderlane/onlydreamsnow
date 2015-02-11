
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <uv.h>

#include <libmissilelauncher/libmissilelauncher.h>

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

	int launcher_id = 0;
	int camera_id = 0;
	int diff = 5;
	int fps = 15;
	int max_difference = 2000;

	int center_count = 0;
	int max_center_count = 3;

	int gone_count = 0;
	int max_gone_count = 10;
	bool loaded = false;

	Launcher *launcher = NULL;

	void static RunInit(uv_timer_t *timer);

public:
	OnlyDreamsNow();
	~OnlyDreamsNow();

	int Load(std::string config_path);
	int Run();
};
