#include <algorithm>
#include <fstream>
#include <random>
#include <list>
#include <iostream>
#include <memory>
#include <set>
#include <iomanip>

#include "util/pidcontroller.hpp"

static std::mt19937 rng((std::random_device())());
std::uniform_int_distribution<> workerHashRateDistribution(50, 3000);

class SimulatedWorker
{
public:
  typedef std::shared_ptr<SimulatedWorker> Ptr;

  static Ptr create(double maxHashRate, double averageTimeWindow)
  {
    return std::make_shared<SimulatedWorker>(maxHashRate, averageTimeWindow);
  }

  bool operator<(const SimulatedWorker& other)
  {
    return hashRateAverage_ < other.hashRateAverage_;
  }

  bool operator>(const SimulatedWorker& other)
  {
    return hashRateAverage_ > other.hashRateAverage_;
  }

  SimulatedWorker(double maxHashRate, double averageTimeWindow)
    : maxHashRate_(maxHashRate), hashRate_(1 * maxHashRate_), averageTimeWindow_(averageTimeWindow)
  {
  }

  void update(double diffFractionOfTimeWindow)
  {
    randomlyChangeHashrate();
    hashRateAverage_ = (hashRateAverage_ * (1 - diffFractionOfTimeWindow)) +
                       (hashRate_ * diffFractionOfTimeWindow);
  }

  void randomlyChangeHashrate()
  {
    hashRate_ = maxHashRate_ - (std::geometric_distribution<>(0.05))(rng);
  }

  const double maxHashRate_;
  double hashRate_;
  double hashRateAverage_;

  double timeSinceInit_;
  double averageTimeWindow_;
};

bool operator>(const SimulatedWorker::Ptr& first, const SimulatedWorker::Ptr& second)
{
  return *first > *second;
}
                       // D -> P -> I                            P -> D -> I
const double KpWorkers = 1;  // 2. oscillates at 67 with Kd=250;       1. osc. at about 1.8
const double KdWorkers = 0; // 1. oscillates at 560
const double KiWorkers = 0;   //0.0001;
const double KpAverage = 0;  // 2. oscillates at 67 with Kd=250;       1. osc. at about 1.8
const double KdAverage = 0; // 1. oscillates at 560
const double KiAverage = 0;   //0.0001;

class SimulatedPoolFull
{
public:
  typedef std::shared_ptr<SimulatedPoolFull> Ptr;

  static Ptr create(const std::string& name, double averageTimeWindow)
  {
    return std::make_shared<SimulatedPoolFull>(name, averageTimeWindow);
  }

  bool operator<(const SimulatedPoolFull& other)
  {
    return hashRate_ < other.hashRate_;
  }

  SimulatedPoolFull(const std::string& name, double averageTimeWindow)
    : name_(name), timeSinceInit_(0), averageTimeWindow_(averageTimeWindow),
      controllerWorkers_(KpWorkers,KdWorkers,KiWorkers),
      controllerAverage_(KpAverage,KdAverage,KiAverage)
  {
  }

  SimulatedPoolFull& operator()(double maxHashRate)
  {
    workers_.push_back(SimulatedWorker::create(maxHashRate, averageTimeWindow_));
    return *this;
  }

  void update(double dt)
  {
    if (dt == 0) return;

    // calculates weight factor
    timeSinceInit_ += dt;
    double diffFractionOfTimeWindow = dt / std::min(averageTimeWindow_, timeSinceInit_);
    diffFractionOfTimeWindow = std::min(diffFractionOfTimeWindow, 1.0);

    // collects hashrates from workers
    double hashRate = 0;
    for (auto& worker : workers_)
    {
      worker->update(diffFractionOfTimeWindow);
      hashRate += worker->hashRate_;
    }

    // sorts workers by hashrate
    workers_.sort([](auto& first, auto& second){return first->hashRateAverage_ > second->hashRateAverage_;});

    // updates hashrate average
    hashRate_ = hashRate;
    hashRateAverage_ = (hashRateAverage_ * (1 - diffFractionOfTimeWindow)) +
                       (hashRate_ * diffFractionOfTimeWindow);
  }

  void updateController(double average, double dt)
  {
    errorValue_ = controllerWorkers_(average - hashRate_, dt) +
                  controllerAverage_(average - hashRateAverage_, dt);
    errorValueAfterWorkerChange_ = errorValue_;

    hashRateToFill_ = average - (hashRateAverage_ - average);
  }

  std::list<SimulatedWorker::Ptr> workers_;

  std::string name_;

  double hashRate_;
  double hashRateAverage_;

  double timeSinceInit_;
  double averageTimeWindow_;

  ses::util::PidController controllerWorkers_;
  ses::util::PidController controllerAverage_;
  double errorValue_;
  double errorValueAfterWorkerChange_;

  double hashRateToFill_;
};

