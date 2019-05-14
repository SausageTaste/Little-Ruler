#pragma once


namespace dal {

	class IClickable {

	public:
		virtual ~IClickable(void) = default;
		virtual void onClick(const float x, const float y) = 0;

	};


	class IKeyInputTaker {

	public:
		virtual ~IKeyInputTaker(void) = default;
		virtual void onKeyInput(const char c) = 0;

	};


	class Widget {

	};


	class RenderedWidget : public Widget {

	};


}