
#include <string>
#include <syslog.h>
#include <confuse.h>
#include <unistd.h>

#include "onlydreamsnow.h"

using std::string;

OnlyDreamsNow::OnlyDreamsNow()
{
}

OnlyDreamsNow::~OnlyDreamsNow()
{
	if (launcher != NULL)
	{
		ml_launcher_unclaim(launcher);
		ml_launcher_dereference(launcher);
	}
}

int
OnlyDreamsNow::Load(const string config_path)
{
	char *local_haar_xml_path = NULL;
	char *local_face_csv_path = NULL;
	int rv = -100;
	uint32_t launchers_count;
	cfg_t *cfg = NULL;

	cfg_opt_t opts[] =
	{
		CFG_SIMPLE_STR((char *)"haar_xml_path", &local_haar_xml_path),
		CFG_SIMPLE_STR((char *)"face_csv_path", &local_face_csv_path),
		CFG_SIMPLE_INT((char *)"camera_id", &camera_id),
		CFG_SIMPLE_INT((char *)"launcher_id", &launcher_id),
		CFG_SIMPLE_INT((char *)"diff", &diff),
		CFG_SIMPLE_INT((char *)"fps", &fps),
		CFG_SIMPLE_INT((char *)"max_difference", &max_difference),
		CFG_END()
	};

	if (loaded)
	{
		syslog(LOG_NOTICE, "Already loaded.");
		return 1;
	}

	rv = ml_launcher_array_new(&launchers, &launchers_count);
	if (rv != ML_OK)
	{
		if (rv != ML_NO_LAUNCHERS)
		{
			syslog(LOG_ERR, "Failed to get launcher array.");
		}
		else
		{
			syslog(LOG_ERR, "No launchers found.");
		}
		goto out;
	}

	cfg = cfg_init(opts, 0);
	rv = cfg_parse(cfg, config_path.c_str());
	if (rv != CFG_SUCCESS)
	{
		syslog(LOG_ERR, "Failed to read configuration file.");
		rv = -2;
		goto out;
	}

	if (local_haar_xml_path == NULL)
	{
		syslog(LOG_ERR, "Failed to find haar_xml_path in config.");
		rv = -3;
		goto out;
	}

	if (local_face_csv_path == NULL)
	{
		syslog(LOG_ERR, "Failed to find face_csv_path in config.");
		rv = -4;
		goto out;
	}

	if (launcher_id < 0)
	{
		syslog(LOG_ERR, "Invalid launcehr id.");
		rv = -5;
		goto out;
	}

	if (launchers_count < (uint32_t) launcher_id)
	{
		syslog(LOG_ERR, "Didn't find launcher.");
		rv = -6;
		goto out;
	}

	haar_xml_path = string(local_haar_xml_path);
	face_csv_path = string(local_face_csv_path);
	launcher = launchers[launcher_id];
	ml_launcher_reference(launcher);

	rv = ml_launcher_claim(launcher);
	if(rv != ML_OK) {
		syslog(LOG_ERR, "Failed to claim launcher.");
		goto out;
	}

	loaded = true;
	rv = 0;
out:
	if (cfg != NULL)
	{
		cfg_free(cfg);
	}
	return rv;
}


/**
 * @brief Read a csv file from a specific format.
 * Taken from OpenCV example.
 *
 * @param filename
 * @param images
 * @param labels
 * @param separator
 */
void
OnlyDreamsNow::ReadCSV(const string& filename, vector<Mat>& images,
                       vector<int>& labels, char separator)
{
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file)
	{
		string error_message = "No valid input file was given, "
		                       "please check the given filename.";
		CV_Error(CV_StsBadArg, error_message);
	}

	string line, path, classlabel;
	while (getline(file, line))
	{
		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, classlabel);
		if (!path.empty() && !classlabel.empty())
		{
			images.push_back(imread(path, 0));
			labels.push_back(atoi(classlabel.c_str()));
		}
	}
}

/**
 * @brief
 *
 * @return
 */
