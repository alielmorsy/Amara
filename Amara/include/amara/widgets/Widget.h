//
// Created by Ali Elmorsy on 5/22/2025.
//

#ifndef WIDGET_H
#define WIDGET_H

namespace amara::widgets {
    enum WidgetType:char {
        VIEW = 0,
        TEXT,
        IMAGE,
        BUTTON,
        HOLDER,
    };

    class Widget {
    public:
        virtual ~Widget() = default;

    protected:
        explicit Widget(const WidgetType type): _type(type) {
        };


        WidgetType _type;

        virtual void applyProps() =0;


    private:
        ComponentContext *_component = nullptr;
        friend class ComponentContext;
    };
};
#endif //WIDGET_H
