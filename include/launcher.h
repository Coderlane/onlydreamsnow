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
  static void RunLoop(void *arg);



  void EnqueueCommand(CommandType command,
                      DirectionType direction = DirectionType::UP,
                      int duration = 0);

  LauncherCommand *ol_command_current = nullptr;
  LauncherCommand *ol_command_next = nullptr;

public:
  Launcher(ml_launcher_t *launcher);
  ~Launcher();

  void Fire();
  void Reset();
  void Stop();
  void Move(DirectionType direction, int msec_duration);

  static void CommandStart(Launcher *launcher, LauncherCommand *command);
  static void CommandDone(Launcher *launcher, LauncherCommand *command);
};


/* Launcher Commands */
class LauncherCommand
{
public:
  LauncherCommand(Launcher *launcher, CommandType type,
                  bool interruptable = false)
  {
    lc_launcher = launcher;
    lc_type = type;
    lc_interruptable = interruptable;
  }

  bool
  IsInterruptable()
  {
    return lc_interruptable;
  }

  CommandType
  GetType()
  {
    return lc_type;
  }

protected:
  Launcher *lc_launcher;
  CommandType lc_type;
  bool lc_interruptable;

  virtual void Start() {
    Launcher::CommandStart(lc_launcher, this);
  }

  virtual void Done() {
    Launcher::CommandDone(lc_launcher, this);
  }
};

class CommandMove : public LauncherCommand
{
public:
  CommandMove(Launcher *launcher, DirectionType direction, int duration)
      : LauncherCommand(launcher, CommandType::MOVE, true)
  {
    cm_direction = direction;
    cm_duration = duration;
  };

private:
  DirectionType cm_direction;
  int cm_duration;
};

class CommandStop : public LauncherCommand
{
public:
  CommandStop(Launcher *launcher)
      : LauncherCommand(launcher, CommandType::STOP){};
};

class CommandReset : public LauncherCommand
{
  CommandReset(Launcher *launcher)
      : LauncherCommand(launcher, CommandType::STOP){};
};

class CommandFire : public LauncherCommand
{
  CommandFire(Launcher *launcher)
      : LauncherCommand(launcher, CommandType::STOP){};
};





#endif /* LAUNCHER_H */
