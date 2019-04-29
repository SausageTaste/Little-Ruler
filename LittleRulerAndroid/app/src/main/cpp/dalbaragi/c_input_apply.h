#pragma once

#include <array>

#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "p_globalfsm.h"


namespace dal {
	
	class InputApplier : public iEventHandler {

	private:
		GlobalFSM mFSM;

		iKeyboardListener* mKeyListener;

	public:
		InputApplier(std::array<QuadPrimitive, 11>& mBoxesForTouchPoint);
		~InputApplier(void);

		virtual void onEvent(const EventStatic& e) override;

		void apply(const float deltaTime, glm::vec3* targetPos, glm::vec2* targetViewDir);

	};

}