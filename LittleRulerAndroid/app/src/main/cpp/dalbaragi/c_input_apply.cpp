#include "c_input_apply.h"

#include <string>
#include <array>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "s_input_queue.h"
#include "s_configs.h"
#include "s_event.h"
#include "u_timer.h"
#include "o_widget_texview.h"


using namespace std::string_literals;


namespace {

    void toggleGameState(void) {
        dal::EventStatic e;
        e.type = dal::EventType::global_fsm_change;

        const auto curState = dal::ConfigsGod::getinst().getGlobalGameState();
        switch ( curState ) {
        case dal::GlobalGameState::game:
            e.intArg1 = static_cast<int>(dal::GlobalGameState::menu); break;
        case dal::GlobalGameState::menu:
            e.intArg1 = static_cast<int>(dal::GlobalGameState::game); break;
        }

        dal::EventGod::getinst().notifyAll(e);
    }

    inline constexpr unsigned int enum2Index(const dal::KeySpec key) {
        return static_cast<unsigned int>(key) - static_cast<unsigned int>(dal::KeySpec::unknown);
    }

    inline constexpr dal::KeySpec index2Enum(const unsigned int index) {
        return static_cast<dal::KeySpec>(index + static_cast<unsigned int>(dal::KeySpec::unknown));
    }

    constexpr char encodeToAscii(const dal::KeySpec key, const bool shift) {
        const auto keyInt = int(key);

        if ( int(dal::KeySpec::a) <= keyInt&& keyInt <= int(dal::KeySpec::z) ) {
            if ( shift ) {
                return char(int('A') + keyInt - int(dal::KeySpec::a));
            }
            else {
                return char(int('a') + keyInt - int(dal::KeySpec::a));
            }
        }
        else if ( int(dal::KeySpec::n0) <= keyInt && keyInt <= int(dal::KeySpec::n9) ) {
            if ( shift ) {
                const auto index = keyInt - int(dal::KeySpec::n0);
                constexpr char map[] = { ')','!','@','#','$','%','^','&','*','(' };
                return map[index];
            }
            else {
                return char(int('0') + keyInt - int(dal::KeySpec::n0));
            }
        }
        else if ( int(dal::KeySpec::backquote) <= keyInt && keyInt <= int(dal::KeySpec::slash) ) {
            // backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash
            const auto index = keyInt - int(dal::KeySpec::backquote);
            if ( shift ) {
                constexpr char map[] = { '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?' };
                return map[index];
            }
            else {
                constexpr char map[] = { '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/' };
                return map[index];
            }
        }
        else if ( int(dal::KeySpec::space) <= keyInt && keyInt <= int(dal::KeySpec::tab) ) {
            // space, enter, backspace, tab
            const auto index = keyInt - int(dal::KeySpec::space);
            constexpr char map[] = { ' ', '\n', '\b', '\t' };
            return map[index];
        }
        else {
            return '\0';
        }
    }

}  // namespace


namespace {

    struct NoclipMoveInfo {
        float xView, yView;
        float xMovePlane, zMovePlane;
        float vertical;
    };

