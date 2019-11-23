#pragma once

#include <memory>


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


    class TaskMaster {

    private:
        class Impl;
        Impl* m_pimpl;

    public:
        TaskMaster(const TaskMaster&) = delete;
        TaskMaster& operator=(const TaskMaster&) = delete;
        TaskMaster(TaskMaster&&) = delete;
        TaskMaster& operator=(TaskMaster&&) = delete;

    public:
        TaskMaster(void);
        ~TaskMaster(void);

        void update(void);
        // If client is null, there will be no notification and ITask object will be deleted.
        void orderTask(std::unique_ptr<ITask> task, ITaskDoneListener* const client);

    };

}
