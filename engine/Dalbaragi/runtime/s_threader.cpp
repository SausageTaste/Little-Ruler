#include "s_threader.h"

#include <thread>
#include <queue>
#include <array>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

#include <spdlog/fmt/fmt.h>

#include <d_logger.h>

#include "u_timer.h"


#define DAL_THREAD_COUNT 1


using namespace fmt::literals;


namespace dal {

#if DAL_THREAD_COUNT > 0

    class TaskMaster::Impl {

    private:
        class TaskQueue {

        private:
            std::queue<std::unique_ptr<dal::ITask>> m_q;
            std::mutex m_mut;

        public:
            void push(std::unique_ptr<dal::ITask> t) {
                std::unique_lock<std::mutex> lck{ m_mut, std::defer_lock };

                m_q.push(std::move(t));
            }

            std::unique_ptr<dal::ITask> pop(void) {
                std::unique_lock<std::mutex> lck{ m_mut, std::defer_lock };

                if ( m_q.empty() ) {
                    return nullptr;
                }
                else {
                    auto v = std::move(m_q.front());
                    m_q.pop();
                    return v;
                }
            }

            size_t getSize(void) {
                std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

                return m_q.size();
            }

        };


        class WorkerClass {

        private:
            TaskQueue* m_inQ = nullptr;
            TaskQueue* m_outQ = nullptr;

            std::atomic_bool m_flagExit;

        public:
            WorkerClass(const WorkerClass&) = delete;
            WorkerClass& operator=(const WorkerClass&) = delete;

        public:
            WorkerClass(TaskQueue& inQ, TaskQueue& outQ)
                : m_inQ(&inQ)
                , m_outQ(&outQ)
                , m_flagExit(ATOMIC_VAR_INIT(false))
            {

            }

            WorkerClass(WorkerClass&& other) noexcept {
                this->m_inQ = other.m_inQ;
                this->m_outQ = other.m_outQ;
                this->m_flagExit.store(other.m_flagExit.load());

                other.m_inQ = nullptr;
                other.m_outQ = nullptr;
            }
            WorkerClass& operator=(WorkerClass&& other) noexcept {
                this->m_inQ = other.m_inQ;
                this->m_outQ = other.m_outQ;
                this->m_flagExit = other.m_flagExit.load();

                other.m_inQ = nullptr;
                other.m_outQ = nullptr;

                return *this;
            }

            void operator()(void) {
                while ( true ) {
                    if ( this->m_flagExit ) {
                        dalVerbose("Worker retired.");
                        return;
                    }

                    auto task = this->m_inQ->pop();
                    if ( task == nullptr ) {
                        continue;
                    }

                    task->start();
                    this->m_outQ->push(std::move(task));

                    dal::sleepFor(0.1f);
                }
            }

            void askGetTerminated(void) {
                this->m_flagExit = true;
            }

        };


        class TaskRegistry {

        private:
            std::unordered_map<const void*, dal::ITaskDoneListener*> m_notifiRecievers;
            std::unordered_set<const void*> m_firedTasks;

        public:
            TaskRegistry(const TaskRegistry&) = delete;
            TaskRegistry& operator=(const TaskRegistry&) = delete;
            TaskRegistry(TaskRegistry&&) noexcept = default;
            TaskRegistry& operator=(TaskRegistry&&) noexcept = default;

        public:
            TaskRegistry(void) = default;

            void registerTask(const dal::ITask* const task, dal::ITaskDoneListener* const listener) {
                const auto ptr = reinterpret_cast<const void*>(task);

                if ( listener != nullptr ) {
                    this->m_notifiRecievers.emplace(ptr, listener);
                }
                else {
                    this->m_firedTasks.emplace(ptr);
                }
            }

            dal::ITaskDoneListener* unregister(const dal::ITask* const task) {
                const auto ptr = reinterpret_cast<const void*>(task);

                {
                    auto iter = this->m_notifiRecievers.find(ptr);
                    if ( iter != this->m_notifiRecievers.end() ) {
                        const auto listener = iter->second;
                        this->m_notifiRecievers.erase(iter);
                        return listener;
                    }
                }

                {
                    auto iter = this->m_firedTasks.find(ptr);
                    if ( this->m_firedTasks.end() != iter ) {
                        this->m_firedTasks.erase(iter);
                        return nullptr;
                    }
                }

                dalAbort("A task returned to TaskGod but it doesn't recognize it.");
            }

        };

    private:
        TaskQueue m_inQ, m_outQ;

        std::vector<WorkerClass> m_workers;
        std::vector<std::thread> m_threads;

        TaskRegistry m_registry;

    public:
        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;

    public:
        Impl(const size_t threadCount) {
            this->m_workers.reserve(threadCount);
            this->m_threads.reserve(threadCount);

            for ( size_t i = 0; i < threadCount; ++i ) {
                this->m_workers.emplace_back(this->m_inQ, this->m_outQ);
            }

            for ( auto& worker : this->m_workers ) {
                this->m_threads.emplace_back(std::ref(worker));
            }
        }

        ~Impl(void) {
            for ( auto& worker : this->m_workers ) {
                worker.askGetTerminated();
            }

            for ( auto& thread : this->m_threads ) {
                thread.join();
            }
        }

        void update(void) {
            auto task = this->m_outQ.pop();
            if ( nullptr == task ) {
                return;
            }

            auto listener = this->m_registry.unregister(task.get());
            if ( nullptr != listener ) {
                listener->notifyTask(std::move(task));
                return;
            }
        }

        void orderTask(std::unique_ptr<ITask> task, ITaskDoneListener* const client) {
            this->m_registry.registerTask(task.get(), client);
            this->m_inQ.push(std::move(task));
        }

    };

#endif


    TaskMaster::TaskMaster(void) {

#if DAL_THREAD_COUNT > 0
            this->m_pimpl = new Impl{ DAL_THREAD_COUNT };
#else
            this->m_pimpl = nullptr;
#endif

    }

    TaskMaster::~TaskMaster(void) {

#if DAL_THREAD_COUNT > 0
        delete this->m_pimpl;
        this->m_pimpl = nullptr;
#endif

    }

    void TaskMaster::update(void) {

#if DAL_THREAD_COUNT > 0
        this->m_pimpl->update();
#endif

    }

    void TaskMaster::orderTask(std::unique_ptr<ITask> task, ITaskDoneListener* const client) {
        if ( task == nullptr ) {
            return;
        }

#if DAL_THREAD_COUNT > 0
        this->m_pimpl->orderTask(std::move(task), client);
#else
        task->start();

        if ( client != nullptr ) {
            client->notifyTask(std::move(task));
        }
#endif

    }

}
