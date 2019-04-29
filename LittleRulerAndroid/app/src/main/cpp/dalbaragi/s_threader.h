#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <string>
#include <unordered_map>

#include "s_event.h"


namespace dal {

	class iTask {

	private:
#ifdef _DEBUG
		std::string m_name;
#endif

	public:
		iTask(const char* const name);
		virtual ~iTask(void) = default;
		virtual void start(void) = 0;

		bool checkNameIs(const char* const str);

	};


	class iTaskDoneListener {
		
	public:
		virtual ~iTaskDoneListener(void) = default;
		virtual void notify(iTask* const task) = 0;

	};


	class TaskGod {

		//////// Attribs ////////

	private:
		std::unordered_map<iTask*, iTaskDoneListener*> m_notificationRecievers;
		uint64_t m_workCount;

		//////// Methods ////////

		TaskGod(void);
		~TaskGod(void);

	public:
		static TaskGod& getinst(void);

		void update(void);

		// If client is null, there will be no notification and iTask object will be deleted.
		void orderTask(iTask* const task, iTaskDoneListener* const client);

	private:
		iTaskDoneListener* findNotifiReciever(iTask* task);

	};

}