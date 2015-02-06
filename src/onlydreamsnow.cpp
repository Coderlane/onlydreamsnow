
#include <string>
#include <syslog.h>
#include <confuse.h>

using std::string;

int run(const string config_path)
{
  char *haar_xml_path = NULL;
  char *face_xml_path = NULL;
  int device_id = -1, rv = -1;
  cfg_t *cfg = NULL;
  cfg_opt_t opts[] = {
    CFG_SIMPLE_STR((char *)"haar_xml_path", &haar_xml_path),
    CFG_SIMPLE_STR((char *)"face_xml_path", &face_xml_path),
    CFG_SIMPLE_INT((char *)"device_id", &device_id),
    CFG_END()
  };


  cfg = cfg_init(opts, 0);
  rv = cfg_parse(cfg, config_path.c_str());
  if(rv != CFG_SUCCESS) {
    goto out;
  }
	if(haar_xml_path == NULL) {

	}
	if(face_xml_path == NULL) {

	}



out:
  if(cfg != NULL) {
    cfg_free(cfg);
  }
  return rv;
}


