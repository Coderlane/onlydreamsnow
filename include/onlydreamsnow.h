

#include <opencv2/core/core.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <libmissilelauncher/libmissilelauncher.h>

using std::ifstream;
using std::stringstream;
using std::vector;
using std::string;

using namespace cv;

class OnlyDreamsNow
{

private:
	string haar_xml_path;
	string face_csv_path;

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
	bool zeroed = false;

	ml_launcher_t **launchers = NULL;
	ml_launcher_t *launcher = NULL;

	void ReadCSV(const string& filename, vector<Mat>& images,
	             vector<int>& labels, char separator = ';');

	void Track(int x, int center);
	void TrackLeft();
	void TrackRight();
	void StopLauncher();
	void FireLauncher();
	void ResetLauncher();

public:
	OnlyDreamsNow();
	~OnlyDreamsNow();

	int Load(string config_path);
	int Run();
};
