

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
	int    device_id;

	bool loaded;

	void ReadCSV(const string& filename, vector<Mat>& images, 
			vector<int>& labels, char separator = ';');
public:
	OnlyDreamsNow();
	~OnlyDreamsNow();

	int Load(string config_path);
	int Run();
};
