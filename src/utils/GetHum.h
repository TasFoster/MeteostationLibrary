#pragma once

#include "Task.h"
#include "../services/MeteoService.h"

class GetHum : public Task {
 public:
  explicit GetHum(const MeteoService& meteo);
  void execute() override;

 private:
  MeteoService meteo_;
};
