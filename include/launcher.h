/**
 * @file launcher.h
 * @brief
 * @author Travis Lane
 * @version 0.0.1
 * @date 2015-10-27
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <chrono>
#include <exception>
#include <queue>

#include <libmissilelauncher/libmissilelauncher.h>
#include <uv.h>

class Launcher;
class LauncherCommand;

/* Launcher Commands */

enum class DirectionType {
  UP = ML_UP,
  DOWN = ML_DOWN,
  LEFT = ML_LEFT,
  RIGHT = ML_RIGHT
};

enum class CommandType { STOP, RESET, FIRE, MOVE, IDLE };

struct LauncherException : std::exception {
  char const *what() const throw();
};

/* Launcher */

class Launcher
{
private:
  ml_launcher_t *ol_launcher = NULL;

  uv_thread_t ol_thread;
  uv_mutex_t ol_mutex;
  uv_async_t ol_async;
  uv_timer_t ol_timer;
  uv_timer_t ol_heartbeat;

  bool ol_running = false;
  bool ol_interruptable = true;
  bool ol_idle = true;

  static void Heartbeat(uv_timer_t *timer, int status);
  static void TimerDone(uv_timer_t *timer, int status);
  static void Run(void *arg);

  void EnqueueCommand(CommandType command,
                      DirectionType direction = DirectionType::UP,
                      int duration = 0);

  void StartCommand(CommandType command, DirectionType direction, int duration);

  void StartCommandImpl(CommandType command, DirectionType direction,
                        int duration);

  CommandType ol_next_command = CommandType::IDLE;
  DirectionType ol_next_direction = DirectionType::UP;
  int ol_next_duration = 0;

  CommandType ol_current_command = CommandType::IDLE;
  DirectionType ol_current_direction = DirectionType::UP;
  int ol_current_duration = 0;

public:
  Launcher(ml_launcher_t *launcher);
  ~Launcher();

  void Fire();
  void Reset();
  void Stop();
  void Move(DirectionType direction, int msec_duration);
};

#endif /* LAUNCHER_H */
