#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>


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
        std::unordered_set<ITask*> m_firedTasks;

        //////// Methods ////////

    private:
        TaskGod(void);
        ~TaskGod(void);

    public:
        TaskGod(const TaskGod&) = delete;
        TaskGod(TaskGod&&) = delete;
        TaskGod& operator=(const TaskGod&) = delete;
        TaskGod& operator=(TaskGod&&) = delete;

    public:
        static TaskGod& getinst(void) {
            static TaskGod inst;
            return inst;
        }

        void update(void);

        // If client is null, there will be no notification and ITask object will be deleted.
        void orderTask(ITask* const task, ITaskDoneListener* const client);

    private:
        ITaskDoneListener* findNotifiReciever(ITask* const task);

    };

}