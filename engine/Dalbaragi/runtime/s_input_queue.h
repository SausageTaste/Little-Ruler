#pragma once

#include <array>
#include <string>

#include <glm/vec2.hpp>

#include <d_input_data.h>

#include "s_configs.h"


namespace dal {

    class ISingleUsageQueue {

        //////// vars ////////

    protected:
        static constexpr size_t kCapacity = 100;
        size_t mCurIndex = 0;

        //////// funcs ////////

    protected:
        ISingleUsageQueue(void) = default;
    public:
        virtual ~ISingleUsageQueue(void) = default;
    private:
        ISingleUsageQueue(const ISingleUsageQueue&) = delete;
        ISingleUsageQueue& operator=(const ISingleUsageQueue&) = delete;
        ISingleUsageQueue(ISingleUsageQueue&&) = delete;
        ISingleUsageQueue& operator=(ISingleUsageQueue&&) = delete;

    public:
        void clear(void);
        bool isFull(void) const;
        size_t getSize(void) const;
        size_t getCapacity(void) const;

    };

}


namespace dal {

    class TouchEvtQueueGod : public dal::ISingleUsageQueue {

        //////// vars ////////

    private:
        std::array<TouchEvent, kCapacity> mArray;

        //////// funcs ////////

    public:
        static TouchEvtQueueGod& getinst(void);

        bool emplaceBack(const float x, const float y, const TouchActionType type, const touchID_t id, const float timeSec);
        bool emplaceBack(const float x, const float y, const TouchActionType type, const touchID_t id);

        const TouchEvent& at(const unsigned int index) const;

    };

}


namespace dal {

    //constexpr unsigned int KEY_SPEC_SIZE = int(KeySpec::eof) - int(KeySpec::unknown);


    class KeyboardEvtQueueGod : public dal::ISingleUsageQueue {

    private:
        std::array<KeyboardEvent, kCapacity> mArray;
        KeyStatesRegistry m_states;

    public:
        static KeyboardEvtQueueGod& getinst(void);

        bool emplaceBack(const KeySpec key, const KeyActionType type);
        bool emplaceBack(const KeySpec key, const KeyActionType type, const float timeSec);

        const KeyboardEvent& at(const unsigned int index) const;
        const KeyboardEvent& operator[](const unsigned int index) const {
            return this->mArray[index];
        }

        const KeyStatesRegistry& getKeyStates(void) const {
            return this->m_states;
        }

    };

}