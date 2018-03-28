#include "util/pidcontroller.hpp"

namespace ses {
namespace util {

PidController::PidController(double proportionalGain, double derivativeGain, double integralGain)
    : proportionalGain_(proportionalGain), derivativeGain_(derivativeGain),
      integralGain_(integralGain), sum_(0), lastErrorValue_(0),
      lastErrorValueTimePoint_(std::chrono::system_clock::now())
{
}

double PidController::operator()(double errorValue, double deltaTime)
{
  sum_ += errorValue;
  double controlVariable = proportionalGain_ * errorValue +
                           integralGain_ * deltaTime * sum_ +
                           derivativeGain_ * (errorValue - lastErrorValue_) / deltaTime;
  lastErrorValue_ = errorValue;
  return controlVariable;
}

double PidController::operator()(double errorValue)
{
  std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  double deltaTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - lastErrorValueTimePoint_).count();
  lastErrorValueTimePoint_ = now;

  return (*this)(errorValue, deltaTime);
}

} // namespace util
} // namespace ses
