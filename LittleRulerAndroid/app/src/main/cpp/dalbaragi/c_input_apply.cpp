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

    const float DPAD_RADIUS_FACTOR_INV = 4.0f;

}


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
        const auto keyInt = static_cast<int>(key);

        if ( static_cast<int>(dal::KeySpec::a) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::z) ) {
            if ( shift ) {
                return static_cast<char>(static_cast<int>('A') + keyInt - static_cast<int>(dal::KeySpec::a));
            }
            else {
                return char(static_cast<int>('a') + keyInt - static_cast<int>(dal::KeySpec::a));
            }
        }
        else if ( static_cast<int>(dal::KeySpec::n0) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::n9) ) {
            if ( shift ) {
                const auto index = keyInt - static_cast<int>(dal::KeySpec::n0);
                constexpr char map[] = { ')','!','@','#','$','%','^','&','*','(' };
                return map[index];
            }
            else {
                return static_cast<char>(static_cast<int>('0') + keyInt - static_cast<int>(dal::KeySpec::n0));
            }
        }
        else if ( static_cast<int>(dal::KeySpec::backquote) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::slash) ) {
            // backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash
            const auto index = keyInt - static_cast<int>(dal::KeySpec::backquote);
            if ( shift ) {
                constexpr char map[] = { '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?' };
                return map[index];
            }
            else {
                constexpr char map[] = { '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/' };
                return map[index];
            }
        }
        else if ( static_cast<int>(dal::KeySpec::space) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::tab) ) {
            // space, enter, backspace, tab
            const auto index = keyInt - static_cast<int>(dal::KeySpec::space);
            constexpr char map[] = { ' ', '\n', '\b', '\t' };
            return map[index];
        }
        else {
            return '\0';
        }
    }

    /*
    In OpenGL coordinate system, if input is (x, z), rotation follows left hand rule.
    */
    glm::vec2 rotateVec2(const glm::vec2& v, const float radians) {
        const auto sinVal = sin(radians);
        const auto cosVal = cos(radians);

        return glm::vec2{
            v.x * cosVal - v.y * sinVal,
            v.x * sinVal + v.y * cosVal
        };
    }

    /*
    Normalize xz components on the plane (0, 1, 0, -v.y).
    So if v.y is longer than length, all 3 components must be resized to make it onto sphere.
    */
    glm::vec3 resizeOnlyXZ(const glm::vec3& v, const float sphereRadius) {
        const auto circleOfIntersecRadiusSqr = sphereRadius * sphereRadius - v.y * v.y;
        if ( circleOfIntersecRadiusSqr > 0.0f ) {
            const auto circleOfIntersecRadius = sqrt(circleOfIntersecRadiusSqr);
            const auto resizedVecXZ = glm::normalize(glm::vec2{ v.x, v.z }) * circleOfIntersecRadius;
            return glm::vec3{ resizedVecXZ.x, v.y, resizedVecXZ.y };
        }
        else {
            return glm::normalize(v) * sphereRadius;
        }
    }

    glm::vec2 clampVec(glm::vec2 v, const float maxLen) {
        const auto lenSqr = glm::dot(v, v);
        if ( lenSqr > maxLen * maxLen ) {
            v *= glm::inversesqrt(lenSqr) * maxLen;
        }
        return v;
    }

    template <unsigned int MAX_LEN>
    glm::vec2 clampVec(glm::vec2 v) {
        constexpr auto maxLen = static_cast<float>(MAX_LEN);
        constexpr auto maxLenSqr = static_cast<float>(MAX_LEN * MAX_LEN);

        const auto lenSqr = glm::dot(v, v);
        if ( lenSqr > maxLenSqr ) {
            v *= glm::inversesqrt(lenSqr) * maxLen;
        }
        return v;
    }

}  // namespace


namespace {

    struct NoclipMoveInfo {
        float xView = 0.0f, yView = 0.0f;
        float xMovePlane = 0.0f, zMovePlane = 0.0f;
        float vertical = 0.0f;

