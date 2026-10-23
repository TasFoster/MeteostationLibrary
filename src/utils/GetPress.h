#pragma once

#include "Task.h"
#include "../services/MeteoService.h"

class GetPress : public Task {
 public:
  explicit GetPress(const MeteoService& meteo);
  void execute() override;

 private:
  MeteoService meteo_;
};
