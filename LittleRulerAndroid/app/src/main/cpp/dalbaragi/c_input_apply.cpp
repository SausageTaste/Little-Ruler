#include "c_input_apply.h"

#include <string>
#include <array>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "s_input_queue.h"
#include "s_configs.h"
#include "s_event.h"
#include "u_timer.h"


using namespace std::string_literals;


namespace {

	void toggleGameState(void) {
		dal::EventStatic e;
		e.type = dal::EventType::global_fsm_change;

		const auto curState = dal::ConfigsGod::getinst().getGlobalGameState();
		switch (curState) {
		case dal::GlobalGameState::game:
			e.intArg1 = static_cast<int>(dal::GlobalGameState::menu); break;
		case dal::GlobalGameState::menu:
			e.intArg1 = static_cast<int>(dal::GlobalGameState::game); break;
		}

		dal::EventGod::getinst().notifyAll(e);
	}

}


namespace {

	struct NoclipMoveInfo {
		float xView, yView;
		float xMovePlane, zMovePlane;
		float vertical;
	};


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
			for (unsigned int i = 0; i < kMaxMultiTouchNum; i++) {
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

			/* Init locals */ {
				*info = { 0 };
			}

			/* Update touchStates[i] */ {
				for (unsigned int i = 0; i < dal::TouchEvtQueueGod::getinst().getSize(); i++) {
					const auto& touch = dal::TouchEvtQueueGod::getinst().at(i);

					if (0 > touch.id || touch.id > touchStates.size()) {
						dalWarn("Touch id is out of boundary: "s + std::to_string(touch.id));
						continue;
					}
					auto& state = touchStates[touch.id];

					switch (touch.type) {

					case dal::TouchType::down:
						if (touch.x < aThridWidth) {
							if (this->isMovementOccupied(touchStates.data(), kMaxMultiTouchNum) && state.currentDuty != TouchDuty::movement) {
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
						switch (state.currentDuty) {

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
						if (isTap(state, touch)) {
							overlay.onClick(state.lastDownPos.x, state.lastDownPos.y);
						}
						break;
					}
				}
				dal::TouchEvtQueueGod::getinst().clear();
			}

			/* Apply to NoclipMoveInfo* */ {
				for (unsigned int i = 0; i < kMaxMultiTouchNum; i++) {
					auto& state = touchStates[i];
					auto rel = (state.lastPos - state.lastDownPos) * 10.0f / widthOrHeightButShorter;

					switch (state.currentDuty) {

					case TouchDuty::movement:
						if (rel.x != 0.0f || rel.y != 0.0f) {
							//rel = glm::normalize(rel);
							info->xMovePlane = rel.x;
							info->zMovePlane = rel.y;
							flagToReturn = true;
						}
						break;
					case TouchDuty::view:
						if (state.accumRel.x != 0.0f || state.accumRel.y != 0.0f) {
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

			/* Apply to boxes */ {
				if (mBoxesForTouchPoint != nullptr) {
					for (unsigned int i = 0; i < kMaxMultiTouchNum; i++) {
						auto& state = touchStates[i];

						switch (state.currentDuty) {

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

						if (!this->isMovementOccupied(touchStates.data(), kMaxMultiTouchNum)) {
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
				for (unsigned int i = 0; i < dal::TouchEvtQueueGod::getinst().getSize(); i++) {
					const auto& touch = dal::TouchEvtQueueGod::getinst().at(i);

					if (0 > touch.id || touch.id > touchStates.size()) {
						dalWarn("Touch id is out of boundary: "s + std::to_string(touch.id));
						continue;
					}
					auto& state = touchStates[touch.id];

					const auto startPos = state.lastPos;

					switch (touch.type) {

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
						if (isTap(state, touch)) {
							overlay.onClick(state.lastDownPos.x, state.lastDownPos.y);
						}
						break;
					}
				}
				dal::TouchEvtQueueGod::getinst().clear();
			}

			// Apply to boxes
			{
				if (mBoxesForTouchPoint != nullptr) {
					mBoxesForTouchPoint->at(10).moveCenterTo_screenCoord(-100.0f, -100.0f);

					for (unsigned int i = 0; i < kMaxMultiTouchNum; i++) {
						auto& state = touchStates[i];

						switch (state.currentDuty) {

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
			for (unsigned int i = 0; i < arrSize; i++) {
				if (touchStates[i].currentDuty == TouchDuty::movement) return true;
			}
			return false;
		}

		static bool isTap(const TouchState& state, const dal::TouchEvent& touch) {
			if (dal::getTime_sec() - state.lastDownSec > 0.2f) return false;

			auto rel = glm::vec2{ touch.x, touch.y } -state.lastDownPos;
			if (glm::length(rel) > 10.0f) return false;

			return true;
		}

	} gTouchMaster;


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

			/* Fetch from queue */ {
				auto& kq = dal::KeyboardEvtQueueGod::getinst();

				for (unsigned int i = 0; i < kq.getSize(); i++) {
					auto& keyEvent = kq.at(i);
					auto& correspondingKeyState = getState(keyEvent.key);

					correspondingKeyState.lastUpdated = keyEvent.timeSec;

					if (keyEvent.key == dal::KeySpec::escape && keyEvent.type == dal::KeyboardType::down) {
						toggleGameState();
						break;
					}
					else {
						if (keyEvent.type == dal::KeyboardType::down) {
							correspondingKeyState.pressed = true;
						}
						else {
							correspondingKeyState.pressed = false;
						}
					}
				}
				kq.clear();
			}

			/* Apply to NoclipMoveInfo */ {
				// Returns true if any of members of info is not 0 which means modification is required.
				*info = { 0 };

				glm::vec2 localMoveDir{ 0.0 };

				if (getState(dal::KeySpec::w).pressed) {
					localMoveDir.y -= 1;
				}
				if (getState(dal::KeySpec::s).pressed) {
					localMoveDir.y += 1;
				}
				if (getState(dal::KeySpec::a).pressed) {
					localMoveDir.x -= 1;
				}
				if (getState(dal::KeySpec::d).pressed) {
					localMoveDir.x += 1;
				}
				if (getState(dal::KeySpec::space).pressed) {
					info->vertical += 0.5f;
				}
				if (getState(dal::KeySpec::lshfit).pressed) {
					info->vertical -= 0.5f;
				}

				const float viewMultiplier = 2.0f * deltaTime;

				if (getState(dal::KeySpec::left).pressed) {
					info->xView -= viewMultiplier;
				}
				if (getState(dal::KeySpec::right).pressed) {
					info->xView += viewMultiplier;
				}
				if (getState(dal::KeySpec::up).pressed) {
					info->yView += viewMultiplier;
				}
				if (getState(dal::KeySpec::down).pressed) {
					info->yView -= viewMultiplier;
				}

				if (localMoveDir.x != 0.0f || localMoveDir.y != 0.0f) {
					localMoveDir = glm::normalize(localMoveDir);
					info->xMovePlane = localMoveDir.x;
					info->zMovePlane = localMoveDir.y;
					toReturn = true;
				}

				if (info->vertical != 0.0f || info->xView != 0.0f || info->yView != 0.0f) toReturn = true;
			}

			return toReturn;
		}

		void fetch_menuControl(std::string* str) {
			static bool shiftPressed = false;

			auto& kq = dal::KeyboardEvtQueueGod::getinst();
			std::vector<char> buf;

			for (unsigned int i = 0; i < kq.getSize(); i++) {
				auto& keyEvent = kq.at(i);

				if (keyEvent.key == dal::KeySpec::lshfit || keyEvent.key == dal::KeySpec::rshfit) {
					shiftPressed = keyEvent.type == dal::KeyboardType::down;
				}
				else if (keyEvent.key == dal::KeySpec::escape && keyEvent.type == dal::KeyboardType::down) {
					toggleGameState();
					break;
				}
				else {
					char c = this->encodeToAscii(keyEvent.key, shiftPressed);
					if (c != '\0' && keyEvent.type == dal::KeyboardType::down) {
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
				if (shift) {
					return char(int('A') + keyInt - int(dal::KeySpec::a));
				}
				else {
					return char(int('a') + keyInt - int(dal::KeySpec::a));
				}
			}
			else if (int(dal::KeySpec::n0) <= keyInt && keyInt <= int(dal::KeySpec::n9)) {
				if (shift) {
					const auto index = keyInt - int(dal::KeySpec::n0);
					constexpr char map[] = { ')','!','@','#','$','%','^','&','*','(' };
					return map[index];
				}
				else {
					return char(int('0') + keyInt - int(dal::KeySpec::n0));
				}
			}
			else if (int(dal::KeySpec::backquote) <= keyInt && keyInt <= int(dal::KeySpec::slash)) {
				// backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash
				const auto index = keyInt - int(dal::KeySpec::backquote);
				if (shift) {
					constexpr char map[] = { '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?' };
					return map[index];
				}
				else {
					constexpr char map[] = { '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/' };
					return map[index];
				}
			}
			else if (int(dal::KeySpec::space) <= keyInt && keyInt <= int(dal::KeySpec::tab)) {
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

	} gKeyboardMaster;

}


namespace {

	void apply_flyDirectional(const float deltaTime, dal::Camera& camera, dal::OverlayMaster& overlay) {
		glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
		float moveUpOrDown = 0.0f;

		/* Take inputs */ {

			NoclipMoveInfo keyboardInfo;
			if (gKeyboardMaster.fetch_noclipMove(&keyboardInfo, deltaTime)) {
				totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
				camera.addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
				moveUpOrDown += keyboardInfo.vertical;
			}

			NoclipMoveInfo touchInfo;
			if (gTouchMaster.fetch_noclipMove(&touchInfo, overlay)) {
				totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
				camera.addViewPlane(touchInfo.xView, touchInfo.yView);
				moveUpOrDown += touchInfo.vertical;
			}
		}

		// Return if target doesn't need to move.
		if (totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f) return;

		// Apply move direction
		{
			glm::mat4 viewMat{ 1.0 };
			const auto viewVec = camera.getViewPlane();
			viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
			viewMat = glm::rotate(viewMat, viewVec.y, glm::vec3(1.0f, 0.0f, 0.0f));

			glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
			if (moveUpOrDown) moveDirection.y = moveUpOrDown;

			//moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
			const float moveMultiplier = 5.0f * deltaTime;
			camera.addPos(moveDirection * moveMultiplier);
		}

	}

	void apply_flyPlane(const float deltaTime, dal::Camera& camera, dal::OverlayMaster& overlay) {
		//auto logger = LoggerGod::getinst();

		glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
		float moveUpOrDown = 0.0f;

		/* Take inputs */ {

			NoclipMoveInfo keyboardInfo;
			if (gKeyboardMaster.fetch_noclipMove(&keyboardInfo, deltaTime)) {
				totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
				camera.addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
				moveUpOrDown += keyboardInfo.vertical;
			}

			NoclipMoveInfo touchInfo;
			if (gTouchMaster.fetch_noclipMove(&touchInfo, overlay)) {
				totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
				camera.addViewPlane(touchInfo.xView, touchInfo.yView);
				moveUpOrDown += touchInfo.vertical;
			}
		}

		// Return if target doesn't need to move.
		if (totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f) return;

		/* Apply move direction */ {
			glm::mat4 viewMat{ 1.0 };
			const auto viewVec = camera.getViewPlane();
			viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
			//viewMat = glm::rotate(viewMat, targetViewDir->y, glm::vec3(1.0f, 0.0f, 0.0f));

			glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
			if (moveUpOrDown) moveDirection.y = moveUpOrDown;

			//moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
			const float moveMultiplier = 5.0f * deltaTime;
			camera.addPos(moveDirection * moveMultiplier);
		}

	}

	void apply_menuControl(dal::OverlayMaster& overlay) {
		gTouchMaster.fetch_menuControl(overlay);

		std::string str;
		gKeyboardMaster.fetch_menuControl(&str);

		if (str.size() > 0) {
			overlay.onKeyInput(str);
		}
	}

}


namespace dal {

	InputApplier::InputApplier(OverlayMaster& overlayMas)
	:	mFSM(GlobalGameState::game),
		m_overlayMas(overlayMas)
	{
		gTouchMaster.giveTouchPointDrawer(&overlayMas.mBoxesForTouchPoint);

		mHandlerName = "InputApplier";
		EventGod::getinst().registerHandler(this, EventType::global_fsm_change);
	}

	InputApplier::~InputApplier(void) {
		EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
	}

	void InputApplier::onEvent(const EventStatic& e) {
		switch (e.type) {

			gTouchMaster.reset();

		case EventType::global_fsm_change:
			mFSM = GlobalGameState(e.intArg1);
			break;
		default:
			LoggerGod::getinst().putWarn("InputApplier can't handle this event: "s + getEventTypeStr(e.type), __LINE__, __func__, __FILE__);
			break;

		}
	}

	void InputApplier::apply(const float deltaTime, Camera& camera) {

		switch (mFSM) {

		case GlobalGameState::game:
#if defined(_WIN32)
			apply_flyPlane(deltaTime, camera, this->m_overlayMas);
#else defined(__ANDROID__)
			apply_flyDirectional(deltaTime, camera, this->m_overlayMas);
#endif
			break;
		case GlobalGameState::menu:
			apply_menuControl(this->m_overlayMas);
			break;
		
		}
	}

}