        NoclipMoveInfo& operator+=(const NoclipMoveInfo& other) {
            this->xView += other.xView;
            this->yView += other.yView;
            this->xMovePlane += other.xMovePlane;
            this->zMovePlane += other.zMovePlane;
            this->vertical += other.vertical;

            return *this;
        }
    };


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
        std::vector<dal::ColoredTile> m_touchDrawers;

        //////// Func ////////

    public:
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

                    if ( touch.id == this->m_moveOccupier ) {
                        const auto moveStickPos = clampVec(glm::vec2{ pos.x, pos.y } - state.m_lastDownPos, widthOrHeightButShorter / DPAD_RADIUS_FACTOR_INV) + state.m_lastDownPos;
                        state.m_pos = moveStickPos;
                    }

                    this->updateTouchDrawer(touch.id);
                }
                else if ( dal::TouchType::down == touch.type ) {
                    state.m_pos = pos;
                    state.m_sec = touch.timeSec;
                    state.m_down = true;

                    state.m_lastDownPos = pos;
                    state.m_lastDownSec = touch.timeSec;

                    if ( touch.x < aThridWidth && -1 == this->m_moveOccupier ) {
                        this->m_moveOccupier = touch.id;
                        this->setTouchDrawer(k_maxTouchCount, state.m_lastDownPos.x, state.m_lastDownPos.y);
                    }

                    this->setTouchDrawer(touch.id, pos.x, pos.y);
                }
                else if ( dal::TouchType::up == touch.type ) {
                    state.m_pos = pos;
                    state.m_sec = touch.timeSec;
                    state.m_down = false;
                    this->hideTouchDrawer(touch.id);

                    if ( touch.id == this->m_moveOccupier ) {
                        this->m_moveOccupier = -1;
                        this->hideTouchDrawer(k_maxTouchCount);
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
                        auto rel = (thisStat.m_pos - thisStat.m_lastDownPos) * DPAD_RADIUS_FACTOR_INV / widthOrHeightButShorter;
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
            this->m_touchDrawers.reserve(k_maxTouchCount + 1);

            for ( size_t i = 0; i < k_maxTouchCount + 1; ++i ) {
                auto& added = this->m_touchDrawers.emplace_back(nullptr, 1.0f, 1.0f, 1.0f, 1.0f);
                overlay.giveWidgetRef(&added);

                added.setWidth(this->k_touchDrawerThiccness);
                added.setHeight(this->k_touchDrawerThiccness);
                added.setPosX(-100.0f);
                added.setPosY(-100.0f);
                added.setPauseOnly(false);
            }
        }

    private:
        static bool isTap(const TouchState& state) {
            if ( dal::getTime_sec() - state.m_lastDownSec > 0.2f ) return false;

            const auto rel = state.m_pos -state.m_lastDownPos;
            if ( glm::length(rel) > 10.0f ) return false;

            return true;
        }

        void updateTouchDrawer(const int32_t index) {
            if ( this->m_touchDrawers.size() > index ) {
                auto& wid = this->m_touchDrawers[index];
                wid.setPosX(this->m_thisStates[index].m_pos.x - this->k_touchDrawerThiccness * 0.5f);
                wid.setPosY(this->m_thisStates[index].m_pos.y - this->k_touchDrawerThiccness * 0.5f);
            }
        }

        void setTouchDrawer(const int32_t index, const float x, const float y) {
            if ( this->m_touchDrawers.size() > index ) {
                auto& wid = this->m_touchDrawers[index];
                wid.setPosX(x - this->k_touchDrawerThiccness * 0.5f);
                wid.setPosY(y - this->k_touchDrawerThiccness * 0.5f);
            }
        }

        void hideTouchDrawer(const int32_t index) {
            if ( this->m_touchDrawers.size() > index ) {
                auto& wid = this->m_touchDrawers[index];
                wid.setPosX(-100.0f - this->k_touchDrawerThiccness * 0.5f);
                wid.setPosY(-100.0f - this->k_touchDrawerThiccness * 0.5f);
            }
        }

    } g_touchMas;  // class TouchStatesMaster


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

            if ( !textBuf.empty() ) {
                commands.emplace_back(std::move(textBuf));
            }

            return commands;
        }

    } g_keyboardMas;  // class KeyboardStatesMaster

}  // namespace


