#ifndef CONSOLE_RENDERER_H
#define CONSOLE_RENDERER_H

#include "ui_element.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <string>
#include <vector>

inline void renderForm(const Form& form, std::ostream& out) {
    long width = 0, height = 0;
    for (std::size_t i = 0; i < form.getElementCount(); ++i) {
        const Rect r = form.getElement(i)->getBoundingBox();
        width = std::max(width, std::lround(r.getRight()));
        height = std::max(height, std::lround(r.getBottom()));
    }
    std::vector<std::string> canvas(static_cast<std::size_t>(height),
        std::string(static_cast<std::size_t>(width), ' '));

    for (std::size_t i = 0; i < form.getElementCount(); ++i) {
        const UIElement& el = *form.getElement(i);
        const Rect r = el.getBoundingBox();
        const long x0 = std::lround(r.getLeft()), y0 = std::lround(r.getTop());
        const long x1 = std::lround(r.getRight()) - 1, y1 = std::lround(r.getBottom()) - 1;
        if (x1 < x0 || y1 < y0 || x0 < 0 || y0 < 0) continue;

        const bool focused = form.getFocusedIndex() == static_cast<int>(i);
        const char corner = focused ? '#' : (el.isEnabled() ? '+' : '.');
        const char horiz = focused ? '#' : (el.isEnabled() ? '-' : '.');
        const char vert = focused ? '#' : (el.isEnabled() ? '|' : ':');

        for (long y = y0; y <= y1; ++y) {
            for (long x = x0; x <= x1; ++x) {
                const bool edgeX = (x == x0 || x == x1);
                const bool edgeY = (y == y0 || y == y1);
                canvas[y][x] = (edgeX && edgeY) ? corner : edgeY ? horiz : edgeX ? vert : ' ';
            }
        }
        const long w = x1 - x0 + 1, h = y1 - y0 + 1;
        if (w >= 3 && h >= 3) {
            const std::string text = el.toString().substr(0, static_cast<std::size_t>(w - 2));
            for (std::size_t k = 0; k < text.size(); ++k) {
                canvas[y0 + h / 2][x0 + 1 + static_cast<long>(k)] = text[k];
            }
        }
    }
    for (std::size_t y = 0; y < canvas.size(); ++y) {
        std::string line = canvas[y];
        const std::size_t last = line.find_last_not_of(' ');
        line.erase(last == std::string::npos ? 0 : last + 1);
        out << line << '\n';
    }
}

#endif // CONSOLE_RENDERER_H