    /*
    class TouchMaster {

        //////// Definitions ////////

    private:
        enum class TouchDuty { idle, movement, view };

        struct TouchState {
            TouchDuty currentDuty = TouchDuty::idle;

            glm::vec2 lastDownPos;
            float lastDownSec = 0.0f;

            glm::vec2 accumRel;
            glm::vec2 lastPos;
        };

        //////// Atrribs ////////

    private:
        static constexpr unsigned int kMaxMultiTouchNum = 10;
        std::array<TouchState, kMaxMultiTouchNum> touchStates;
        std::array<dal::QuadPrimitive, 11>* mBoxesForTouchPoint = nullptr;

        //////// Methods ////////

    public:
        void reset(void) {
            for ( unsigned int i = 0; i < kMaxMultiTouchNum; i++ ) {
                touchStates[i].currentDuty = TouchDuty::idle;
            }
        }

        bool fetch_noclipMove(NoclipMoveInfo* info, dal::OverlayMaster& overlay) {
            //auto& logger = dal::LoggerGod::getinst();

            bool flagToReturn = false;

            const float winWidth = (float)dal::ConfigsGod::getinst().getWinWidth();
            const float winHeight = (float)dal::ConfigsGod::getinst().getWinHeight();
            const float widthOrHeightButShorter = winWidth < winHeight ? winWidth : winHeight;
            const float viewMultiplier = 5.0f / widthOrHeightButShorter;
            const float aThridWidth = winWidth / 3.0f;

            // Init locals
            {
                *info = { 0 };
            }

            // Update touchStates[i]
            {
                for ( unsigned int i = 0; i < dal::TouchEvtQueueGod::getinst().getSize(); i++ ) {
                    const auto& touch = dal::TouchEvtQueueGod::getinst().at(i);

                    if ( 0 > touch.id || touch.id > touchStates.size() ) {
                        dalWarn("Touch id is out of boundary: "s + std::to_string(touch.id));
                        continue;
                    }
                    auto& state = touchStates[touch.id];

                    switch ( touch.type ) {

                    case dal::TouchType::down:
                        if ( touch.x < aThridWidth ) {
                            if ( this->isMovementOccupied(touchStates.data(), kMaxMultiTouchNum) && state.currentDuty != TouchDuty::movement ) {
                                state.currentDuty = TouchDuty::view;
                                state.lastDownPos = { touch.x, touch.y };
                                state.lastDownSec = touch.timeSec;
                                state.lastPos = { touch.x, touch.y };
                                state.accumRel = { 0.0f, 0.0f };
                            }
                            else {
                                state.currentDuty = TouchDuty::movement;
                                state.lastDownPos = { touch.x, touch.y };
                                state.lastDownSec = touch.timeSec;
                                state.lastPos = state.lastDownPos;
                            }
                        }
                        else {
                            state.currentDuty = TouchDuty::view;
                            state.lastDownPos = { touch.x, touch.y };
                            state.lastDownSec = touch.timeSec;
                            state.lastPos = { touch.x, touch.y };
                            state.accumRel = { 0.0f, 0.0f };
                        }
                        break;
                    case dal::TouchType::move:
                        switch ( state.currentDuty ) {

                        case TouchDuty::movement:
                            state.lastPos = { touch.x, touch.y };
                            break;
                        case TouchDuty::view:
                            state.accumRel += glm::vec2{ touch.x, touch.y } -state.lastPos;
                            state.lastPos = { touch.x, touch.y };
                            break;
                        default:
                            break;

                        }
                        break;
                    case dal::TouchType::up:
                        state.currentDuty = TouchDuty::idle;
                        if ( isTap(state, touch) ) {
                            overlay.onClick(state.lastDownPos.x, state.lastDownPos.y);
                        }
                        break;
                    }
                }
                dal::TouchEvtQueueGod::getinst().clear();
            }

            // Apply to NoclipMoveInfo*
            {
                for ( unsigned int i = 0; i < kMaxMultiTouchNum; i++ ) {
                    auto& state = touchStates[i];
                    auto rel = (state.lastPos - state.lastDownPos) * 10.0f / widthOrHeightButShorter;

                    switch ( state.currentDuty ) {

                    case TouchDuty::movement:
                        if ( rel.x != 0.0f || rel.y != 0.0f ) {
                            //rel = glm::normalize(rel);
                            info->xMovePlane = rel.x;
                            info->zMovePlane = rel.y;
                            flagToReturn = true;
                        }
                        break;
                    case TouchDuty::view:
                        if ( state.accumRel.x != 0.0f || state.accumRel.y != 0.0f ) {
                            info->xView += state.accumRel.x * viewMultiplier;
                            info->yView -= state.accumRel.y * viewMultiplier;
                            state.accumRel = { 0.0f, 0.0f };
                            flagToReturn = true;
                        }
                        break;
                    default:
                        break;

                    }
                }
            }

            // Apply to boxes
            {
                if ( mBoxesForTouchPoint != nullptr ) {
                    for ( unsigned int i = 0; i < kMaxMultiTouchNum; i++ ) {
                        auto& state = touchStates[i];

                        switch ( state.currentDuty ) {

                        case TouchDuty::movement:
                            mBoxesForTouchPoint->at(i).moveCenterTo_screenCoord(state.lastPos.x, state.lastPos.y);
                            mBoxesForTouchPoint->at(10).moveCenterTo_screenCoord(state.lastDownPos.x, state.lastDownPos.y);
                            break;
                        case TouchDuty::view:
                            mBoxesForTouchPoint->at(i).moveCenterTo_screenCoord(state.lastPos.x, state.lastPos.y);
                            break;
                        default:
                            mBoxesForTouchPoint->at(i).moveCenterTo_screenCoord(-100.0f, -100.0f);
                            break;

                        }

                        if ( !this->isMovementOccupied(touchStates.data(), kMaxMultiTouchNum) ) {
                            mBoxesForTouchPoint->at(10).moveCenterTo_screenCoord(-100.0f, -100.0f);
                        }
                    }
                }
            }

            return flagToReturn;
        }

        void fetch_menuControl(dal::OverlayMaster& overlay) {
            // Update touchStates[i]
            {
                for ( unsigned int i = 0; i < dal::TouchEvtQueueGod::getinst().getSize(); i++ ) {
                    const auto& touch = dal::TouchEvtQueueGod::getinst().at(i);

                    if ( 0 > touch.id || touch.id > touchStates.size() ) {
                        dalWarn("Touch id is out of boundary: "s + std::to_string(touch.id));
                        continue;
                    }
                    auto& state = touchStates[touch.id];

                    const auto startPos = state.lastPos;

                    switch ( touch.type ) {

                    case dal::TouchType::down:
                        state.currentDuty = TouchDuty::view;
                        state.lastDownPos = { touch.x, touch.y };
                        state.lastDownSec = touch.timeSec;
                        state.lastPos = { touch.x, touch.y };
                        state.accumRel = { 0.0f, 0.0f };
                        break;
                    case dal::TouchType::move:
                        state.accumRel += glm::vec2{ touch.x, touch.y } -state.lastPos;
                        state.lastPos = { touch.x, touch.y };
                        overlay.onDrag(startPos, state.lastPos);
                        break;
                    case dal::TouchType::up:
                        state.currentDuty = TouchDuty::idle;
                        if ( isTap(state, touch) ) {
                            overlay.onClick(state.lastDownPos.x, state.lastDownPos.y);
                        }
                        break;
                    }
                }
                dal::TouchEvtQueueGod::getinst().clear();
            }

            // Apply to boxes
            {
                if ( mBoxesForTouchPoint != nullptr ) {
                    mBoxesForTouchPoint->at(10).moveCenterTo_screenCoord(-100.0f, -100.0f);

                    for ( unsigned int i = 0; i < kMaxMultiTouchNum; i++ ) {
                        auto& state = touchStates[i];

                        switch ( state.currentDuty ) {

                        case TouchDuty::view:
                            mBoxesForTouchPoint->at(i).moveCenterTo_screenCoord(state.lastPos.x, state.lastPos.y);
                            break;
                        default:
                            mBoxesForTouchPoint->at(i).moveCenterTo_screenCoord(-100.0f, -100.0f);
                            break;

                        }
                    }
                }
            }
        }

        void giveTouchPointDrawer(std::array<dal::QuadPrimitive, 11>* v) {
            mBoxesForTouchPoint = v;
        }

    private:
        static bool isMovementOccupied(const TouchState* const touchStates, const size_t arrSize) {
            for ( unsigned int i = 0; i < arrSize; i++ ) {
                if ( touchStates[i].currentDuty == TouchDuty::movement ) return true;
            }
            return false;
        }

        static bool isTap(const TouchState& state, const dal::TouchEvent& touch) {
            if ( dal::getTime_sec() - state.lastDownSec > 0.2f ) return false;

            auto rel = glm::vec2{ touch.x, touch.y } -state.lastDownPos;
            if ( glm::length(rel) > 10.0f ) return false;

            return true;
        }

    };  // class TouchMaster
    */

