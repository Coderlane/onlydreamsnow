
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
  char *local_face_xml_path = NULL;
  int local_device_id = -1, rv = -100;
  cfg_t *cfg = NULL;
  cfg_opt_t opts[] = {
    CFG_SIMPLE_STR((char *)"haar_xml_path", &local_haar_xml_path),
    CFG_SIMPLE_STR((char *)"face_xml_path", &local_face_xml_path),
    CFG_SIMPLE_INT((char *)"device_id", &local_device_id),
    CFG_END()
  };

	if(loaded) {
		syslog(LOG_NOTICE, "Already loaded.");
		return 1;
	}

  cfg = cfg_init(opts, 0);
  rv = cfg_parse(cfg, config_path.c_str());
  if(rv != CFG_SUCCESS) {
		syslog(LOG_ERR, "Failed to read configuration file.");
		rv = -1;
    goto out;
  }

	if(local_haar_xml_path == NULL) {
		syslog(LOG_ERR, "Failed to find haar_xml_path in config.");
		rv = -2;
		goto out;
	}

	if(local_face_xml_path == NULL) {
		syslog(LOG_ERR, "Failed to find face_xml_path in config.");
		rv = -3;
		goto out;
	}

	if(local_device_id == -1) {
		syslog(LOG_ERR, "Failed to find device_id in config.");
		rv = -4;
		goto out;
	}
	
	haar_xml_path = string(local_haar_xml_path);
	face_xml_path = string(local_face_xml_path);
	device_id = local_device_id;

	loaded = true;
	rv = 0;
out:
  if(cfg != NULL) {
    cfg_free(cfg);
  }
  return rv;
}

int 
OnlyDreamsNow::Run()
{

	return -1;
}
