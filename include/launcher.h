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

class CommandReset;
class CommandStop;
class CommandMove;
class CommandFire;

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

  void Start();
  void Done();
};

class CommandMove : public LauncherCommand
{
public:
  CommandMove(Launcher *launcher)
      : LauncherCommand(launcher, CommandType::MOVE, true){};

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
public:
  CommandReset(Launcher *launcher)
      : LauncherCommand(launcher, CommandType::STOP){};
};

class CommandFire : public LauncherCommand
{
public:
  CommandFire(Launcher *launcher)
      : LauncherCommand(launcher, CommandType::STOP){};
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

  CommandReset ol_command_reset;
  CommandMove ol_command_move;
  CommandStop ol_command_stop;
  CommandFire ol_command_fire;

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



#endif /* LAUNCHER_H */
