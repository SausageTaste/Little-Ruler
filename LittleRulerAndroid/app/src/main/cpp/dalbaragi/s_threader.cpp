#include "s_threader.h"

#include <thread>
#include <queue>
#include <array>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "u_timer.h"

#define DAL_THREAD_COUNT 1


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

#if DAL_THREAD_COUNT > 0

    template <typename T>
    class ThreadQueue {

    private:
        std::queue<T*> m_q;
        std::mutex m_mut;

    public:
        void push(T* t) {
            std::unique_lock<std::mutex> lck{ m_mut, std::defer_lock };

            m_q.push(t);
        }

        T* pop(void) {
            std::unique_lock<std::mutex> lck{ m_mut, std::defer_lock };

            if ( m_q.empty() ) {
                return nullptr;
            }
            else {
                T* v = m_q.front();
                m_q.pop();
                return v;
            }
        }

        size_t getSize(void) {
            std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

            return m_q.size();
        }
    };

    ThreadQueue<dal::ITask> g_inQ, g_outQ;


    class WorkerClass {

    private:
        std::atomic_bool m_flagExit;

    public:
        void operator()(void) {
            while ( true ) {
                if ( this->m_flagExit ) return;

                auto task = g_inQ.pop();
                if ( task == nullptr ) {
                    continue;
                }

                task->start();
                g_outQ.push(task);

                dal::sleepFor(0.1f);
            }
        }

        void askGetTerminated(void) {
            this->m_flagExit = true;
        }

    };


    constexpr unsigned int NUM_WORKERS = DAL_THREAD_COUNT;
    std::array<WorkerClass, NUM_WORKERS> g_workers;
    std::vector<std::thread> g_threads;

#endif

}


namespace dal {

    TaskGod::TaskGod(void) {

#if DAL_THREAD_COUNT > 0
        for ( auto& worker : g_workers ) {
            g_threads.emplace_back(std::ref(worker));
        }
#endif

    }

    TaskGod::~TaskGod(void) {

#if DAL_THREAD_COUNT > 0
        for ( auto& worker : g_workers ) {
            worker.askGetTerminated();
        }

        for ( auto& thread : g_threads ) {
            thread.join();
        }
#endif

    }

    TaskGod& TaskGod::getinst(void) {
        static TaskGod inst;
        return inst;
    }

    void TaskGod::update(void) {

#if DAL_THREAD_COUNT > 0
        auto task = g_outQ.pop();
        if ( nullptr == task ) return;

        auto listener = this->findNotifiReciever(task);
        if ( nullptr != listener ) {
            listener->notifyTask(std::unique_ptr<ITask>{ task });
            return;
        }
#endif

    }

    void TaskGod::orderTask(ITask* const task, ITaskDoneListener* const client) {
        if ( task == nullptr ) return;

#if DAL_THREAD_COUNT > 0
        g_inQ.push(task);

        if ( client != nullptr ) {
            m_notificationRecievers.emplace(task, client);
        }
        else {
            this->m_firedTasks.emplace(task);
        }
#else
        task->start();

        if ( client != nullptr ) {
            client->notifyTask(std::unique_ptr<ITask>{ task });
        }
        else {
            delete task;
        }
#endif

    }

    ITaskDoneListener* TaskGod::findNotifiReciever(ITask* const task) {
        {
            auto iter = this->m_notificationRecievers.find(task);
            if ( iter != this->m_notificationRecievers.end() ) {
                const auto listener = iter->second;
                this->m_notificationRecievers.erase(iter);
                return listener;
            }
        }

        {
            auto iter = this->m_firedTasks.find(task);
            if ( this->m_firedTasks.end() != iter ) {
                this->m_firedTasks.erase(iter);
                return nullptr;
            }
        }

        dalAbort("A task returned to TaskGod but it doesn't recognize it.");
    }

}