#pragma once


namespace dal {

    class IContext {

    public:
        virtual ~IContext(void) = default;
        virtual IContext* update(const float deltaTime) = 0;

    };

}
