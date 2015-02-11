
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <syslog.h>

#include "launcher.h"

using namespace cv;
using namespace std;

Launcher::Launcher(uv_loop_t *main_loop,
                   ml_launcher_t *launcher, int camera_id, int fps,
                   std::string haar_body_path, std::string haar_face_path)
{

	int rv;
	ml_launcher_reference(launcher);
	rv = ml_launcher_claim(launcher);
	if (rv != ML_OK)
	{
		syslog(LOG_ERR, "Failed to claim launcher.");
	}

	this->main_loop = main_loop;
	this->launcher = launcher;
	this->camera_id = camera_id;
	this->fps = fps;
	this->haar_body_path = haar_body_path;
	this->haar_face_path = haar_face_path;

	uv_mutex_init(&(this->launcher_mutex));
	uv_timer_init(this->main_loop, &(this->launcher_stop_timer));

	this->launcher_stop_timer.data = this;
}

Launcher::~Launcher()
{
	if (launcher != NULL)
	{
		ml_launcher_unclaim(launcher);
		ml_launcher_dereference(launcher);
	}
}

int
Launcher::Start()
{
	int rv = -100;
	uv_mutex_lock(&this->launcher_mutex);
	if (running)
	{
		rv = 0;
		goto out;
	}
	continue_loop = true;
	rv = uv_thread_create(&(this->launcher_thread),
	                      Launcher::Run, &(this->continue_loop));

out:
	uv_mutex_unlock(&this->launcher_mutex);
	return rv;
}

int
Launcher::Stop()
{
	int rv = -100;
	uv_mutex_lock(&(this->launcher_mutex));
	if (!running)
	{
		rv = 0;
		goto out;
	}
	continue_loop = false;
	rv = uv_thread_join(&(this->launcher_thread));

out:
	uv_mutex_unlock(&this->launcher_mutex);
	return rv;
}

void
Launcher::Run(void *arg)
{
	Launcher *launcher = static_cast<Launcher *>(arg);

	vector<Mat> images;
	vector<int> labels;

	CascadeClassifier body_cascade, face_cascade;
	body_cascade.load(launcher->haar_body_path);
	face_cascade.load(launcher->haar_face_path);


	VideoCapture cap(launcher->camera_id);
	if (!cap.isOpened())
	{
		syslog(LOG_ERR, "Failed to open capture device.");
	}

	cap.set(CV_CAP_PROP_FPS, launcher->fps);

	while (launcher->continue_loop)
	{
		Rect frame_rect;
		Mat gray_frame, orig_frame;

		vector< Rect_<int> > bodies;

		if (!cap.grab())
			continue;

		if (!cap.retrieve(orig_frame))
			continue;

		frame_rect = Rect(0, 0, orig_frame.cols, orig_frame.rows);

		cvtColor(orig_frame, gray_frame, CV_BGR2GRAY);
		body_cascade.detectMultiScale(gray_frame, bodies);

		if (bodies.size() == 0)
		{
			std::cerr << "No body found." << std::endl;
		}

		for (size_t i = 0; i < bodies.size(); i++)
		{
			Rect face;
			vector< Rect_<int> > faces;
			Mat body_frame = gray_frame(bodies[i]);

			// Make sure there is actually a face in the body.
			// The body detector seems a bit weak.
			face_cascade.detectMultiScale(body_frame, faces);

			if (faces.size() == 0)
			{
				std::cerr << "No face found." << std::endl;
				continue;
			}

			// Use the first face found, and set it's offset.
			face = faces[0];
			face.x += bodies[i].x;
			face.y += bodies[i].y;

			//Track(frame_rect, face);
		}
	}
}

void
Launcher::Track(Launcher *launcher, Rect frame_rect, Rect face_rect)
{
	int center_x, face_center_x, face_diff_x;

	center_x = frame_rect.width / 2;
	face_center_x = face_rect.x + (face_rect.width / 2);
	face_diff_x = center_x - face_center_x;

	if (abs(face_diff_x) <= launcher->diff)
	{
		// Centered
		if (launcher->center_count++ == launcher->max_center_count)
		{
			launcher->center_count = 0;
			LauncherFire(launcher);
		}
	}
	else
	{
		int msec = 0;

		if (face_diff_x < center_x / 8)
		{
			msec = 40;
		}
		else if (face_diff_x < center_x / 4)
		{
			msec = 80;
		}
		else
		{
			msec = 120;
		}
		std::cerr << face_center_x << "," << center_x << " "
		          << face_diff_x << " " << msec << std::endl;

		// Not Centered.
		launcher->center_count = 0;
		if (face_center_x > center_x)
		{
			LauncherRight(launcher, msec);
		}
		else
		{
			LauncherLeft(launcher, msec);
		}
	}
}

void
Launcher::LauncherStop(uv_timer_t *timer)
{
	int rv;
	Launcher *launcher = static_cast<Launcher *>(timer->data);

	uv_mutex_lock(&(launcher->launcher_mutex));

	rv = ml_launcher_stop(launcher->launcher);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to stop launcher.");
	}

	launcher->state = LauncherState::STOPPED;

	uv_mutex_unlock(&(launcher->launcher_mutex));
}

int
Launcher::LauncherFire(Launcher *launcher)
{
	int rv;

	uv_mutex_lock(&(launcher->launcher_mutex));

	if (launcher->state != LauncherState::STOPPED)
	{
		rv = -1;
		goto out;
	}

	rv = ml_launcher_fire(launcher->launcher);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to fire launcher.");
		goto out;
	}

	launcher->state = LauncherState::FIRE;
	uv_timer_start(&(launcher->launcher_stop_timer),
	               Launcher::LauncherStop, 0, 5000);

out:
	uv_mutex_unlock(&(launcher->launcher_mutex));
	return rv;
}

int
Launcher::LauncherRight(Launcher *launcher, int msec)
{
	int rv;

	uv_mutex_lock(&(launcher->launcher_mutex));

	if (launcher->state != LauncherState::STOPPED)
	{
		rv = -1;
		goto out;
	}

	rv = ml_launcher_move(launcher->launcher, ML_RIGHT);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to track right.");
		goto out;
	}

	launcher->state = LauncherState::RIGHT;
	uv_timer_start(&(launcher->launcher_stop_timer),
	               Launcher::LauncherStop, 0, msec);

out:
	uv_mutex_unlock(&(launcher->launcher_mutex));
	return rv;
}

int
Launcher::LauncherLeft(Launcher *launcher, int msec)
{
	int rv;

	uv_mutex_lock(&(launcher->launcher_mutex));

	if (launcher->state != LauncherState::STOPPED)
	{
		rv = -1;
		goto out;
	}

	rv = ml_launcher_move(launcher->launcher, ML_LEFT);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to track left.");
		goto out;
	}

	launcher->state = LauncherState::LEFT;
	uv_timer_start(&(launcher->launcher_stop_timer),
	               Launcher::LauncherStop, 0, msec);
out:
	uv_mutex_unlock(&(launcher->launcher_mutex));
	return rv;
}

/**
 * @brief ResetLauncher the position of the launcher.
 */
int
Launcher::LauncherReset(Launcher *launcher)
{
	int rv;

	uv_mutex_lock(&(launcher->launcher_mutex));

	launcher->center_count = 0;

	rv = ml_launcher_zero(launcher->launcher);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to zero launcher.");
		goto out;
	}

	rv = ml_launcher_move_mseconds(launcher->launcher, ML_UP, 400);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to reset launcher angle.");
	}

out:
	uv_mutex_unlock(&(launcher->launcher_mutex));
	return rv;
}





