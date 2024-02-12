#pragma once

class Scheduler {
public:
  Scheduler()          = default;
  virtual ~Scheduler() = default;

  // disable copy and move
  Scheduler(const Scheduler &)            = delete;
  Scheduler &operator=(const Scheduler &) = delete;
  Scheduler(Scheduler &&)                 = delete;
  Scheduler &operator=(Scheduler &&)      = delete;

  // called by listeners when some of the pipelines are changed
  virtual void update() = 0;
};