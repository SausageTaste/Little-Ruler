#include "o_widgetbase.h"

#include "s_logger_god.h"


namespace {

    inline glm::vec2 screen2device(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
             2.0f * p.x / (winWidth) - 1.0f,
            -2.0f * p.y / (winHeight) + 1.0f
        };
    }

    inline glm::vec2 screen2device(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return screen2device(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }

    inline glm::vec2 device2screen(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
            (p.x + 1.0f) * (winWidth) / 2.0f,
            (1.0f - p.y) * (winHeight) / 2.0f
        };
    }

    inline glm::vec2 device2screen(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return device2screen(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }

}


// ScreenSpaceBox
namespace dal {

    std::pair<glm::vec2, glm::vec2> ScreenSpaceBox::makeDeviceSpace(const unsigned int winWidth, const unsigned int winHeight) const {
        std::pair<glm::vec2, glm::vec2> result;

        result.first = screen2device(this->getPoint01(), winWidth, winHeight);
        result.second = screen2device(this->getPoint10(), winWidth, winHeight);

        dalAssert(result.first.x <= result.second.x);
        dalAssert(result.first.y <= result.second.y);
    }

}