int
OnlyDreamsNow::Run()
{
	vector<Mat> images;
	vector<int> labels;
	try
	{
		ReadCSV(face_csv_path, images, labels);
	}
	catch (cv::Exception& e)
	{
		syslog(LOG_ERR, "Failed to open csv file.");
		return -1;
	}

	int face_width = images[0].cols;
	int face_height = images[0].rows;

	Ptr<FaceRecognizer> model = createFisherFaceRecognizer();
	model->train(images, labels);

	CascadeClassifier haar_cascade;
	haar_cascade.load(haar_xml_path);

	VideoCapture cap(camera_id);
	if (!cap.isOpened())
	{
		syslog(LOG_ERR, "Failed to open capture device.");
		return -2;
	}

	cap.set(CV_CAP_PROP_FPS, 2);

	ResetLauncher();

	for (;;)
	{
		int prediction = -1;
		double confidence = 0.0;
		Mat gray, frame, original;
		vector< Rect_<int> > faces;

		if (!cap.grab())
			continue;

		if (!cap.retrieve(frame))
			continue;

		original = frame.clone();
		cvtColor(original, gray, CV_BGR2GRAY);
		haar_cascade.detectMultiScale(gray, faces);

		if (faces.size() == 0)
		{
			std::cerr << "No face found." << std::endl;
			if (gone_count++ == max_gone_count)
			{
				gone_count = 0;
				ResetLauncher();
			}

		}

		for (size_t i = 0; i < faces.size(); i++)
		{
			Rect face_rect = faces[i];
			Mat face = gray(face_rect);

			Mat face_resized;
			cv::resize(face, face_resized,
			           Size(face_width, face_height),
			           1.0, 1.0, INTER_CUBIC);

			model->predict(face_resized, prediction, confidence);
			if (confidence > max_difference)
			{
				std::cerr << "No Match. Confidence: " << confidence << std::endl;
				if (gone_count++ == max_gone_count)
				{
					gone_count = 0;
					ResetLauncher();
				}
				continue;
			}

			gone_count = 0;
			StopLauncher();

			std::cerr << "Match: " << prediction << " "
			          << "Confidence: " << confidence << std::endl;

			std::cerr << "Position: (" << face_rect.x + (face_rect.width / 2) << ","
			          << face_rect.y + (face_rect.height / 2) << ")\n";

			Track(face_rect.x + (face_rect.width / 2), original.cols / 2);
		}
	}

	return 0;
}

void
OnlyDreamsNow::Track(int x, int center)
{
	if (center + diff > x && center - diff <  x)
	{
		// Centered
		if (center_count++ == max_center_count)
		{
			center_count = 0;
			FireLauncher();
		}
		return;
	}

	// Not Centered.
	center_count = 0;
	if (center + diff < x)
	{
		TrackRight();
	}
	else
	{
		TrackLeft();
	}
}

void
OnlyDreamsNow::StopLauncher()
{
	int rv;
	rv = ml_launcher_stop(launcher);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to stop launcher.");
	}
}

void
OnlyDreamsNow::FireLauncher()
{
	int rv;
	std::cerr << "Firing.\n";
	rv = ml_launcher_fire(launcher);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to fire launcher.");
	}
}

void
OnlyDreamsNow::TrackRight()
{
	int rv;
	std::cerr << "Track Right.\n";
	rv = ml_launcher_move_mseconds(launcher, ML_RIGHT, 100);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to track right.");
	}
	zeroed = false;
}

void
OnlyDreamsNow::TrackLeft()
{
	int rv;
	std::cerr << "Track Left.\n";
	rv = ml_launcher_move_mseconds(launcher, ML_LEFT, 100);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to track left.");
	}
	zeroed = false;
}

/**
 * @brief ResetLauncher the position of the launcher.
 */
void
OnlyDreamsNow::ResetLauncher()
{
	int rv;
	std::cerr << "ResetLauncher.\n";

	center_count = 0;
	gone_count = 0;

	// Already zeroed.
	if (zeroed)
		return;

	rv = ml_launcher_zero(launcher);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to zero launcher.");
		return;
	}

	rv = ml_launcher_move_mseconds(launcher, ML_UP, 300);
	if (rv != ML_OK)
	{
		syslog(LOG_NOTICE, "Failed to reset launcher angle.");
	}

	zeroed = true;
}