namespace {

    void apply_flyDirectional(const float deltaTime, dal::StrangeEulerCamera& camera, dal::OverlayMaster& overlay) {
        glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
        float moveUpOrDown = 0.0f;

        g_keyboardMas.fetch();
        g_touchMas.fetch();

        /* Take inputs */
        {
            NoclipMoveInfo keyboardInfo;
            if ( g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime) ) {
                totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
                camera.addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
                moveUpOrDown += keyboardInfo.vertical;
            }

            NoclipMoveInfo touchInfo;
            if ( g_touchMas.makeMoveInfo(touchInfo, overlay) ) {
                totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                camera.addViewPlane(touchInfo.xView, touchInfo.yView);
                moveUpOrDown += touchInfo.vertical;
            }

            camera.updateViewMat();
        }

        // Return if target doesn't need to move.
        if ( totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f ) return;

        // Apply move direction
        {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera.getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            viewMat = glm::rotate(viewMat, viewVec.y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
            if ( moveUpOrDown ) moveDirection.y = moveUpOrDown;

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera.m_pos += moveDirection * moveMultiplier;
        }

        camera.updateViewMat();
    }

    void apply_flyPlane(const float deltaTime, dal::StrangeEulerCamera& camera, dal::OverlayMaster& overlay) {
        glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
        float moveUpOrDown = 0.0f;

        g_keyboardMas.fetch();
        g_touchMas.fetch();

        /* Take inputs */
        {
            NoclipMoveInfo keyboardInfo;
            if ( g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime) ) {
                totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
                camera.addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
                moveUpOrDown += keyboardInfo.vertical;
            }

            NoclipMoveInfo touchInfo;
            if ( g_touchMas.makeMoveInfo(touchInfo, overlay) ) {
                totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                camera.addViewPlane(touchInfo.xView, touchInfo.yView);
                moveUpOrDown += touchInfo.vertical;
            }

            camera.updateViewMat();
        }

        // Return if target doesn't need to move.
        if ( totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f ) return;

        /* Apply move direction */
        {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera.getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            //viewMat = glm::rotate(viewMat, targetViewDir->y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
            if ( moveUpOrDown ) moveDirection.y = moveUpOrDown;

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera.m_pos += moveDirection * moveMultiplier;
            camera.m_pos.y += moveDirection.y * 50.0f * deltaTime;
        }

