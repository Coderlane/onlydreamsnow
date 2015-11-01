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

typedef std::chrono::duration<int, std::milli> MilliDurationType;

enum class CommandType { STOP, RESET, FIRE, MOVE, IDLE };

class LauncherCommand
{
  friend class Launcher;
protected:
  LauncherCommand(CommandType type, MilliDurationType duration,
                  bool interruptable = false)
  {
    lc_command_type = type;
    lc_duration = duration;
    lc_interruptable = interruptable;
  }

  MilliDurationType lc_duration;
  CommandType lc_command_type;
  bool lc_interruptable;

public:
  virtual void Run() = 0;

  /**
   * @brief Check to see if the command is interruptable or not.
   *
   * @return True/False can the command be interrupted.
   */
  bool IsInterruptable() {
    return lc_interruptable;
  }

  /**
   * @brief Get how long the command should run for.
   *
   * @return How long the command should take to run.
   */
  MilliDurationType GetDuration() {
    return lc_duration;
  }
};

class StopCommand : public LauncherCommand
{
public:
  StopCommand()
      : LauncherCommand(CommandType::STOP, MilliDurationType(100)){};
  virtual void Run();
};

class ResetCommand : public LauncherCommand
{
public:
  ResetCommand()
      : LauncherCommand(CommandType::RESET, MilliDurationType(10000)){};
  virtual void Run();
};

class FireCommand : public LauncherCommand
{
public:
  FireCommand()
      : LauncherCommand(CommandType::FIRE, MilliDurationType(5000)){};
  virtual void Run();
};

class MoveCommand : public LauncherCommand
{
public:
  MoveCommand(DirectionType direction, MilliDurationType duration)
      : LauncherCommand(CommandType::FIRE, duration, true)
  {
    mc_direction = direction;
  }

  virtual void Run();

private:
  DirectionType mc_direction;
};

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
  void StartCommand(LauncherCommand *command);
  void EnqueueCommand(LauncherCommand *command);

  LauncherCommand *ol_next_command = nullptr;

public:
  Launcher(ml_launcher_t *launcher);
  ~Launcher();

  //void Run(void *arg);

  void Fire();
  void Reset();
  void Stop();
  void Move(DirectionType direction, MilliDurationType msec_duration);
};

#endif /* LAUNCHER_H */
