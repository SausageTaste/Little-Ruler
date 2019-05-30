#pragma once

#include <list>
#include <string>


namespace dal {

    enum class EventType {
        quit_game = 0,
        window_resize,
        global_fsm_change,
        eof,
    };

    static constexpr unsigned int NUMOF_EVENTTYPE_NORMAL = int(EventType::eof) - int(EventType::quit_game);

    const char* getEventTypeStr(const EventType type);


    class iKeyboardListener {

    public:
        virtual ~iKeyboardListener(void) = default;
        virtual void give(const char* const str) = 0;

    };


    struct EventStatic {
        EventType type;
        int32_t intArg1, intArg2;
        float floatArg1, floatArg2;
        const char* strArg1 = nullptr;
        iKeyboardListener* keyListner = nullptr;
    };


    class iEventHandler {

    protected:
        std::string mHandlerName;

    public:
        virtual ~iEventHandler(void) = default;
        virtual void onEvent(const EventStatic& e) = 0;
        virtual void onEvent_task(void* task) {};
        std::string getHandlerName(void);

    };


    class EventGod {

        //////// Vars ////////

    private:

        std::list<iEventHandler*> mHandlers[NUMOF_EVENTTYPE_NORMAL];

        //////// Funcs ////////

        EventGod(void);
        ~EventGod(void) = default;
        EventGod(EventGod&) = delete;
        EventGod& operator=(EventGod&) = delete;

    public:
        static EventGod& getinst(void);

        void registerHandler(iEventHandler* handler, const EventType type);
        void deregisterHandler(iEventHandler* handler, const EventType type);

        void notifyAll(const EventStatic& e);

    };

}