        camera.updateViewMat();
    }

    void apply_topdown(const float deltaTime, dal::StrangeEulerCamera& camera, dal::cpnt::Transform& cpntTrans, dal::OverlayMaster& overlay) {


        // This means it must represent move direction when targetViewDir == { 0, 0 }
        NoclipMoveInfo totalMoveInfo{ 0.0f };

        // Take inputs
        {
            g_keyboardMas.fetch();
            g_touchMas.fetch();

            NoclipMoveInfo keyboardInfo;
            const auto keyUpdated = g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime);
            if ( keyUpdated ) {
                totalMoveInfo += keyboardInfo;
            }

            NoclipMoveInfo touchInfo;
            const auto touchUpdated = g_touchMas.makeMoveInfo(touchInfo, overlay);
            if ( touchUpdated ) {
                //const auto moveVec = clampVec<1>(glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane });
                const auto moveVec = glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                touchInfo.xMovePlane = moveVec.x;
                touchInfo.zMovePlane = moveVec.y;

                totalMoveInfo += touchInfo;
            }

            if ( !keyUpdated && !touchUpdated ) {
                return;
            }
        }

        // Post process
        {
            const auto clampedMovePlane = clampVec<1>(glm::vec2{ totalMoveInfo.xMovePlane, totalMoveInfo.zMovePlane });
            totalMoveInfo.xMovePlane = clampedMovePlane.x;
            totalMoveInfo.zMovePlane = clampedMovePlane.y;
        }

        // Apply move direction
        {
            const glm::vec3 k_modelCamOffset{ 0.0f, 1.3f, 0.0f };

            {
                constexpr float CAM_ROTATE_SPEED_INV = 0.5f;
                static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

                const auto camViewVec = dal::strangeEuler2Vec(camera.getStrangeEuler());
                const auto rotatorAsCamX = glm::rotate(glm::mat4{ 1.0f }, camera.getStrangeEuler().getX(), glm::vec3{ 0.0f, 1.0f, 0.0f });
                const auto rotatedMoveVec = rotateVec2(glm::vec2{ totalMoveInfo.xMovePlane, totalMoveInfo.zMovePlane }, camera.getStrangeEuler().getX());

                const auto deltaPos = glm::vec3{ rotatedMoveVec.x, totalMoveInfo.vertical, rotatedMoveVec.y } *deltaTime * 5.0f;
                cpntTrans.m_pos += deltaPos;
                camera.m_pos += deltaPos * CAM_ROTATE_SPEED_INV;
                if ( rotatedMoveVec.x != 0.0f || rotatedMoveVec.y != 0.0f ) {
                    cpntTrans.m_quat = dal::rotateQuat(glm::quat{}, atan2(rotatedMoveVec.x, rotatedMoveVec.y), glm::vec3{ 0.0f, 1.0f, 0.0f });
                }
            }

            {
                constexpr float MAX_Y_DEGREE = 75.0f;

                const auto obj2CamVec = camera.m_pos - (cpntTrans.m_pos + k_modelCamOffset);
                const auto len = glm::length(obj2CamVec);
                auto obj2CamSEuler = dal::vec2StrangeEuler(obj2CamVec);

                obj2CamSEuler.addX(totalMoveInfo.xView);
                obj2CamSEuler.addY(-totalMoveInfo.yView);

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::strangeEuler2Vec(obj2CamSEuler);
                camera.m_pos = cpntTrans.m_pos + k_modelCamOffset + rotatedVec * len;
            }

            {
                constexpr float OBJ_CAM_DISTANCE = 2.0f;

                const auto cam2ObjVec = cpntTrans.m_pos - camera.m_pos + k_modelCamOffset;
                const auto cam2ObjSEuler = dal::vec2StrangeEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.getX(), cam2ObjSEuler.getY());

                camera.m_pos = cpntTrans.m_pos + k_modelCamOffset - resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE);
            }

            camera.updateViewMat();
            cpntTrans.updateMat();
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
                break;
            default:
                dalWarn("Unhandled touch command: "s + std::to_string(int(cmd.getType())));

            }
        }
    }

}  // namespace


namespace dal {

    InputApplier::InputApplier(OverlayMaster& overlayMas)
        : mFSM(GlobalGameState::game)
        , m_overlayMas(overlayMas)
    {
        this->mHandlerName = "InputApplier";
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

    void InputApplier::apply(const float deltaTime, StrangeEulerCamera& camera, cpnt::Transform& cpntTrans) {
        if ( GlobalGameState::game == this->mFSM ) {
#if defined(_WIN32)
            apply_topdown(deltaTime, camera, cpntTrans, this->m_overlayMas);
#else defined(__ANDROID__)
            apply_topdown(deltaTime, camera, cpntTrans, this->m_overlayMas);
#endif
        }
        else if ( GlobalGameState::menu == this->mFSM ) {
            apply_menuControl(this->m_overlayMas);
        }
        else {
            dalAbort("Unknown global game state.");
        }
    }

}  // namespace dal