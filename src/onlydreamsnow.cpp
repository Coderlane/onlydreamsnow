
#include <string>
#include <syslog.h>
#include <confuse.h>
#include <unistd.h>

#include <uv.h>

#include "onlydreamsnow.h"

using std::string;

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
	char *local_face_csv_path = NULL;
	int rv = -100;
	uint32_t launchers_count;
	cfg_t *cfg = NULL;

	ml_launcher_t **launchers = NULL;
	ml_launcher_t *launcher = NULL;

	cfg_opt_t opts[] =
	{
		CFG_SIMPLE_STR((char *)"haar_face_path", &local_haar_face_path),
		CFG_SIMPLE_STR((char *)"haar_body_path", &local_haar_body_path),
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

	max_difference = 5000;
	if (local_haar_face_path == NULL)
	{
		syslog(LOG_ERR, "Failed to find haar_face_path in config.");
		rv = -3;
		goto out;
	}

	if (local_haar_body_path == NULL)
	{
		syslog(LOG_ERR, "Failed to find haar_body_path in config.");
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

	haar_face_path = string(local_haar_face_path);
	haar_body_path = string(local_haar_body_path);
	face_csv_path = string(local_face_csv_path);
	launcher = launchers[launcher_id];

	this->launcher = new Launcher(main_loop, launcher, camera_id, fps,
	                          haar_body_path, haar_face_path);


	loaded = true;
	rv = 0;
out:
	if (cfg != NULL)
	{
		cfg_free(cfg);
	}
	return rv;
}


void OnlyDreamsNow::RunInit(uv_timer_t *timer)
{
	OnlyDreamsNow *dreams = static_cast<OnlyDreamsNow *>(timer->data);

	dreams->launcher->Start();
}


/**
 * @brief
 *
 * @return
 */
int
OnlyDreamsNow::Run()
{
	uv_timer_t init_timer;

	uv_timer_init(main_loop, &init_timer);
	init_timer.data = this;
	uv_timer_start(&init_timer, OnlyDreamsNow::RunInit, 0, 0);

	return uv_run(main_loop, UV_RUN_DEFAULT);
}
