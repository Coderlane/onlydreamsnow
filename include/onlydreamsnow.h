

#include <string>

using std::string;

class OnlyDreamsNow
{

private:
	string haar_xml_path;
	string face_xml_path;
	int    device_id;

	bool loaded;

public:
	OnlyDreamsNow();
	~OnlyDreamsNow();

	int Load(string config_path);
	int Run();
};
