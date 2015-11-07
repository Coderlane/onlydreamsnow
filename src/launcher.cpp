

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

void Launcher::Heartbeat(uv_timer_t *, int) {
	//cerr << "Heartbeat.\n";
}

void Launcher::Run(void *arg)
{
  uv_loop_t *loop = uv_loop_new();
  Launcher *launcher = static_cast<Launcher *>(arg);

  uv_timer_init(loop, &(launcher->ol_timer));
	uv_timer_init(loop, &(launcher->ol_heartbeat));
  uv_async_init(loop, &(launcher->ol_async), NULL);

	uv_timer_start(&(launcher->ol_heartbeat), Launcher::Heartbeat, 10, 10);

  launcher->ol_timer.data = launcher;

  while(launcher->ol_running) {
    // Run the launcher loop.
    uv_run(loop, UV_RUN_ONCE);
  }

  uv_loop_delete(loop);
}

void
Launcher::TimerDone(uv_timer_t *timer, int)
{
  Launcher *launcher = (Launcher *) timer->data;

	uv_mutex_lock(&(launcher->ol_mutex));

  ml_launcher_stop(launcher->ol_launcher);

	launcher->ol_idle = true;

	uv_mutex_unlock(&(launcher->ol_mutex));
}

void
Launcher::EnqueueCommand(CommandType command, DirectionType direction,
                         int duration)
{
  uv_mutex_lock(&ol_mutex);
  if (ol_idle) {
    StartCommandImpl(command, direction, duration);
  }


  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::StartCommand(CommandType command, DirectionType direction,
                         int duration)
{
  uv_mutex_lock(&ol_mutex);
  StartCommandImpl(command, direction, duration);
  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::StartCommandImpl(CommandType command, DirectionType direction,
                         int duration)
{
  switch(command) {
    case CommandType::STOP:
      ml_launcher_stop(ol_launcher);
      break;
    case CommandType::RESET:
      // TODO: Implement...
      break;
    case CommandType::FIRE:
      ml_launcher_fire(ol_launcher);
      break;
    case CommandType::MOVE:
      ml_launcher_move(ol_launcher, (ml_launcher_direction)direction);
      // TODO: start timer.
      break;
     default:
       break;
  }
}

void
Launcher::Fire()
{
  uv_mutex_lock(&ol_mutex);
  ol_interruptable = false;

  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::Reset()
{
  uv_mutex_lock(&ol_mutex);
  ol_interruptable = false;

  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::Stop()
{
  uv_mutex_lock(&ol_mutex);
  ol_interruptable = false;


  uv_mutex_unlock(&ol_mutex);
}

void
Launcher::Move(DirectionType direction, int duration)
{
  uv_mutex_lock(&ol_mutex);
  ol_interruptable = true;

  uv_mutex_unlock(&ol_mutex);
}
