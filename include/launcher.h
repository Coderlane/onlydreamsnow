
#include <opencv2/core/core.hpp>

#include <string>

#include <uv.h>

#include <libmissilelauncher/libmissilelauncher.h>

enum class LauncherState
{
    FIRE,
    LEFT,
    RIGHT,
    STOPPED
};

class Launcher
{
private:

	cv::Point GetClosestFace(cv::Mat frame);

	uv_loop_t *main_loop;

	uv_thread_t launcher_thread;
	uv_thread_t camera_thread;

	uv_mutex_t movement_mutex;
	uv_timer_t stop_timer;

	bool running = false;
	bool continue_loop = false;
	LauncherState state = LauncherState::STOPPED;

	std::string haar_body_path;
	std::string haar_face_path;

	ml_launcher_t *launcher;
	int camera_id;
	int fps;

	int center_count = 0;
	int max_center_count = 5;
	int diff = 40;


	void static RunCamera(void *arg);
	void static RunLauncher(void *arg);
	void static Track(Launcher *launcher,
	                  cv::Rect frame_rect, cv::Rect face_rect);


	void static LauncherStop(uv_timer_t *timer);
	int static LauncherReset(Launcher *launcher);
	int static LauncherFire(Launcher *launcher);
	int static LauncherLeft(Launcher *launcher, int msec);
	int static LauncherRight(Launcher *launcher, int msec);

public:
	Launcher(uv_loop_t *main_loop,
	         ml_launcher_t *launcher, int camera_id, int fps,
	         std::string haar_body_path, std::string haar_face_path);
	~Launcher();
	int Start();
	int Stop();
};



