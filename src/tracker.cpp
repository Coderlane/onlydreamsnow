/**
 * @file tracker.cpp
 * @brief
 * @author Travis Lane
 * @version 0.0.1
 * @date 2015-10-27
 */

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

Tracker::Tracker(Launcher *launcher, int camera_id, int fps, int diff,
                 std::string haar_body_path, std::string haar_face_path)
{
	ot_launcher = launcher;
	ot_camera_id = camera_id;
	ot_fps = fps;
	ot_diff = diff;

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

	cerr << tracker->ot_camera_id << endl;
  VideoCapture cap(tracker->ot_camera_id);
  if (!cap.isOpened()) {
    syslog(LOG_ERR, "Failed to open capture device.");
  }

	cerr << "FPS: " << tracker->ot_fps << endl;
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

			//rectangle(orig_frame, bodies[i], CV_RGB(0, 255, 0), 1);
      // Use the first face found, and set it's offset.
      face = faces[0];
      face.x += bodies[i].x;
      face.y += bodies[i].y;

			//rectangle(orig_frame, face, CV_RGB(0, 0, 255), 1);

      Track(tracker, frame_rect, face);
    }

		//imshow("face", orig_frame);
		//waitKey(5);
  }
}

void
Tracker::Track(Tracker *tracker, Rect frame_rect, Rect face_rect)
{
  int center_x, face_center_x, face_diff_x;

  center_x = frame_rect.width / 2;
  face_center_x = face_rect.x + (face_rect.width / 2);
  face_diff_x = abs(center_x - face_center_x);

  if (face_diff_x <= tracker->ot_diff) {
    // Centered
    if (tracker->ot_center_count++ == tracker->ot_max_center_count) {
      tracker->ot_center_count = 0;
   		tracker->ot_launcher->Fire();
		}
  } else {
    int msec = 0;

		cerr << center_x << " " << face_center_x << " "
			 << face_diff_x << endl;

    if (face_diff_x < center_x / 8) {
      msec = 10;
    } else if (face_diff_x < center_x / 4) {
      msec = 30;
    } else {
      msec = 60;
    }

    MilliDurationType duration(msec);

    // Not Centered.
    tracker->ot_center_count = 0;
    if (face_center_x > center_x) {
      tracker->ot_launcher->Move(DirectionType::RIGHT, duration);
    } else {
      tracker->ot_launcher->Move(DirectionType::LEFT, duration);
    }
  }
}
