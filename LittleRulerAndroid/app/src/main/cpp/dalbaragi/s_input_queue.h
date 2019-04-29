#pragma once

#include <array>
#include <string>

#include "s_logger_god.h"
#include "s_configs.h"


namespace dal_private {

	class iSingleUsageQueue {

		//////// vars ////////

	protected:
		static constexpr unsigned int kCapacity = 100;
		unsigned int mCurIndex = 0;

		//////// funcs ////////

		iSingleUsageQueue(void) = default;
		~iSingleUsageQueue(void) = default;
	private:
		iSingleUsageQueue(iSingleUsageQueue&) = delete;
		iSingleUsageQueue& operator=(iSingleUsageQueue&) = delete;

	public:
		void clear(void);
		bool isFull(void) const;
		unsigned int getSize(void) const;
		unsigned int getCapacity(void) const;

	};

}


namespace dal {

	enum class TouchType { down=1, move=2, up=3 };

	struct TouchEvent {
		float x, y;
		TouchType type;
		int32_t id;
		float timeSec;
	};

	class TouchEvtQueueGod : public dal_private::iSingleUsageQueue {

	//////// vars ////////

	private:
		std::array<TouchEvent, kCapacity> mArray;

	//////// funcs ////////
		
	public:
		static TouchEvtQueueGod& getinst(void);

		bool emplaceBack(const float x, const float y, const TouchType type, const int32_t id, const float timeSec);
		bool emplaceBack(const float x, const float y, const TouchType type, const int32_t id);
		
		const TouchEvent& at(const unsigned int index) const;

	};

}


namespace dal {

	// enum�� ������ ���� ������ �ڿ��� ������� �����Ǿ�� ��. ���� ��ȣ�� �������.
	enum class KeySpec {
		/* unknown */
		unknown = 40,
		/* Alphabets */
		a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
		/* Vertical numbers */
		n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,
		/* Misc in keyboard main area */
		backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash,
		/* Special characters */
		space, enter, backspace, tab,
		/* No characters */
		escape, lshfit, rshfit, lctrl, rctrl, lalt, ralt, up, down, left, right,
		/* EOF just to calculate number of elements of Enum class */
		eof
	};
	constexpr unsigned int KEY_SPEC_SIZE = int(KeySpec::eof) - int(KeySpec::unknown);

	enum class KeyboardType { down, up };

	struct KeyboardEvent {
		KeySpec key;
		KeyboardType type;
		float timeSec;
	};

	class KeyboardEvtQueueGod : public dal_private::iSingleUsageQueue {

	//////// vars ////////

	private:
		std::array<KeyboardEvent, kCapacity> mArray;

//////// funcs ////////

	public:
		static KeyboardEvtQueueGod& getinst(void);

		bool emplaceBack(const KeySpec key, const KeyboardType type);
		bool emplaceBack(const KeySpec key, const KeyboardType type, const float timeSec);

		const KeyboardEvent& at(const unsigned int index) const;

	};

}