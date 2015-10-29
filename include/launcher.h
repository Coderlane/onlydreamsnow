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

enum class LauncherDirectionType {
  UP = ML_UP,
  DOWN = ML_DOWN,
  LEFT = ML_LEFT,
  RIGHT = ML_RIGHT
};

typedef std::chrono::duration<int, std::milli> MilliDurationType;

enum class LauncherCommandType { STOP, RESET, FIRE, MOVE, IDLE };

class LauncherCommand
{
  friend class Launcher;
protected:
  LauncherCommand(LauncherCommandType type);
  LauncherCommandType lc_command_type;

public:
  virtual MilliDurationType Run() = 0;
}

class StopCommand : LauncherCommand
{
public:
  StopCommand() : LauncherCommand(LauncherCommandType::STOP){};
  MilliDurationType Run();
}

class ResetCommand : LauncherCommand
{
public:
  ResetCommand() : LauncherCommand(LauncherCommandType::RESET){};
  MilliDurationType Run();
}

class FireCommand : LauncherCommand
{
public:
  FireCommand() : LauncherCommand(LauncherCommandType::FIRE){};
  MilliDurationType Run();
}

class MoveCommand : LauncherCommand
{
public:
  MoveCommand(LauncherDirectionType direction, MilliDurationType duration)
      : LauncherCommand(LauncherCommandType::FIRE), mc_direction(direction),
        mc_duration(duration){};

  MilliDurationType Run();

private:
  LauncherDirectionType mc_direction;
  MilliDurationType mc_duration;
}

struct LauncherException : std::exception {
  char const *what() const throw();
};

/* Launcher */

class Launcher
{
  friend class StopCommand;
  friend class ResetCommand;
  friend class FireCommand;
  friend class MoveCommand;

private:
  ml_launcher_t *ol_launcher = NULL;

  uv_thread_t ol_thread;
  uv_mutex_t ol_mutex;
  uv_async_t ol_async;
  uv_timer_t ol_timer;
  uv_timer_t ol_heartbeat;

  bool ol_running = false;
  bool ol_idle = true;

  static void Heartbeat(uv_timer_t *timer);
  static void TimerDone(uv_timer_t *timer);
  static MilliDurationType Run(void *arg);
  void StartCommand(const LauncherCommand &command);
  void EnqueueCommand(const LauncherCommand &command);

  std::queue<LauncherCommand> ol_commands;

public:
  Launcher(ml_launcher_t *launcher);
  ~Launcher();

  void Fire();
  void Reset();
  void Stop();
  void Move(LauncherDirection direction, int msec_duration);
};


#endif /* LAUNCHER_H */