    class TouchStatesMaster {

        //////// Definitions ////////

    public:
        class TouchCommand {

        public:
            enum class CmdType { toggle_game_state, click };

        private:
            const CmdType m_type;
            glm::vec2 m_vec2;

        public:
            TouchCommand(const CmdType cmdType)
                : m_type(cmdType)
            {

            }

            TouchCommand(const glm::vec2& touchPos)
                :m_type(CmdType::click),
                m_vec2(touchPos)
            {

            }

            CmdType getType(void) const {
                return this->m_type;
            }

            const glm::vec2& getClickPos(void) const {
                return this->m_vec2;
            }

        };

    private:
        struct TouchState {
            glm::vec2 m_lastDownPos, m_pos;
            float m_lastDownSec = 0.0f, m_sec = 0.0f;
            bool m_down = false;
        };

        //////// Var ////////

    private:
        static constexpr int k_maxTouchCount = 10;
        std::array<TouchState, k_maxTouchCount> m_thisStates, m_lastStates;
        std::vector<int32_t> m_updated;  // I wanted to use unordered_set but VC++ has a bug.
        int32_t m_moveOccupier = -1;

        static constexpr float k_touchDrawerThiccness = 20.0f;
        std::array<dal::ColoredTile*, k_maxTouchCount + 1> m_touchDrawers = { nullptr };

        //////// Func ////////

    public:
        ~TouchStatesMaster(void) {
            for ( size_t i = 0; i < this->m_touchDrawers.size(); i++ ) {
                if ( nullptr != this->m_touchDrawers[i] ) {
                    delete this->m_touchDrawers[i];
                    this->m_touchDrawers[i] = nullptr;
                }
            }
        }


