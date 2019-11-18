#pragma once


namespace dal {

    class IContext {

    public:
        virtual ~IContext(void) = default;
        virtual IContext* update(const float deltaTime) = 0;

        virtual void onWinResize(const unsigned width, const unsigned height) = 0;

    };

}
