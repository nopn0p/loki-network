#ifndef LLARP_CODEL_QUEUE_HPP
#define LLARP_CODEL_QUEUE_HPP
#include <llarp/time.h>
#include <cmath>
#include <functional>
#include <mutex>
#include <queue>
#include <string>

namespace llarp
{
  namespace util
  {
    template < typename T, typename GetTime, typename PutTime,
               llarp_time_t dropMs = 20, llarp_time_t initialIntervalMs = 100 >
    struct CoDelQueue
    {
      CoDelQueue(const std::string& name) : m_name(name)
      {
      }

      struct CoDelCompare
      {
        bool
        operator()(const T& left, const T& right) const
        {
          return GetTime()(left) < GetTime()(right);
        }
      };

      void
      Put(const T& i)
      {
        std::unique_lock< std::mutex > lock(m_QueueMutex);
        // llarp::Info("CoDelQueue::Put - adding item, queue now has ",
        // m_Queue.size(), " items at ", getTime(*item));
        PutTime()(i);
        m_Queue.push(i);
        if(firstPut == 0)
          firstPut = GetTime()(i);
      }

      void
      Process(std::queue< T >& result)
      {
        llarp_time_t lowest = 0xFFFFFFFFFFFFFFFFUL;
        // auto start          = llarp_time_now_ms();
        // llarp::Info("CoDelQueue::Process - start at ", start);
        std::unique_lock< std::mutex > lock(m_QueueMutex);
        auto start = firstPut;
        while(m_Queue.size())
        {
          // llarp::Info("CoDelQueue::Process - queue has ", m_Queue.size());
          const auto& item = m_Queue.top();
          auto dlt         = start - GetTime()(item);
          // llarp::Info("CoDelQueue::Process - dlt ", dlt);
          lowest = std::min(dlt, lowest);
          if(m_Queue.size() == 1)
          {
            // llarp::Info("CoDelQueue::Process - single item: lowest ", lowest,
            // " dropMs: ", dropMs);
            if(lowest > dropMs)
            {
              // drop
              nextTickInterval += initialIntervalMs / std::sqrt(++dropNum);
              llarp::Warn("CoDel queue ", m_name, " drop ", nextTickInterval,
                          " ms next interval lowest=", lowest);
              delete item;
              m_Queue.pop();
              break;
            }
            else
            {
              nextTickInterval = initialIntervalMs;
              dropNum          = 0;
            }
          }
          // llarp::Info("CoDelQueue::Process - passing");
          result.push(item);
          m_Queue.pop();
        }
        firstPut = 0;
      }

      llarp_time_t firstPut         = 0;
      size_t dropNum                = 0;
      llarp_time_t nextTickInterval = initialIntervalMs;
      std::mutex m_QueueMutex;
      std::priority_queue< T, std::vector< T >, CoDelCompare > m_Queue;
      std::string m_name;
    };
  }  // namespace util
}  // namespace llarp

#endif
