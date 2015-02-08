
#include <string>
#include <syslog.h>
#include <confuse.h>

#include "onlydreamsnow.h"

using std::string;

OnlyDreamsNow::OnlyDreamsNow()
{
	loaded = false;
}

OnlyDreamsNow::~OnlyDreamsNow()
{

}

int
OnlyDreamsNow::Load(const string config_path)
{
	char *local_haar_xml_path = NULL;
	char *local_face_csv_path = NULL;
	int local_device_id = -1, rv = -100;
	cfg_t *cfg = NULL;
	cfg_opt_t opts[] =
	{
		CFG_SIMPLE_STR((char *)"haar_xml_path", &local_haar_xml_path),
		CFG_SIMPLE_STR((char *)"face_csv_path", &local_face_csv_path),
		CFG_SIMPLE_INT((char *)"device_id", &local_device_id),
		CFG_END()
	};

	if (loaded)
	{
		syslog(LOG_NOTICE, "Already loaded.");
		return 1;
	}

	cfg = cfg_init(opts, 0);
	rv = cfg_parse(cfg, config_path.c_str());
	if (rv != CFG_SUCCESS)
	{
		syslog(LOG_ERR, "Failed to read configuration file.");
		rv = -1;
		goto out;
	}

	if (local_haar_xml_path == NULL)
	{
		syslog(LOG_ERR, "Failed to find haar_xml_path in config.");
		rv = -2;
		goto out;
	}

	if (local_face_csv_path == NULL)
	{
		syslog(LOG_ERR, "Failed to find face_csv_path in config.");
		rv = -3;
		goto out;
	}

	if (local_device_id == -1)
	{
		syslog(LOG_ERR, "Failed to find device_id in config.");
		rv = -4;
		goto out;
	}

	haar_xml_path = string(local_haar_xml_path);
	face_csv_path = string(local_face_csv_path);
	device_id = local_device_id;

	loaded = true;
	rv = 0;
out:
	if (cfg != NULL)
	{
		cfg_free(cfg);
	}
	return rv;
}


void OnlyDreamsNow::ReadCSV(const string& filename, vector<Mat>& images,
                            vector<int>& labels, char separator)
{
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file)
	{
		string error_message = "No valid input file was given, please check the given filename.";
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


int
OnlyDreamsNow::Run()
{

	int rv = -100;
	// These vectors hold the images and corresponding labels:
	vector<Mat> images;
	vector<int> labels;
	// Read in the data (fails if no valid input filename is given, but you'll get an error message):
	try
	{
		ReadCSV(face_csv_path, images, labels);
	}
	catch (cv::Exception& e)
	{
		syslog(LOG_ERR, "Failed to open csv file.");
		return -1;
	}
	// Get the height from the first image. We'll need this
	// later in code to reshape the images to their original
	// size AND we need to reshape incoming faces to this size:
	int im_width = images[0].cols;
	int im_height = images[0].rows;
	// Create a FaceRecognizer and train it on the given images:
	Ptr<FaceRecognizer> model = createFisherFaceRecognizer();
	model->train(images, labels);
	// That's it for learning the Face Recognition model. You now
	// need to create the classifier for the task of Face Detection.
	// We are going to use the haar cascade you have specified in the
	// command line arguments:
	//
	CascadeClassifier haar_cascade;
	haar_cascade.load(haar_xml_path);
	// Get a handle to the Video device:
	VideoCapture cap(device_id);
	// Check if we can use this device at all:
	if (!cap.isOpened())
	{
		syslog(LOG_ERR, "Failed to open capture device.");
		return -2;
	}
	// Holds the current frame from the Video device:
	Mat frame;
	for (;;)
	{
		cap >> frame;
		Mat original = frame.clone();
		Mat gray;
		cvtColor(original, gray, CV_BGR2GRAY);
		vector< Rect_<int> > faces;
		haar_cascade.detectMultiScale(gray, faces);
		
		for (size_t i = 0; i < faces.size(); i++)
		{
			Rect face_i = faces[i];
			Mat face = gray(face_i);

			Mat face_resized;
			cv::resize(face, face_resized,
			           Size(im_width, im_height),
			           1.0, 1.0, INTER_CUBIC);
			
			int prediction = model->predict(face_resized);
			// And finally write all we've found out to the original image!
			// First of all draw a green rectangle around the detected face:
			rectangle(original, face_i, CV_RGB(0, 255, 0), 1);
			// Create the text we will annotate the box with:
			string box_text = format("Prediction = %d", prediction);
			// Calculate the position for annotated text (make sure we don't
			// put illegal values in there):
			int pos_x = std::max(face_i.tl().x - 10, 0);
			int pos_y = std::max(face_i.tl().y - 10, 0);
			// And now put it into the image:
			putText(original, box_text, Point(pos_x, pos_y),
			        FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0), 2.0);
		}
		// Show the result:
		imshow("face_recognizer", original);
		// And display it:
		char key = (char) waitKey(20);
		// Exit this loop on escape:
		if (key == 27)
			break;
	}

	return rv;
}