        void fetch(void) {
            const float winWidth = (float)dal::ConfigsGod::getinst().getWinWidth();
            const float winHeight = (float)dal::ConfigsGod::getinst().getWinHeight();
            const float widthOrHeightButShorter = winWidth < winHeight ? winWidth : winHeight;
            const float aThridWidth = winWidth / 3.0f;

            this->m_lastStates = this->m_thisStates;

            auto& tq = dal::TouchEvtQueueGod::getinst();

            for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                const auto& touch = tq.at(i);
                if ( 0 > touch.id || touch.id >= this->k_maxTouchCount ) {
                    dalWarn("Touch id is out of boundary: "s + std::to_string(touch.id));
                    continue;
                }
                auto& state = this->m_thisStates[touch.id];
                const auto pos = glm::vec2{ touch.x, touch.y };
                

                if ( this->m_updated.end() == std::find(this->m_updated.begin(), this->m_updated.end(), touch.id) ) {
                    this->m_updated.push_back(touch.id);
                }

                if ( dal::TouchType::move == touch.type ) {
                    state.m_pos = pos;
                    state.m_sec = touch.timeSec;
                    this->updateTouchDrawer(touch.id, pos.x, pos.y);
                }
                else if ( dal::TouchType::down == touch.type ) {
                    state.m_pos = pos;
                    state.m_sec = touch.timeSec;
                    state.m_down = true;

                    state.m_lastDownPos = pos;
                    state.m_lastDownSec = touch.timeSec;
                    this->updateTouchDrawer(touch.id, pos.x, pos.y);

                    if ( touch.x < aThridWidth && -1 == this->m_moveOccupier ) {
                        this->m_moveOccupier = touch.id;
                        this->updateTouchDrawer(k_maxTouchCount, state.m_lastDownPos.x, state.m_lastDownPos.y);
                    }
                }
                else if ( dal::TouchType::up == touch.type ) {
                    state.m_pos = pos;
                    state.m_sec = touch.timeSec;
                    state.m_down = false;
                    this->updateTouchDrawer(touch.id, -100.0f, -100.0f);

                    if ( touch.id == this->m_moveOccupier ) {
                        this->m_moveOccupier = -1;
                        this->updateTouchDrawer(k_maxTouchCount, -100.0f, -100.0f);
                    }
                }
                else {
                    dalAbort("WTF");
                }
            }

            tq.clear();
        }

        bool makeMoveInfo(NoclipMoveInfo& info, dal::OverlayMaster& overlay) const {
            const float winWidth = (float)dal::ConfigsGod::getinst().getWinWidth();
            const float winHeight = (float)dal::ConfigsGod::getinst().getWinHeight();
            const float widthOrHeightButShorter = winWidth < winHeight ? winWidth : winHeight;
            const float viewMultiplier = 5.0f / widthOrHeightButShorter;

            bool toReturn = false;

            info = { 0 };

            for ( const auto index : this->m_updated ) {
                const auto& thisStat = this->m_thisStates[index];
                const auto& lastStat = this->m_lastStates[index];

                if ( thisStat.m_down ) {  // Being touched
                    if ( this->m_moveOccupier == index ) {
                        auto rel = (thisStat.m_pos - thisStat.m_lastDownPos) * 3.0f / widthOrHeightButShorter;
                        if ( rel.x != 0.0f || rel.y != 0.0f ) {
                            //rel = glm::normalize(rel);
                            info.xMovePlane = rel.x;
                            info.zMovePlane = rel.y;
                            toReturn = true;
                        }
                    }
                    else {
                        glm::vec2 rel;
                        if ( thisStat.m_lastDownSec != lastStat.m_lastDownSec ) {  // If newly touched
                            rel = thisStat.m_pos - thisStat.m_lastDownPos;
                        }
                        else {
                            rel = thisStat.m_pos - lastStat.m_pos;
                        }

                        if ( rel.x != 0.0f || rel.y != 0.0f ) {
                            info.xView += rel.x * viewMultiplier;
                            info.yView -= rel.y * viewMultiplier;
                            toReturn = true;
                        }
                    }
                }
                else {  // Assume this is always recently untouched
                    if ( this->isTap(thisStat) ) {
                        overlay.onClick(thisStat.m_pos.x, thisStat.m_pos.y);
                    }
                }
            }

            return toReturn;
        }

        std::vector<TouchCommand> getCommands(void) const {
            std::vector<TouchCommand> cmds;

            for ( const auto index : this->m_updated ) {
                const auto& thisStat = this->m_thisStates[index];
                const auto& lastStat = this->m_lastStates[index];

                if ( !thisStat.m_down ) {
                    if ( this->isTap(thisStat) ) {
                        cmds.emplace_back(thisStat.m_pos);
                    }
                }
            }

            return cmds;
        }


