#include "s_threader.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <queue>
#include <array>

#include "s_logger_god.h"

#define DAL_THREAD_COUNT 1


using namespace std;


namespace {

#if DAL_THREAD_COUNT > 0

	template <typename T>
	class ThreadQueue {

	private:
		queue<T*> m_q;
		mutex m_mut;

	public:
		void push(T* t) {
			unique_lock<mutex> lck{ m_mut, defer_lock };

			m_q.push(t);
		}

		T* pop(void) {
			unique_lock<mutex> lck{ m_mut, defer_lock };

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
		ThreadQueue<dal::iTask> m_inQ, m_outQ;

		atomic_bool m_flagExit;

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

		void allocateTask(dal::iTask* const task) {
			m_inQ.push(task);
		}

		dal::iTask* get(void) {
			return m_outQ.pop();
		}

		void askGetTerminated(void) {
			m_flagExit = true;
		}

	};


	constexpr unsigned int NUM_WORKERS = DAL_THREAD_COUNT;
	array<WorkerClass, NUM_WORKERS> g_workers;
	vector<thread> g_threads;

#endif

}


namespace dal {

	iTask::iTask(const char* const name) {
#ifdef _DEBUG
		m_name = name != nullptr ? name : "__noname__";
#endif
	}

	bool iTask::checkNameIs(const char* const str) {
#ifdef _DEBUG
		return m_name == std::string(str);
#else
		return true;
#endif
	}

}


namespace dal {

	TaskGod::TaskGod(void) : m_workCount(0) {

#if DAL_THREAD_COUNT > 0
		for (auto& worker : g_workers) {
			g_threads.emplace_back(ref(worker));
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
				listener->notify(task);
				return;
			}
			else {
				delete task;
			}
		}
#endif

	}

	void TaskGod::orderTask(iTask* const task, iTaskDoneListener* const client) {
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
			client->notify(task);
		}
		else {
			delete task;
		}
#endif

	}

	iTaskDoneListener* TaskGod::findNotifiReciever(iTask* task) {
		auto result = this->m_notificationRecievers.find(task);
		if (result == this->m_notificationRecievers.end())
			return nullptr;
		else
			return result->second;
	}

}