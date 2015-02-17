
#ifndef TRACKER_H
#define TRACKER_H

#include <opencv2/core/core.hpp>

#include <string>
#include <uv.h>

#include "launcher.h"

class Tracker
{
private:
	Launcher *ot_launcher = NULL;

  uv_thread_t ot_thread;
  uv_async_t ot_async;

  std::string ot_haar_body_path;
  std::string ot_haar_face_path;

  int ot_camera_id = 0;
  int ot_fps = 20;

	bool ot_running = false;

  int ot_center_count = 0;
  int ot_max_center_count = 5;
  int ot_diff = 40;

  void static Run(void *arg);
  void static Track(Tracker *tracker,
                    cv::Rect frame_rect, cv::Rect face_rect);

  void static TrackerStop(uv_timer_t *timer);
  int static TrackerReset(Tracker *tracker);
  int static TrackerFire(Tracker *tracker);
  int static TrackerLeft(Tracker *tracker, int msec);
  int static TrackerRight(Tracker *tracker, int msec);

public:
  Tracker(Launcher *launcher, int camera_id, int fps, int diff,
           std::string haar_body_path, std::string haar_face_path);
  ~Tracker();

  int Start();
  int Stop();
};

#endif /* TRACKER_H */
