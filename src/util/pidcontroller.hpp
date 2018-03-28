#pragma once

#include <chrono>

namespace ses {
namespace util {

class PidController
{
public:
  PidController(double proportionalGain, double derivativeGain, double integralGain);

  double operator()(double errorValue, double deltaTime);
  double operator()(double errorValue);

private:
  const double proportionalGain_;
  const double derivativeGain_;
  const double integralGain_;

  double sum_;
  double lastErrorValue_;

  std::chrono::time_point<std::chrono::system_clock> lastErrorValueTimePoint_;
};

} // namespace util
} // namespace ses
