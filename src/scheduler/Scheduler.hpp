#pragma once

class PipelineScheduler {
public:
  PipelineScheduler()          = default;
  virtual ~PipelineScheduler() = default;

  // disable copy and move
  PipelineScheduler(const PipelineScheduler &)            = delete;
  PipelineScheduler &operator=(const PipelineScheduler &) = delete;
  PipelineScheduler(PipelineScheduler &&)                 = delete;
  PipelineScheduler &operator=(PipelineScheduler &&)      = delete;

  // called by listeners when some of the pipelines are changed
  virtual void onPipelineRebuilt() = 0;
};
