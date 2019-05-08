#include "s_threader.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <queue>
#include <array>

#include "s_logger_god.h"

#define DAL_THREAD_COUNT 1


using namespace std::string_literals;


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

			if (m_q.empty()) {
				return nullptr;
			}
			else {
				T* v = m_q.front();
				m_q.pop();
				return v;
			}
		}

	};


	class WorkerClass {

	private:
		ThreadQueue<dal::ITask> m_inQ, m_outQ;

		std::atomic_bool m_flagExit;

	public:
		void operator()(void) {
			while (true) {
				if (m_flagExit) return;
				this->update();
			}
		}
		
		void update(void) {
			auto task = m_inQ.pop();
			if (task == nullptr) return;

			task->start();

			m_outQ.push(task);
		}

		void allocateTask(dal::ITask* const task) {
			m_inQ.push(task);
		}

		dal::ITask* get(void) {
			return m_outQ.pop();
		}

		void askGetTerminated(void) {
			m_flagExit = true;
		}

	};


	constexpr unsigned int NUM_WORKERS = DAL_THREAD_COUNT;
	std::array<WorkerClass, NUM_WORKERS> g_workers;
	std::vector<std::thread> g_threads;

#endif

}


namespace dal {

	TaskGod::TaskGod(void) : m_workCount(0) {

#if DAL_THREAD_COUNT > 0
		for (auto& worker : g_workers) {
			g_threads.emplace_back(std::ref(worker));
		}
#endif

	}

	TaskGod::~TaskGod(void) {

#if DAL_THREAD_COUNT > 0
		for (auto& worker : g_workers) {
			worker.askGetTerminated();
		}

		for (auto& thread : g_threads) {
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
		for (auto& worker : g_workers) {
			auto task = worker.get();
			if (task == nullptr) continue;

			auto listener = this->findNotifiReciever(task);
			if (listener != nullptr) {
				listener->notifyTask(task);
				return;
			}
			else {
				delete task;
			}
		}
#endif

	}

	void TaskGod::orderTask(ITask* const task, ITaskDoneListener* const client) {
		if (task == nullptr) return;

#if DAL_THREAD_COUNT > 0
		const auto selectedWorkerIndex = m_workCount % NUM_WORKERS;
		g_workers.at(selectedWorkerIndex).allocateTask(task);

		if (client != nullptr) {
			m_notificationRecievers.emplace(task, client);
		}
#else
		task->start();

		if (client != nullptr) {
			client->notifyTask(task);
		}
		else {
			delete task;
		}
#endif

	}

	ITaskDoneListener* TaskGod::findNotifiReciever(ITask* task) {
		auto result = this->m_notificationRecievers.find(task);
		if (result == this->m_notificationRecievers.end())
			return nullptr;
		else
			return result->second;
	}

}