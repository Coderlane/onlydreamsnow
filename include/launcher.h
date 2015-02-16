


#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <exception>

#include <libmissilelauncher/libmissilelauncher.h>
#include <uv.h>


struct LauncherException : std::exception {
  char const *what() const throw();
};

enum class LauncherDirection
{
  UP = ML_UP,
  DOWN = ML_DOWN,
  LEFT = ML_LEFT,
  RIGHT = ML_RIGHT
};

enum class LauncherCommand
{
  STOP,
  RESET,
  FIRE,
  MOVE,
  IDLE
};

class Launcher
{
private:
  ml_launcher_t *ol_launcher = NULL;

  uv_loop_t ol_loop;
  uv_thread_t ol_thread;
  uv_mutex_t ol_mutex;
  uv_async_t ol_async;
	uv_timer_t ol_timer;

  bool ol_running = false;
	bool ol_idle = true;

	static void TimerDone(uv_timer_t *timer);
  static void Run(void *arg);
  static void RunCommand(Launcher *launcher);


  LauncherCommand ol_command = LauncherCommand::IDLE;
  LauncherDirection ol_direction = LauncherDirection::UP;
  int ol_duration = 0;

public:
  Launcher(ml_launcher_t *launcher);
  ~Launcher();

  void Fire();
  void Reset();
  void Stop();
  void Move(LauncherDirection direction, int msec_duration);
};


#endif /* LAUNCHER_H */
