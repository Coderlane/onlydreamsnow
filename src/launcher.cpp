

#include <iostream>

#include "launcher.h"

using namespace std;

Launcher::Launcher(ml_launcher_t *launcher)
{
  int rv;

  if(launcher == NULL) {
    throw "The ml_launcher was null";
  }

  rv = ml_launcher_claim(launcher);
  if(rv != ML_OK) {
    throw "Failed to claim launcher.";
  }

  ol_launcher = launcher;
  ol_running = true;

  uv_mutex_init(&(ol_mutex));
  uv_thread_create(&(ol_thread), Launcher::Run, this);
}

Launcher::~Launcher()
{
  if(ol_running) {
    uv_unref((uv_handle_t *) & (ol_async));
    ol_running = false;
    uv_thread_join(&(ol_thread));
  }

  if(ol_launcher != NULL) {
    ml_launcher_stop(ol_launcher);
    ml_launcher_unclaim(ol_launcher);
  }
}

void Launcher::Heartbeat(uv_timer_t *timer) {
	//cerr << "Heartbeat.\n";
}

void Launcher::Run(void *arg)
{
  uv_loop_t loop;
  Launcher *launcher = static_cast<Launcher *>(arg);

  uv_loop_init(&loop);
  uv_timer_init(&loop, &(launcher->ol_timer));
	uv_timer_init(&loop, &(launcher->ol_heartbeat));
  uv_async_init(&loop, &(launcher->ol_async), NULL);

	uv_timer_start(&(launcher->ol_heartbeat), Launcher::Heartbeat, 10, 10);

  launcher->ol_timer.data = launcher;

  while(launcher->ol_running) {
    // Run the launcher loop.
    uv_run(&loop, UV_RUN_ONCE);
  }
}

void
Launcher::RunCommand(Launcher *launcher)
{
  int rv = 0;

  switch(launcher->ol_command) {
  case LauncherCommand::MOVE:
		launcher->ol_idle = false;
		cerr << "Move " << (int) launcher->ol_direction << " for ";
		cerr << launcher->ol_duration << endl;

    rv = ml_launcher_move(launcher->ol_launcher,
                          (ml_launcher_direction) launcher->ol_direction);

    uv_timer_start(&(launcher->ol_timer),
                   Launcher::TimerDone,
                   launcher->ol_duration, 0);
    break;
  case LauncherCommand::FIRE:
		launcher->ol_idle = false;
    rv = ml_launcher_fire(launcher->ol_launcher);
    uv_timer_start(&(launcher->ol_timer), Launcher::TimerDone, 5000, 0);
    break;
  case LauncherCommand::STOP:
    rv = ml_launcher_stop(launcher->ol_launcher);
    break;
  case LauncherCommand::RESET:
    rv = ml_launcher_zero(launcher->ol_launcher);
    ml_launcher_move_mseconds(launcher->ol_launcher, ML_UP, 400);
    break;
  default:
    rv = 0;
    break;
  }

	launcher->ol_command = LauncherCommand::IDLE;

  if(rv != ML_OK) {
    // Failed to run command.
  }
}

void 
Launcher::TimerDone(uv_timer_t *timer)
{
  Launcher *launcher = (Launcher *) timer->data;

	cerr << "Stop.\n";

	uv_mutex_lock(&(launcher->ol_mutex));

  ml_launcher_stop(launcher->ol_launcher);

	launcher->ol_idle = true;

	if(launcher->ol_command != LauncherCommand::IDLE) {
		RunCommand(launcher);
	}

	uv_mutex_unlock(&(launcher->ol_mutex));
}

void
Launcher::Fire()
{
  uv_mutex_lock(&ol_mutex);

  // Set this as the next command.
  ol_command = LauncherCommand::FIRE;

  if(ol_idle) {
    // Launcher was ready for a command.
    Launcher::RunCommand(this);
  }

  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::Reset()
{
  uv_mutex_lock(&ol_mutex);

  // Set this as the next command.
  ol_command = LauncherCommand::RESET;

  if(ol_idle) {
    // Launcher was ready for a command.
    Launcher::RunCommand(this);
  }

  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::Stop()
{
  uv_mutex_lock(&ol_mutex);

  // Set this as the next command.
  ol_command = LauncherCommand::STOP;

  if(ol_idle) {
    // Launcher was ready for a command.
    Launcher::RunCommand(this);
  }

  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::Move(LauncherDirection direction, int duration)
{
  uv_mutex_lock(&ol_mutex);

  ol_direction = direction;
  ol_duration = duration;
  ol_command = LauncherCommand::MOVE;

  if(ol_idle) {
    // Launcher was ready for a command.
    Launcher::RunCommand(this);
  }

  uv_mutex_unlock(&ol_mutex);
}
