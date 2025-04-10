//
// Created by Ali Elmorsy on 3/27/2025.
//

#ifndef WIDGET_H
#define WIDGET_H
#include <memory>

class Widget {
public:
    virtual ~Widget() = default;

    virtual void addChild(std::unique_ptr<Widget> child) = 0;

    std::optional<int> elementIndex; // Tracks the element index this widget was built from
    template<typename T>
    T *as() {
        return dynamic_cast<T *>(this);
    }

    template<typename T>
    bool is() {
        return as<T>();
    }
};

class PrimitiveWidget : public Widget {
public:
    std::string type;
    std::vector<std::unique_ptr<Widget> > children;

    explicit PrimitiveWidget(const std::string &type) : type(type) {
    }

    void addChild(std::unique_ptr<Widget> child) override {
        children.emplace_back(std::move(child));
    }

    template <typename T, typename... Args>
    void reset(T&& first, Args&&... res) {
        type = std::forward<std::string>(first);
        // Reset widget to initial state
        children.clear();
        elementIndex.reset();
        // Add any additional reset logic here
    }

    ~PrimitiveWidget() override = default;
};

class TextWidget : public Widget {
public:
    std::string val;

    explicit TextWidget(const std::string &val) : val(std::move(val)) {
    }

    ~TextWidget() override = default;

    void addChild(std::unique_ptr<Widget> child) override {
    }
};
#endif //WIDGET_H
