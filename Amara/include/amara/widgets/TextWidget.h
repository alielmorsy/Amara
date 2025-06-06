#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H
#include "Widget.h"

namespace amara::widgets {
    class TextWidget : public Widget {
    public:
        TextWidget(): Widget(TEXT) {
        }
    };
}
#endif //TEXTWIDGET_H
