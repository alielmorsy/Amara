//
// Created by Ali Elmorsy on 3/31/2025.
//

#ifndef WIDGET_H
#define WIDGET_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>

#include "../runtime/PropMap.h"

#include "ComponentContext.h"
#include "../utils/css/Style.h"
#include "Key.h"

class WidgetHolder;

class Widget;
using SharedWidget = std::shared_ptr<Widget>;
using namespace std;

enum WidgetType {
    CONTAINER = 0,
    TEXT,
    IMAGE,
    BUTTON
};

class Widget : public std::enable_shared_from_this<Widget> {
protected:
    WidgetType _type;
    bool available = false;
    std::weak_ptr<Widget> parent;
    std::shared_ptr<ComponentContext> _component;
    std::unordered_map<std::string, std::string> props;

    std::unique_ptr<PropMap> style;
    WidgetStyle widgetStyle;

    Widget(std::unique_ptr<PropMap> propMap, std::shared_ptr<ComponentContext> component,
           WidgetType type): propMap(std::move(propMap))
                             , _component(std::move(component)), _type(type) {
        parseProps();
    }


    void parseProps();

    void parseStyle();


    virtual std::string getValue() =0;

    bool operator==(const SharedWidget &other) {
        return this->_type == other->_type && this->key == other->key;
    }

public:
    Key key;
    std::unique_ptr<PropMap> propMap;
    template<class T>
    std::shared_ptr<T> as() {
        return std::dynamic_pointer_cast<T>(shared_from_this());
    }

    template<typename T>
    bool is() {
        return dynamic_cast<T *>(this);
    }

    void reuse(std::unique_ptr<PropMap> propMap, std::shared_ptr<ComponentContext> component) {
        available = false;
        this->_component = std::move(component);
        this->propMap = std::move(propMap);
        parseProps();
    }

    void setParent(const std::weak_ptr<Widget> &parent) {
        this->parent = parent;
    }

    void resetPointer() {
        if (available) return;
        auto thisPtr = this;
        auto found = std::find_if(_component->widgets.begin(), _component->widgets.end(),
                                  [&](auto &c) {
                                      //     std::cout << "Checking c: " << c.get() << " against this: " << thisPtr << "\n";
                                      return c.get() == thisPtr;
                                  });
        if (found != _component->widgets.end())
            _component->widgets.erase(std::move(found));


        goReset();
        propMap.reset();
        _component.reset();


        available = true;
    }

    std::shared_ptr<ComponentContext> &component() {
        return _component;
    }

    virtual void goReset() =0;

    virtual void printTree(std::string prefix = "", bool isLast = true) =0;

    virtual ~Widget() {
        propMap.reset();
    };
};

class ContainerWidget : public Widget {
public:
    ContainerWidget(std::unique_ptr<PropMap> propMap, std::shared_ptr<ComponentContext> component): Widget(
        std::move(propMap), std::move(component), WidgetType::CONTAINER) {
    }

    void addChild(std::shared_ptr<Widget> &widget);

    void addStaticChild(IEngine *engine, std::unique_ptr<WidgetHolder> widget);

    void insertChild(IEngine *engine, std::string id, std::unique_ptr<WidgetHolder> holder);


    void goReset() override {
        //Children components need to be freed too
        for (auto &element: _children) {
            element->resetPointer();
        }

        _children.clear();
        props.clear();
    }

    bool hasChildren() {
        return !_children.empty();
    }

    void printTree(std::string prefix = "", bool isLast = true) override {
        cout << prefix;

        if (!prefix.empty()) {
            cout << "|----- ";
        }

        cout << getValue() << endl;

        for (size_t i = 0; i < _children.size(); ++i) {
            _children[i]->printTree(prefix + (isLast ? "      " : "|     "), i == _children.size() - 1);
        }
    }

    std::string getValue() override {
        return "Container with " + std::to_string(_children.size()) + " children";
    }

    void replaceChild(size_t index, const SharedWidget &widget);

    std::vector<std::shared_ptr<Widget> > &children() {
        return _children;
    }

    //Will do dumb erase from the vector because its mainly used for list reconcile. If we ever needed to make it more complex, Please make sure
    void removeChild(size_t index);

    void replaceChildren(vector<std::shared_ptr<Widget> > vector);

    void insertChild(size_t position, std::shared_ptr<Widget> widget);


protected:
    std::vector<std::shared_ptr<ComponentContext> > childrenComponents;
    std::vector<std::shared_ptr<Widget> > _children;
    std::unordered_map<std::string, size_t> insertedChildren;
    std::unordered_map<std::string, size_t> staticChildren;
};

class ButtonWidget : public ContainerWidget {
public:
    ButtonWidget(std::unique_ptr<PropMap> propMap, std::shared_ptr<ComponentContext> component)
        : ContainerWidget(std::move(propMap), std::move(component)) {
    }


    void goReset() override {
    };

protected:
    std::string getValue() override {
        return "Button with " + std::to_string(_children.size()) + " children";
    };

    //TODO
};

class ImageWidget : public Widget {
public:
    explicit ImageWidget(std::unique_ptr<PropMap> propMap, std::shared_ptr<ComponentContext> component): Widget(
        std::move(propMap), std::move(component), WidgetType::IMAGE) {
    }


    void goReset() override {
        std::string().swap(path);
    }

protected:
    void printTree(std::string prefix, bool isLast) override {
        cout << prefix;

        if (!prefix.empty()) {
            cout << (isLast ? "|----- " : "|     ");
        }
        cout << getValue() << endl;
    };

    std::string getValue() override {
        return "An Image";
    };

private:
    std::string path;
};

class TextWidget : public Widget {
public:
    explicit TextWidget(std::unique_ptr<PropMap> propMap,
                        std::shared_ptr<ComponentContext> component): Widget(
        std::move(propMap), std::move(component), WidgetType::TEXT) {
    }


    void goReset() override {
        children.clear();
    };

    void addText(const std::string &newText) {
        children.push_back(newText);
    }

    void addChild(std::shared_ptr<Widget> &widget) {
        throw std::runtime_error("You cannot add an child for a text widget");
    }

    void insertChild(std::string &id, const std::string &text);

    void printTree(std::string prefix = "", bool isLast = true) override {
        cout << prefix;

        if (!prefix.empty()) {
            cout << (isLast ? "|----- " : "|     ");
        }

        cout << getValue() << endl;
    }

    std::string getValue() override {
        std::string result;
        for (const auto &child: children) {
            result += child;
        }
        return "Text Widget: " + result;
    };

private:
    std::unordered_map<std::string, size_t> insertedChildren;
    std::vector<std::string> children;
};

class HolderWidget : public Widget {
protected:
    std::string getValue() override {
        return "Widget Holder";
    };

public:
    explicit HolderWidget(std::unique_ptr<PropMap> propMap,
                          std::shared_ptr<ComponentContext> component): Widget(
        std::move(propMap), std::move(component), WidgetType::TEXT) {
    }

    std::shared_ptr<Widget> child;

    void goReset() override {
        child.reset();
    };

    void setChild(std::shared_ptr<Widget> child) {
        this->child = child;
    }

    void printTree(std::string prefix, bool isLast) override {
        cout << prefix;

        if (!prefix.empty()) {
            cout << (isLast ? "|----- " : "|     ");
        }
        child->printTree(prefix + (isLast ? "      " : "|     "));
        cout << getValue() << endl;
    };
};
#endif //WIDGET_H
