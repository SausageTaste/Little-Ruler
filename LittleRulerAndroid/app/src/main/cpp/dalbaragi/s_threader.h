#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <string>
#include <unordered_map>

#include "s_event.h"


namespace dal {

	class ITask {

	public:
		virtual ~ITask(void) = default;
		virtual void start(void) = 0;

	};


	class ITaskDoneListener {
		
	public:
		virtual ~ITaskDoneListener(void) = default;
		virtual void notifyTask(std::unique_ptr<ITask> task) = 0;

	};


	class TaskGod {

		//////// Attribs ////////

	private:
		std::unordered_map<ITask*, ITaskDoneListener*> m_notificationRecievers;

		//////// Methods ////////

		TaskGod(void);
		~TaskGod(void);

	public:
		static TaskGod& getinst(void);

		void update(void);

		// If client is null, there will be no notification and ITask object will be deleted.
		void orderTask(ITask* const task, ITaskDoneListener* const client);

	private:
		ITaskDoneListener* findNotifiReciever(ITask* task);

	};

}