#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H
#include "Widget.h"

namespace amara::widgets {
    class ButtonWidget : public widgets::Widget {
    public:
        ButtonWidget(): Widget(BUTTON) {
        };
    };
}
#endif //BUTTONWIDGET_H