// complete re-assignment - worst fit
void balance(std::list<SimulatedPoolFull::Ptr> pools, double average, double dt)
{
  std::chrono::time_point<std::chrono::system_clock> begin = std::chrono::system_clock::now();

  // gets all workers from pools
  std::multiset<SimulatedWorker::Ptr, std::greater<SimulatedWorker::Ptr> > availableWorkers;
  for (auto& poolPtr : pools)
  {
    poolPtr->updateController(average, dt);
    for (auto& worker : poolPtr->workers_)
    {
      availableWorkers.insert(worker);
    }
    poolPtr->workers_.clear();
  }

  auto workerIt = availableWorkers.begin();
  while (workerIt != availableWorkers.end())
  {
    pools.sort([](auto& first, auto& second) { return first->hashRateToFill_ >
                                                      second->hashRateToFill_; });
    auto& worker = *workerIt;
    auto& pool = *(pools.front());
    pool.workers_.push_back(worker);
    pool.hashRateToFill_ -= worker->hashRateAverage_;
    ++workerIt;
  }

  for (auto& pool : pools)
  {
    std::cout << "\t| " << pool->name_ << ":"
              << " error=" << std::setw(5) << pool->errorValue_
              << " errorAfter=" << std::setw(5) << pool->errorValueAfterWorkerChange_
              << " numWorkers=" << std::setw(5) << pool->workers_.size();
  }

  double deltaTime =
    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin).count();
  std::cout << "\t| t=" << deltaTime << std::endl;
}

//// overflow re-assignment
//void balance(std::list<SimulatedPoolFull::Ptr> pools, double average, double dt)
//{
//  for (auto& pool : pools) pool->updateController(average, dt);
//
//  pools.sort([](auto& first, auto& second){return first->errorValue_ < second->errorValue_;});
//
//  std::multiset<SimulatedWorker::Ptr, std::greater<SimulatedWorker::Ptr> > availableWorkers;
//  uint32_t numWorkers = 0;
//  for (auto& poolPtr : pools)
//  {
//    auto& pool = *poolPtr;
//    double errorValue = pool.errorValue_;
//    std::cout << "\t| " << pool.name_ << ": error=" << std::setw(5) << errorValue;
//
//    if (errorValue < 0)
//    {
//      uint32_t numRemoved = 0;
//      auto it = pool.workers_.begin();
//      while (it != pool.workers_.end() && errorValue < 0)
//      {
//        auto& worker = *it;
//        if (worker->hashRateAverage_ < -errorValue)
//        {
//          errorValue += worker->hashRateAverage_;
//          availableWorkers.insert(worker);
//          it = pool.workers_.erase(it);
//          ++numRemoved;
//        }
//        else
//        {
//          ++it;
//        }
//      }
//      // also removes last workers in list if the error value exceeds at least half their hashrate
//      while (errorValue < 0 && !pool.workers_.empty())
//      {
//        auto& worker = pool.workers_.back();
//        if (worker->hashRateAverage_ < (-2 * errorValue))
//        {
//          errorValue += worker->hashRateAverage_;
//          availableWorkers.insert(worker);
//          pool.workers_.pop_back();
//          ++numRemoved;
//        }
//        else
//        {
//          break;
//        }
//      }
//      std::cout << " removed=" << numRemoved;
//    }
//    else
//    {
//      auto it = availableWorkers.begin();
//      uint32_t numAdded = 0;
//      while (errorValue > 0 && it != availableWorkers.end())
//      {
//        auto& worker = *it;
//        if (worker->hashRateAverage_ <= errorValue)
//        {
//          pool.workers_.push_back(worker);
//          errorValue -= worker->hashRateAverage_;
//          it = availableWorkers.erase(it);
//          ++numAdded;
//        }
//        else
//        {
//          ++it;
//        }
//      }
//      std::cout << " added=" << numAdded;
//    }
//    numWorkers += pool.workers_.size();
//    std::cout << " worker=" << pool.workers_.size()
//              << " available=" << availableWorkers.size();
//  }
//  numWorkers += availableWorkers.size();
//  std::cout << "\t| total=" << numWorkers << std::endl;
//  for (auto& worker : availableWorkers)
//  {
//    pools.back()->workers_.push_back(worker);
//  }
//}

