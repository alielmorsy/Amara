#include <amara/ComponentContext.h>
#include <amara/widgets/Widget.h>
#include <amara/BaseEngine.h>
using namespace amara;

void ComponentContext::registerWidget(widgets::Widget *widget) {
    widgets.push_back(widget);
    widget->_component = this;
}

void ComponentContext::clean() {
    //Freeing all widgets inside this
    for (auto widget: widgets) {
        _engine->returnToPool(widget);
    }
    widgets.clear();
    for (auto component: _children) {
        delete component;
    }
    _children.clear();
}
