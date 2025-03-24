/******************************************************************************
 * OpenHD
 *
 * Licensed under the GNU General Public License (GPL) Version 3.
 *
 * This software is provided "as-is," without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose, and non-infringement. For details, see the
 * full license in the LICENSE file provided with this source code.
 *
 * Non-Military Use Only:
 * This software and its associated components are explicitly intended for
 * civilian and non-military purposes. Use in any military or defense
 * applications is strictly prohibited unless explicitly and individually
 * licensed otherwise by the OpenHD Team.
 *
 * Contributors:
 * A full list of contributors can be found at the OpenHD GitHub repository:
 * https://github.com/OpenHD
 *
 * © OpenHD, All Rights Reserved.
 ******************************************************************************/

#include <utility>

#ifndef OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_WORK_ITEM_H_
#define OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_WORK_ITEM_H_


// 该代码定义了一个 WorkItem 类，用于表示一个待执行的任务。其主要功能包括：

// 任务封装：

// 将任务（一个 std::function<void()> 类型的函数对象）封装为一个工作项。
// 为任务添加标签（TAG），用于标识任务。
// 为任务指定一个最早执行时间点（m_earliest_execution_time），控制任务的执行时机。
// 任务执行：

// 提供了 execute() 方法，用于执行任务。
// 提供了 ready_to_be_executed() 方法，用于检查当前时间是否已经达到任务的最早执行时间点。
// 设计模式：

// 该类的设计模式借鉴自 MAVSDK，适用于多线程或异步任务队列的场景。
// 任务可以在队列中等待，直到满足执行条件（时间到达）时由工作线程处理。
// 性能优化：

// 使用 std::move 优化了字符串和函数对象的传递，避免不必要的拷贝。
// 该代码的核心作用是 封装任务并控制任务的执行时机，为多线程或异步任务处理提供了一个灵活且高效的机制。



// I took this pattern from MAVSDK.
// A work item refers to some task that is queued up for a worker thread to
// handle
// 该模式借鉴自MAVSDK
// WorkItem表示一个任务，它会被放入队列中，等待工作线程处理
class WorkItem {
 public:
  /**
   * @param work lambda - the work to perform
   * @param earliest_execution_time earliest time point this work item should be
   * handled.
   */
  /**
   * @param tag 任务的标签，用于标识任务
   * @param work 一个lambda表达式，表示需要执行的任务
   * @param earliest_execution_time 任务的最早执行时间点
   */
  explicit WorkItem(
      std::string tag, std::function<void()> work,
      std::chrono::steady_clock::time_point earliest_execution_time)
      : TAG(std::move(tag)),  // 使用std::move优化字符串的传递
        m_earliest_execution_time(earliest_execution_time),// 设置最早执行时间
        m_work(std::move(work)) {} // 使用std::move优化函数对象的传递

  // 执行任务
  void execute() { m_work(); }

  // 检查任务是否已经可以执行
  bool ready_to_be_executed() {
    return std::chrono::steady_clock::now() >= m_earliest_execution_time;
  }
  // 任务的标签，用于标识任务
  const std::string TAG;

 private:
 // 任务的最早执行时间点
  const std::chrono::steady_clock::time_point m_earliest_execution_time;
  // 需要执行的任务，是一个无参数、无返回值的函数对象
  const std::function<void()> m_work;
};

#endif  // OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_WORK_ITEM_H_
