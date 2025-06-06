//
// Created by Ali Elmorsy on 5/22/2025.
//

#ifndef COMPONENTCONTEXT_H
#define COMPONENTCONTEXT_H
#include <memory>
#include <string>
#include <vector>

namespace amara {
    namespace widgets {
        class Widget;
    }

    class BaseEngine;


    class ComponentContext {
    public:
        ComponentContext(BaseEngine *engine, std::string id): _engine(engine), _id(std::move(id)) {
        };

        ~ComponentContext() {
            clean();
        }

        void registerWidget(widgets::Widget *widgets);

        friend class BaseEngine;

    private:
        BaseEngine *_engine;
        std::string _id;

        std::vector<ComponentContext *> _children;
        std::vector<widgets::Widget *> widgets;

        void clean();
    };
}
#endif //COMPONENTCONTEXT_H