        void reset(void) {
            for ( auto& stat : this->m_thisStates ) {
                stat.m_down = false;
            }
            for ( auto& stat : this->m_lastStates ) {
                stat.m_down = false;
            }
            this->m_updated.clear();
            this->m_moveOccupier = -1;
        }

        void initOverlay(dal::OverlayMaster& overlay) {
            for ( size_t i = 0; i < this->m_touchDrawers.size(); i++ ) {
                auto wid = new dal::ColoredTile(nullptr, 1.0f, 1.0f, 1.0f, 1.0f);
                overlay.addWidget(wid);
                this->m_touchDrawers[i] = wid;

                wid->setWidth(this->k_touchDrawerThiccness);
                wid->setHeight(this->k_touchDrawerThiccness);
                wid->setPosX(-100.0f);
                wid->setPosY(-100.0f);
                wid->setPauseOnly(false);
            }
        }

    private:
        static bool isTap(const TouchState& state) {
            if ( dal::getTime_sec() - state.m_lastDownSec > 0.2f ) return false;

            const auto rel = state.m_pos -state.m_lastDownPos;
            if ( glm::length(rel) > 10.0f ) return false;

            return true;
        }

        void updateTouchDrawer(const int32_t index, const float x, const float y) {
            if ( nullptr != this->m_touchDrawers[index] ) {
                auto const wid = this->m_touchDrawers[index];
                wid->setPosX(x - this->k_touchDrawerThiccness * 0.5f);
                wid->setPosY(y - this->k_touchDrawerThiccness * 0.5f);
            }
        }

    } g_touchMas;  // class TouchStatesMaster

    /*
    class KeyboardMaster {

        //////// Definitions ////////

    private:
        struct KeyState {
            bool pressed = false;
            float lastUpdated = 0.0f;
        };

        //////// Atrribs ////////

    private:
        std::array<KeyState, dal::KEY_SPEC_SIZE> mKeyStates;

        //////// Methods ////////

    public:
        bool fetch_noclipMove(NoclipMoveInfo* info, const float deltaTime) {
            bool toReturn = false;

            // Fetch from queue
            {
                auto& kq = dal::KeyboardEvtQueueGod::getinst();

                for ( unsigned int i = 0; i < kq.getSize(); i++ ) {
                    auto& keyEvent = kq.at(i);
                    auto& correspondingKeyState = getState(keyEvent.key);

                    correspondingKeyState.lastUpdated = keyEvent.timeSec;

                    if ( keyEvent.key == dal::KeySpec::escape && keyEvent.type == dal::KeyboardType::down ) {
                        toggleGameState();
                        break;
                    }
                    else {
                        if ( keyEvent.type == dal::KeyboardType::down ) {
                            correspondingKeyState.pressed = true;
                        }
                        else {
                            correspondingKeyState.pressed = false;
                        }
                    }
                }
                kq.clear();
            }

            // Apply to NoclipMoveInfo
            {
                // Returns true if any of members of info is not 0 which means modification is required.
                *info = { 0 };

                glm::vec2 localMoveDir{ 0.0 };

                if ( getState(dal::KeySpec::w).pressed ) {
                    localMoveDir.y -= 1;
                }
                if ( getState(dal::KeySpec::s).pressed ) {
                    localMoveDir.y += 1;
                }
                if ( getState(dal::KeySpec::a).pressed ) {
                    localMoveDir.x -= 1;
                }
                if ( getState(dal::KeySpec::d).pressed ) {
                    localMoveDir.x += 1;
                }
                if ( getState(dal::KeySpec::space).pressed ) {
                    info->vertical += 0.5f;
                }
                if ( getState(dal::KeySpec::lshfit).pressed ) {
                    info->vertical -= 0.5f;
                }

                const float viewMultiplier = 2.0f * deltaTime;

                if ( getState(dal::KeySpec::left).pressed ) {
                    info->xView -= viewMultiplier;
                }
                if ( getState(dal::KeySpec::right).pressed ) {
                    info->xView += viewMultiplier;
                }
                if ( getState(dal::KeySpec::up).pressed ) {
                    info->yView += viewMultiplier;
                }
                if ( getState(dal::KeySpec::down).pressed ) {
                    info->yView -= viewMultiplier;
                }

                if ( localMoveDir.x != 0.0f || localMoveDir.y != 0.0f ) {
                    localMoveDir = glm::normalize(localMoveDir);
                    info->xMovePlane = localMoveDir.x;
                    info->zMovePlane = localMoveDir.y;
                    toReturn = true;
                }

                if ( info->vertical != 0.0f || info->xView != 0.0f || info->yView != 0.0f ) toReturn = true;
            }

            return toReturn;
        }

        void fetch_menuControl(std::string* str) {
            static bool shiftPressed = false;

            auto& kq = dal::KeyboardEvtQueueGod::getinst();
            std::vector<char> buf;

            for ( unsigned int i = 0; i < kq.getSize(); i++ ) {
                auto& keyEvent = kq.at(i);

                if ( keyEvent.key == dal::KeySpec::lshfit || keyEvent.key == dal::KeySpec::rshfit ) {
                    shiftPressed = keyEvent.type == dal::KeyboardType::down;
                }
                else if ( keyEvent.key == dal::KeySpec::escape && keyEvent.type == dal::KeyboardType::down ) {
                    toggleGameState();
                    break;
                }
                else {
                    char c = this->encodeToAscii(keyEvent.key, shiftPressed);
                    if ( c != '\0' && keyEvent.type == dal::KeyboardType::down ) {
                        buf.push_back(c);
                    }
                }
            }
            kq.clear();

            str->assign(buf.begin(), buf.end());
        }

    private:
        static char encodeToAscii(const dal::KeySpec key, const bool shift) {
            const auto keyInt = int(key);

            if ( int(dal::KeySpec::a) <= keyInt&& keyInt <= int(dal::KeySpec::z) ) {
                if ( shift ) {
                    return char(int('A') + keyInt - int(dal::KeySpec::a));
                }
                else {
                    return char(int('a') + keyInt - int(dal::KeySpec::a));
                }
            }
            else if ( int(dal::KeySpec::n0) <= keyInt && keyInt <= int(dal::KeySpec::n9) ) {
                if ( shift ) {
                    const auto index = keyInt - int(dal::KeySpec::n0);
                    constexpr char map[] = { ')','!','@','#','$','%','^','&','*','(' };
                    return map[index];
                }
                else {
                    return char(int('0') + keyInt - int(dal::KeySpec::n0));
                }
            }
            else if ( int(dal::KeySpec::backquote) <= keyInt && keyInt <= int(dal::KeySpec::slash) ) {
                // backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash
                const auto index = keyInt - int(dal::KeySpec::backquote);
                if ( shift ) {
                    constexpr char map[] = { '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?' };
                    return map[index];
                }
                else {
                    constexpr char map[] = { '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/' };
                    return map[index];
                }
            }
            else if ( int(dal::KeySpec::space) <= keyInt && keyInt <= int(dal::KeySpec::tab) ) {
                // space, enter, backspace, tab
                const auto index = keyInt - int(dal::KeySpec::space);
                constexpr char map[] = { ' ', '\n', '\b', '\t' };
                return map[index];
            }
            else {
                return '\0';
            }
        }

        KeyState& getState(const dal::KeySpec key) {
            const auto index = static_cast<unsigned int>(key) - static_cast<unsigned int>(dal::KeySpec::unknown);
            return mKeyStates.at(index);
        }

    };  // class KeyboardMaster
    */

    class KeyboardStatesMaster {

        //////// Definitions ////////

    public:
        class KeyboardCommand {

        public:
            enum class CmdType { text, toggle_game_state };

        private:
            const CmdType m_type;
            const std::string m_text;

        public:
            KeyboardCommand(const CmdType type)
            : m_type(type)
            {

            }

            KeyboardCommand(std::string&& text)
                : m_type(CmdType::text),
                m_text(std::move(text))
            {

            }

            CmdType getType(void) const {
                return this->m_type;
            }

            const std::string& getData_text(void) const {
                return this->m_text;
            }

        };

    private:
        struct KeyState {
            float m_lastUpdated = 0.0f;
            bool m_withShift = false;
            bool m_pressed = false;
        };

        //////// Var ////////

    private:
        std::array<KeyState, dal::KEY_SPEC_SIZE> m_states;
        std::vector<unsigned int> m_recentlyUpdatedIndex;

        //////// Func ////////

    public:
        void fetch(void) {
            auto& kq = dal::KeyboardEvtQueueGod::getinst();
            this->m_recentlyUpdatedIndex.clear();

            for ( size_t i = 0; i < kq.getSize(); i++ ) {
                const auto& keyEvent = kq.at(i);
                const auto keyIndex = enum2Index(keyEvent.key);
                auto& theState = this->m_states.at(keyIndex);

                this->m_recentlyUpdatedIndex.push_back(keyIndex);

                theState.m_withShift = (this->m_states[enum2Index(dal::KeySpec::lshfit)].m_pressed ||
                                        this->m_states[enum2Index(dal::KeySpec::rshfit)].m_pressed);
                theState.m_lastUpdated = keyEvent.timeSec;
                theState.m_pressed = (keyEvent.type == dal::KeyboardType::down);
            }

            kq.clear();
        }

        bool makeMoveInfo(NoclipMoveInfo& info, const float deltaTime) const {
            // Returns true if any of members of info is not 0 which means modification is required.
            info = { 0 };

            glm::vec2 localMoveDir{ 0.0 };

            if ( this->m_states[enum2Index(dal::KeySpec::w)].m_pressed ) {
                localMoveDir.y -= 1;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::s)].m_pressed ) {
                localMoveDir.y += 1;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::a)].m_pressed ) {
                localMoveDir.x -= 1;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::d)].m_pressed ) {
                localMoveDir.x += 1;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::space)].m_pressed ) {
                info.vertical += 0.5f;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::lshfit)].m_pressed ) {
                info.vertical -= 0.5f;
            }

            const float viewMultiplier = 2.0f * deltaTime;

            if ( this->m_states[enum2Index(dal::KeySpec::left)].m_pressed ) {
                info.xView -= viewMultiplier;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::right)].m_pressed ) {
                info.xView += viewMultiplier;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::up)].m_pressed ) {
                info.yView += viewMultiplier;
            }
            if ( this->m_states[enum2Index(dal::KeySpec::down)].m_pressed ) {
                info.yView -= viewMultiplier;
            }

            if ( localMoveDir.x != 0.0f || localMoveDir.y != 0.0f ) {
                localMoveDir = glm::normalize(localMoveDir);
                info.xMovePlane = localMoveDir.x;
                info.zMovePlane = localMoveDir.y;
                return true;
            }

            if ( info.vertical != 0.0f || info.xView != 0.0f || info.yView != 0.0f ) {
                return true;
            }

            return false;
        }

        std::vector<KeyboardCommand> getCommands(void) const {
            std::vector<KeyboardCommand> commands;
            std::string textBuf;

            for ( auto index : this->m_recentlyUpdatedIndex ) {
                const auto state = this->m_states[index];
                const auto key = index2Enum(index);

                if ( key == dal::KeySpec::escape && state.m_pressed  ) {
                    if ( !textBuf.empty() ) {
                        commands.emplace_back(std::move(textBuf));
                        textBuf.clear();
                    }
                    commands.emplace_back(KeyboardCommand::CmdType::toggle_game_state);
                }
                else {
                    if ( state.m_pressed ) {
                        const auto c = encodeToAscii(key, state.m_withShift);
                        if ( c != '\0' ) {
                            textBuf += c;
                        }
                    }
                    
                }
            }

            return commands;
        }

    } g_keyboardMas;  // class KeyboardStatesMaster

}  // namespace


