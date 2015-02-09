/**
 * @file main.c
 * @brief Generic daemon
 * @author Travis Lane
 * @version 0.1.0
 * @date 2015-02-02
 */

#include <iostream>
#include <string>
#include <cctype>

#include <libgen.h>
#include <stdlib.h>
#include <cstring>
#include <syslog.h>
#include <unistd.h>

#include <libmissilelauncher/libmissilelauncher.h>

#include "onlydreamsnow.h"

using std::string;
using std::cerr;
using std::cout;

static string program_name = "";
static string program_version = "0.1.0";
static string default_config_path = "/.onlydreamsnow/onlydreamsnow.cfg";

int run(const string config_path);
void usage();
void version();

int
main(int argc, char **argv)
{
	char *path = NULL;
	int opt, rv = -3;
	bool foreground = false;
	string config_path = "";

	path = strdup(argv[0]);
	program_name = basename(path);
	if (program_name.empty())
	{
		cerr << "Failed to get program name.\n";
		rv = -1;
		goto out;
	}

	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog(program_name.c_str(), LOG_CONS | LOG_PERROR | LOG_PID, LOG_USER);

	while ((opt = getopt (argc, argv, "c:fhrv")) != -1)
	{
		switch (opt)
		{
		case 'c':
			config_path = string(optarg);
			break;
		case 'f':
			foreground = 1;
			break;
		case 'h':
			usage();
			rv = 0;
			goto out;
		case 'v':
			version();
			rv = 0;
			goto out;
		case '?':
			usage();
			rv = 1;
			goto out;
		default:
			/* Shouldn't happen. */
			rv = -1;
			goto out;
		}
	}

	if (!foreground)
	{
		daemon(0, 0);
	}

	if (config_path.empty())
	{
		string home(getenv("HOME"));
		config_path = home + default_config_path;
	}

	syslog(LOG_NOTICE, "Starting...");

	rv = run(config_path);

	syslog(LOG_NOTICE, "Stopping...");

out:
	if (path != NULL)
	{
		free(path);
	}

	return rv;
}

/**
 * @brief Print daemon usage.
 */
void
usage()
{
	cout << "Usage:\n";
	cout << "\t" << program_name <<  "  [option] - %s\n\n" <<
	     "A daemon that watches over all longboard processies.";
	cout << "Options:\n";
	cout << "\t-c\t\tThe config file to load.\n";
	cout << "\t-f\t\tRun in the forground.\n";
	cout << "\t-h\t\tDisplay this help.\n";
	cout << "\t-v\t\tPrint the version of this program.\n\n";
}

/**
 * @brief Print daemon usage.
 */
void
version()
{
	cout << program_name << " version " << program_version << "\n";
}

int
run(const string config_path)
{
	int rv;
	OnlyDreamsNow sleeper;
	ml_init_library();

	rv = sleeper.Load(config_path);
	if(rv != 0) {
		goto out;	
	}
 	rv = sleeper.Run();

out:
	ml_cleanup_library();
	return rv;
}
