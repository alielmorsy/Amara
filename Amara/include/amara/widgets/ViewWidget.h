//
// Created by Ali Elmorsy on 6/4/2025.
//

#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H
#include "Widget.h"

namespace amara::widgets {
    class ViewWidget : public Widget {
    public:
        ViewWidget(): Widget(VIEW) {
        }

        ~ViewWidget() override {
            for (auto widget: _widgets) {
                //cleaning the widget itwself
                delete widget;
            }
        }

    private:
        std::vector<Widget *> _widgets;
    };
}
#endif //VIEWWIDGET_H