namespace {

    void apply_flyDirectional(const float deltaTime, dal::Player& player, dal::OverlayMaster& overlay) {
        glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
        float moveUpOrDown = 0.0f;
        auto const camera = player.getCamera();

        g_keyboardMas.fetch();
        g_touchMas.fetch();

        /* Take inputs */
        {
            NoclipMoveInfo keyboardInfo;
            if ( g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime) ) {
                totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
                camera->addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
                moveUpOrDown += keyboardInfo.vertical;
            }

            NoclipMoveInfo touchInfo;
            if ( g_touchMas.makeMoveInfo(touchInfo, overlay) ) {
                totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                camera->addViewPlane(touchInfo.xView, touchInfo.yView);
                moveUpOrDown += touchInfo.vertical;
            }

            camera->updateViewMat();
        }

        // Return if target doesn't need to move.
        if ( totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f ) return;

        // Apply move direction
        {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera->getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            viewMat = glm::rotate(viewMat, viewVec.y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
            if ( moveUpOrDown ) moveDirection.y = moveUpOrDown;

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera->m_pos += moveDirection * moveMultiplier;
        }

        camera->updateViewMat();
    }

    void apply_flyPlane(const float deltaTime, dal::Player& player, dal::OverlayMaster& overlay) {
        auto const camera = player.getCamera();

        glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
        float moveUpOrDown = 0.0f;

        g_keyboardMas.fetch();
        g_touchMas.fetch();

        /* Take inputs */
        {
            NoclipMoveInfo keyboardInfo;
            if ( g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime) ) {
                totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
                camera->addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
                moveUpOrDown += keyboardInfo.vertical;
            }

            NoclipMoveInfo touchInfo;
            if ( g_touchMas.makeMoveInfo(touchInfo, overlay) ) {
                totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                camera->addViewPlane(touchInfo.xView, touchInfo.yView);
                moveUpOrDown += touchInfo.vertical;
            }

            camera->updateViewMat();
        }