int main()
{
  const double averageTimeWindow = 300;
  std::list<SimulatedPoolFull::Ptr> pools;
  pools.push_back(SimulatedPoolFull::create("A", averageTimeWindow));
  //(*pools.back())(125)(125)(650)(1000)(2100);
  (*pools.back())(125)(125)(650)(200)(200)(200)(200)(200)(2100);
//  (*pools.back())(200)(3800);
  pools.push_back(SimulatedPoolFull::create("B", averageTimeWindow));
  (*pools.back())(125)(125)(650)(200)(200)(200)(200)(200)(2100);
  pools.push_back(SimulatedPoolFull::create("C", averageTimeWindow));
  pools.push_back(SimulatedPoolFull::create("D", averageTimeWindow));
  pools.push_back(SimulatedPoolFull::create("E", averageTimeWindow));

  // prepares csv outpu
  std::ofstream outFile("PID.csv");
  outFile << "time";
  for (uint32_t i = 0; i < pools.size(); ++i)
  {
    outFile << " hash" << i << " hashAvrg" << i;
  }
  outFile << " avrg" << std::endl;

  const double dt = 20;
  const double end = 2 * 60 * 60;
  for (double time = 0; time <= end; time += dt)
  {
    if (time == 2400)
    {
      (*pools.front())(50)(50)(300)(300)(300)(1000)(2000);//(25000);
    }

    double average = 0;
    outFile << time << " ";
    for (auto& poolPtr : pools)
    {
      auto& pool = *poolPtr;
      pool.update(dt);
      outFile << pool.hashRate_ << " " << pool.hashRateAverage_ << " ";
      average += pool.hashRate_;
    }
    average /= pools.size();
    outFile << average << std::endl;

    balance(pools, average, dt);
  }
}

//class SimulatedPoolSimple
//{
//public:
//  SimulatedPoolSimple(double averageTimeWindow, double hashRate)
//    : averageTimeWindow_(averageTimeWindow), hashRate_(hashRate)
//  {
//
//  }
//
//  void update(double dt)
//  {
//    randomlyChangeHashrate();
//
//    timeSinceInit_ += dt;
//    double diffFractionOfTimeWindow = dt / std::min(averageTimeWindow_, timeSinceInit_);
//    diffFractionOfTimeWindow = std::min(diffFractionOfTimeWindow, 1.0);
//    hashRateAverage_ = (hashRateAverage_ * (1 - diffFractionOfTimeWindow)) +
//                       (hashRate_ * diffFractionOfTimeWindow);
//  }
//
//  void randomlyChangeHashrate()
//  {
//    hashRate_ += std::normal_distribution<double>(-200, 200)(rng);
//  }
//
//  double timeSinceInit_;
//  double averageTimeWindow_;
//  double hashRate_;
//  double hashRateAverage_;
//};

// gnuplot
// set key outside
// plot for [col=2:6] 'PID_test.csv' using 1:col with lines title columnheader

//int main()
//{
//
//  std::ofstream outFile("PID_test.csv");
//
//  outFile << "time avrg hash1 hashAvrg1 hash2 hashAvrg2 control1 control2\n";
//
//  double averageTimeWindow = 600;
//  SimulatedPoolSimple pool1(averageTimeWindow, 1000);
//  SimulatedPoolSimple pool2(averageTimeWindow, 2000);
//  double Kp = .8;
//  double Kd = 50;
//  double Ki = 0.0001;
//  ses::util::PidController controller1(Kp,Kd,Ki);
//  ses::util::PidController controller2(Kp,Kd,Ki);
//
//  outFile << "0 1500 1000 0 2000 0 0 0\n";
//
//  double dt = 20;
//  double chunkSize = 100;
//  double halfChunkSize = chunkSize / 2;
//
//  for (double time = 0; time <= (60 * 60); time += dt)
//  {
//    if (time == 600)
//    {
//      pool1.hashRate_ += 500;
//    }
//    if (time == 2000)
//    {
//      pool2.hashRate_ -= 1000;
//    }
//    pool1.update(dt);
//    pool2.update(dt);
//    double average = (pool1.hashRateAverage_ + pool2.hashRateAverage_) / 2;
//
//    double control1 = controller1(average - pool1.hashRateAverage_, dt);
//    double control2 = controller2(average - pool2.hashRateAverage_, dt);
//
//    double available = 0;
//
//    double tmpControl = control1;
//    while (tmpControl < -halfChunkSize && pool1.hashRate_ > halfChunkSize)
//    {
//      available += chunkSize; pool1.hashRate_ -= chunkSize; tmpControl += chunkSize;
//    }
//    tmpControl = control2;
//    while (tmpControl < -halfChunkSize && pool2.hashRate_ > halfChunkSize)
//    {
//      available += chunkSize; pool2.hashRate_ -= chunkSize; tmpControl += chunkSize;
//    }
//
//    if (pool1.hashRateAverage_ < pool2.hashRateAverage_)
//    {
//      while(available > 0)
//      {
//        available -= chunkSize; pool1.hashRate_ += chunkSize; tmpControl -= chunkSize;
//      }
//    }
//    else
//    {
//      while(available > 0)
//      {
//        available -= chunkSize; pool2.hashRate_ += chunkSize; tmpControl -= chunkSize;
//      }
//    }
//
//    outFile << time << " " << average << " "
//            << pool1.hashRate_ << " " << pool1.hashRateAverage_ << " "
//            << pool2.hashRate_ << " " << pool2.hashRateAverage_ << " "
//            << control1 << " " << control2 << "\n";
//  }
//
//  return 0;
//}