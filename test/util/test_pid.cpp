#include <algorithm>
#include <fstream>

#include "util/pidcontroller.hpp"

class SimulatedPool
{
public:
  SimulatedPool(double averageTimeWindow, double hashRate)
    : averageTimeWindow_(averageTimeWindow), hashRate_(hashRate)
  {

  }

  void update(double dt)
  {
    //randomlyChangeHashrate();

    timeSinceInit_ += dt;
    double diffFractionOfTimeWindow = dt / std::min(averageTimeWindow_, timeSinceInit_);
    diffFractionOfTimeWindow = std::min(diffFractionOfTimeWindow, 1.0);
    hashRateAverage_ = (hashRateAverage_ * (1 - diffFractionOfTimeWindow)) +
                       (hashRate_ * diffFractionOfTimeWindow);
  }

  void randomlyChangeHashrate()
  {
    double random = std::rand() - (RAND_MAX / 2);
    random /= RAND_MAX;
    random *= .1;
    hashRate_ += hashRate_ * random;
  }

  double timeSinceInit_;
  double averageTimeWindow_;
  double hashRate_;
  double hashRateAverage_;
};

// gnuplot
// set key outside
// plot for [col=2:6] 'PID_test.csv' using 1:col with lines title columnheader

int main()
{
  std::srand(std::time(nullptr));
  std::ofstream outFile("PID_test.csv");

  outFile << "time avrg hash1 hashAvrg1 hash2 hashAvrg2 control1 control2\n";

  double averageTimeWindow = 600;
  SimulatedPool pool1(averageTimeWindow, 1000);
  SimulatedPool pool2(averageTimeWindow, 2000);
  double Kp = .8;
  double Kd = 50;
  double Ki = 0.0001;
  ses::util::PidController controller1(Kp,Kd,Ki);
  ses::util::PidController controller2(Kp,Kd,Ki);

  outFile << "0 1500 1000 0 2000 0 0 0\n";

  double dt = 20;
  double chunkSize = 100;
  double halfChunkSize = chunkSize / 2;

  for (double time = 0; time <= (60 * 60); time += dt)
  {
    if (time == 600)
    {
      pool1.hashRate_ += 500;
    }
    if (time == 2000)
    {
      pool2.hashRate_ -= 1000;
    }
    pool1.update(dt);
    pool2.update(dt);
    double average = (pool1.hashRateAverage_ + pool2.hashRateAverage_) / 2;

    double control1 = controller1(average - pool1.hashRateAverage_, dt);
    double control2 = controller2(average - pool2.hashRateAverage_, dt);

    double available = 0;

    double tmpControl = control1;
    while (tmpControl < -halfChunkSize && pool1.hashRate_ > halfChunkSize)
    {
      available += chunkSize; pool1.hashRate_ -= chunkSize; tmpControl += chunkSize;
    }
    tmpControl = control2;
    while (tmpControl < -halfChunkSize && pool2.hashRate_ > halfChunkSize)
    {
      available += chunkSize; pool2.hashRate_ -= chunkSize; tmpControl += chunkSize;
    }

    if (pool1.hashRateAverage_ < pool2.hashRateAverage_)
    {
      while(available > 0)
      {
        available -= chunkSize; pool1.hashRate_ += chunkSize; tmpControl -= chunkSize;
      }
    }
    else
    {
      while(available > 0)
      {
        available -= chunkSize; pool2.hashRate_ += chunkSize; tmpControl -= chunkSize;
      }
    }

    outFile << time << " " << average << " "
            << pool1.hashRate_ << " " << pool1.hashRateAverage_ << " "
            << pool2.hashRate_ << " " << pool2.hashRateAverage_ << " "
            << control1 << " " << control2 << "\n";
  }

  return 0;
}