        // Return if target doesn't need to move.
        if ( totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f ) return;

        /* Apply move direction */
        {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera->getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            //viewMat = glm::rotate(viewMat, targetViewDir->y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
            if ( moveUpOrDown ) moveDirection.y = moveUpOrDown;

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera->m_pos += moveDirection * moveMultiplier;
        }

        camera->updateViewMat();
    }

    void apply_topdown(const float deltaTime, dal::Player& player, dal::OverlayMaster& overlay) {
        glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }

        g_keyboardMas.fetch();
        g_touchMas.fetch();

        // Take inputs
        {
            NoclipMoveInfo keyboardInfo;
            if ( g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime) ) {
                totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
            }

            NoclipMoveInfo touchInfo;
            if ( g_touchMas.makeMoveInfo(touchInfo, overlay) ) {
                auto moveVec = glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                auto moveVecLen = glm::length(moveVec);
                if ( moveVecLen > 1.0f ) {
                    moveVec /= moveVecLen;
                }
                totalMovePlane += moveVec;
            }
        }

        // Return if target doesn't need to move.
        if ( totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f ) return;

        // Apply move direction
        {
            auto actor = player.getActor();
            actor->m_pos.x += totalMovePlane.x * deltaTime * 5.0f;
            actor->m_pos.z += totalMovePlane.y * deltaTime * 5.0f;

            actor->m_quat = glm::quat{};
            const glm::vec3 moveVec3{ totalMovePlane.x, 0.0f, -totalMovePlane.y };
            const auto stranEuler = dal::vec2StrangeEuler(moveVec3);
            actor->rotate(stranEuler.x, glm::vec3{ 0.0f, 1.0f, 0.0f });

            auto camera = player.getCamera();
            constexpr float cameraAngle = 55.0f;
            const auto camOffset = glm::vec3{ 0.0, sin(glm::radians(cameraAngle)), cos(glm::radians(cameraAngle)) } * 3.0f;
            camera->m_pos = actor->m_pos + camOffset + glm::vec3{ 0.0, 1.0, 0.0 };
            camera->setViewPlane(0.0f, glm::radians(-cameraAngle));

            camera->updateViewMat();
        }
    }

    void apply_menuControl(dal::OverlayMaster& overlay) {
        g_touchMas.fetch();
        g_keyboardMas.fetch();

        for ( const auto& com : g_keyboardMas.getCommands() ) {
            switch ( com.getType() ) {

            case KeyboardStatesMaster::KeyboardCommand::CmdType::text:
                overlay.onKeyInput(com.getData_text());
                break;
            case KeyboardStatesMaster::KeyboardCommand::CmdType::toggle_game_state:
                toggleGameState();
                break;
            default:
                dalWarn("Unhandled keyboard command: "s + std::to_string(int(com.getType())));

            }
        }

        glm::vec2 pos;
        for ( const auto& cmd : g_touchMas.getCommands() ) {
            switch ( cmd.getType() ) {

            case TouchStatesMaster::TouchCommand::CmdType::click:
                pos = cmd.getClickPos();
                overlay.onClick(pos.x, pos.y);
            default:
                dalWarn("Unhandled touch command: "s + std::to_string(int(cmd.getType())));

            }
        }
    }

}  // namespace


