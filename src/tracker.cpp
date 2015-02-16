
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

#include "tracker.h"

using namespace cv;
using namespace std;

Tracker::Tracker(Launcher *launcher, int camera_id, int fps,
                 std::string haar_body_path, std::string haar_face_path)
{
	ot_launcher = launcher;
	ot_camera_id = camera_id;
	ot_fps = fps;

	ot_haar_body_path = haar_body_path;
	ot_haar_face_path = haar_face_path;

	ot_running = true;
	uv_thread_create(&(ot_thread), Tracker::Run, this);
}

Tracker::~Tracker()
{
	ot_running = false;
	uv_thread_join(&(ot_thread));
}

void
Tracker::Run(void *arg)
{
  Tracker *tracker = static_cast<Tracker *>(arg);
  vector<Mat> images;
  vector<int> labels;
  CascadeClassifier body_cascade, face_cascade;

  body_cascade.load(tracker->ot_haar_body_path);
  face_cascade.load(tracker->ot_haar_face_path);

  // Reset the launcher.
	tracker->ot_launcher->Reset();

  VideoCapture cap(tracker->ot_camera_id);
  if (!cap.isOpened()) {
    syslog(LOG_ERR, "Failed to open capture device.");
  }

  cap.set(CV_CAP_PROP_FPS, tracker->ot_fps);

  while (tracker->ot_running) {
    Rect frame_rect;
    Mat gray_frame, orig_frame;

    vector< Rect_<int> > bodies;

    if (!cap.grab()) {
      continue;
    }

    if (!cap.retrieve(orig_frame)) {
      continue;
    }

    frame_rect = Rect(0, 0, orig_frame.cols, orig_frame.rows);

    cvtColor(orig_frame, gray_frame, CV_BGR2GRAY);
    body_cascade.detectMultiScale(gray_frame, bodies);

    // No bodies foudn.
    if (bodies.size() == 0) {
      continue;
    }

    for (size_t i = 0; i < bodies.size(); i++) {
      Rect face;
      vector< Rect_<int> > faces;
      Mat body_frame = gray_frame(bodies[i]);

      // Make sure there is actually a face in the body.
      // The body detector seems a bit weak.
      face_cascade.detectMultiScale(body_frame, faces);

      // No faces found.
      if (faces.size() == 0) {
        continue;
      }

      // Use the first face found, and set it's offset.
      face = faces[0];
      face.x += bodies[i].x;
      face.y += bodies[i].y;

      Track(tracker, frame_rect, face);
    }
  }
}

void
Tracker::Track(Tracker *tracker, Rect frame_rect, Rect face_rect)
{
  int center_x, face_center_x, face_diff_x;

  center_x = frame_rect.width / 2;
  face_center_x = face_rect.x + (face_rect.width / 2);
  face_diff_x = center_x - face_center_x;

  if (abs(face_diff_x) <= tracker->ot_diff) {
    // Centered
    if (tracker->ot_center_count++ == tracker->ot_max_center_count) {
      tracker->ot_center_count = 0;
   		tracker->ot_launcher->Fire(); 
		}
  } else {
    int msec = 0;

    if (face_diff_x < center_x / 8) {
      msec = 40;
    } else if (face_diff_x < center_x / 4) {
      msec = 80;
    } else {
      msec = 120;
    }

    // Not Centered.
    tracker->ot_center_count = 0;
    if (face_center_x > center_x) {
      tracker->ot_launcher->Move(LauncherDirection::RIGHT, msec);
    } else {
      tracker->ot_launcher->Move(LauncherDirection::LEFT, msec);
    }
  }
}
