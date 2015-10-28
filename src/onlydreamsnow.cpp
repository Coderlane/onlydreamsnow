
/**
 * @file onlydreamsnow.cpp
 * @brief
 * @author Travis Lane
 * @version 0.0.1
 * @date 2015-10-27
 */
#include <string>
#include <syslog.h>
#include <confuse.h>
#include <unistd.h>

#include <uv.h>

#include "onlydreamsnow.h"

using namespace std;

OnlyDreamsNow::OnlyDreamsNow()
{
  main_loop = uv_default_loop();
}

OnlyDreamsNow::~OnlyDreamsNow()
{

}

int
OnlyDreamsNow::Load(const string config_path)
{
  char *local_haar_face_path = NULL;
  char *local_haar_body_path = NULL;
  int rv = -100;
  uint32_t launchers_count;
  cfg_t *cfg = NULL;

  ml_launcher_t **launchers = NULL;
  ml_launcher_t *launcher = NULL;

  cfg_opt_t opts[] = {
    CFG_SIMPLE_STR((char *)"haar_face_path", &local_haar_face_path),
    CFG_SIMPLE_STR((char *)"haar_body_path", &local_haar_body_path),
    CFG_SIMPLE_INT((char *)"camera_id", &camera_id),
    CFG_SIMPLE_INT((char *)"launcher_id", &launcher_id),
    CFG_SIMPLE_INT((char *)"diff", &diff),
    CFG_SIMPLE_INT((char *)"fps", &fps),
    CFG_END()
  };

  if (loaded) {
    syslog(LOG_NOTICE, "Already loaded.");
    return 1;
  }

  rv = ml_launcher_array_new(&launchers, &launchers_count);
  if (rv != ML_OK) {
    if (rv != ML_NO_LAUNCHERS) {
      syslog(LOG_ERR, "Failed to get launcher array.");
    } else {
      syslog(LOG_ERR, "No launchers found.");
    }
    goto out;
  }

  cfg = cfg_init(opts, 0);
  rv = cfg_parse(cfg, config_path.c_str());
  if (rv != CFG_SUCCESS) {
    syslog(LOG_ERR, "Failed to read configuration file.");
    rv = -2;
    goto out;
  }

  if (local_haar_face_path == NULL) {
    syslog(LOG_ERR, "Failed to find haar_face_path in config.");
    rv = -3;
    goto out;
  }

  if (local_haar_body_path == NULL) {
    syslog(LOG_ERR, "Failed to find haar_body_path in config.");
    rv = -3;
    goto out;
  }

  if (launcher_id < 0) {
    syslog(LOG_ERR, "Invalid launcehr id.");
    rv = -5;
    goto out;
  }

  if (launchers_count < (uint32_t) launcher_id) {
    syslog(LOG_ERR, "Didn't find launcher.");
    rv = -6;
    goto out;
  }

  haar_face_path = string(local_haar_face_path);
  haar_body_path = string(local_haar_body_path);
  launcher = launchers[launcher_id];

  cerr << camera_id << endl;

  this->odn_launcher = new Launcher(launcher);
  this->odn_tracker = new Tracker(this->odn_launcher,
                                  (int) camera_id, (int) fps, (int) diff,
                                  haar_body_path, haar_face_path);

  loaded = true;
  rv = 0;
out:
  if (cfg != NULL) {
    cfg_free(cfg);
  }
  return rv;
}

/**
 * @brief
 *
 * @return
 */
int
OnlyDreamsNow::Run()
{
  uv_async_t async;
  uv_async_init(main_loop, &async, NULL);

  return uv_run(main_loop, UV_RUN_DEFAULT);
}