namespace dal {

    InputApplier::InputApplier(OverlayMaster& overlayMas)
        : mFSM(GlobalGameState::game),
        m_overlayMas(overlayMas)
    {
        mHandlerName = "InputApplier";
        EventGod::getinst().registerHandler(this, EventType::global_fsm_change);

        g_touchMas.initOverlay(overlayMas);
    }

    InputApplier::~InputApplier(void) {
        EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
    }

    void InputApplier::onEvent(const EventStatic& e) {
        switch ( e.type ) {

        case EventType::global_fsm_change:
            g_touchMas.reset();
            mFSM = GlobalGameState(e.intArg1);
            break;
        default:
            LoggerGod::getinst().putWarn("InputApplier can't handle this event: "s + getEventTypeStr(e.type), __LINE__, __func__, __FILE__);
            break;

        }
    }

    void InputApplier::apply(const float deltaTime, Player& player) {

        switch ( mFSM ) {

        case GlobalGameState::game:
#if defined(_WIN32)
            apply_flyPlane(deltaTime, player, this->m_overlayMas);
#else defined(__ANDROID__)
            apply_topdown(deltaTime, player, this->m_overlayMas);
#endif
            break;
        case GlobalGameState::menu:
            apply_menuControl(this->m_overlayMas);
            break;

        }
    }

}  // namespace dal