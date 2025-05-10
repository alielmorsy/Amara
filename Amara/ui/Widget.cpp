#include "Widget.h"

#include "../runtime/hermes/HermesWidgetHolder.h"
#include "../utils/ScopedTimer.h"
#include "../utils/css/CssUtils.h"
#include "../utils/css/BorderInfo.h"

void Widget::parseProps() {
    //TODO REF
    if (propMap->has("ref")) {
        //TODO
        auto ref = propMap->getObject("ref");
    }
    if (propMap->has("style")) {
        this->style = std::move(propMap->getObject("style"));
        //  parseStyle();
    }
}

void Widget::parseStyle() {
    //ScopedTimer timer;
    if (style->has("display")) {
        widgetStyle.display = parseDisplay(style->getString("display"));
    }
    if (style->has("width")) {
        auto width = style->getString("width");
        widgetStyle.width = parseCSSValue(width);
    }
    if (style->has("height")) {
        auto height = style->getString("height");
        widgetStyle.height = parseCSSValue(height);
    }
    if (style->has("margin")) {
        auto values = parseMarginOrPadding(style->getString("margin"));
        auto &margin = widgetStyle.margin;
        margin.top = values.top;
        margin.right = values.right;
        margin.bottom = values.bottom;
        margin.left = values.left;
    }

    // Individual margin properties (override shorthand)
    if (style->has("marginTop"))
        widgetStyle.margin.top = parseCSSValue(style->getString("marginTop"));
    if (style->has("marginRight"))
        widgetStyle.margin.right = parseCSSValue(style->getString("marginRight"));
    if (style->has("marginBottom"))
        widgetStyle.margin.bottom = parseCSSValue(style->getString("marginBottom"));
    if (style->has("marginLeft"))
        widgetStyle.margin.left = parseCSSValue(style->getString("marginLeft"));

    // Padding handling
    if (style->has("padding")) {
        auto values = parseMarginOrPadding(style->getString("padding"));
        auto &padding = widgetStyle.padding;
        padding.top = values.top;
        padding.right = values.right;
        padding.bottom = values.bottom;
        padding.left = values.left;
    }

    // Individual padding properties (override shorthand)
    if (style->has("paddingTop"))
        widgetStyle.padding.top = parseCSSValue(style->getString("paddingTop"));
    if (style->has("paddingRight"))
        widgetStyle.padding.right = parseCSSValue(style->getString("paddingRight"));
    if (style->has("paddingBottom"))
        widgetStyle.padding.bottom = parseCSSValue(style->getString("paddingBottom"));
    if (style->has("paddingLeft"))
        widgetStyle.padding.left = parseCSSValue(style->getString("paddingLeft"));

    auto &border = widgetStyle.border;
    if (style->has("border")) {
        auto shorthand = parseBorderShorthand(style->getString("border"));


        if (shorthand.width || shorthand.style || shorthand.color) {
            BorderEdge newEdge;
            if (shorthand.width) newEdge.width = *shorthand.width;
            if (shorthand.style) newEdge.style = *shorthand.style;
            if (shorthand.color) newEdge.color = *shorthand.color;
            border.setAllEdges(newEdge);
        }
    }

    // Border color shorthand
    if (style->has("border-color")) {
        auto colors = parseBorderColor(style->getString("border-color"));
        border.top.color = colors[0];
        border.right.color = colors[1];
        border.bottom.color = colors[2];
        border.left.color = colors[3];
    }

    // Border radius
    if (style->has("border-radius")) {
        border.radius = parseBorderRadius(style->getString("border-radius"));
    }

    // Individual border edges
    auto parseAndApplyEdge = [&](const std::string &propName, BorderEdge &edge) {
        if (!style->has(propName)) return;
        auto parts = parseBorderEdge(style->getString(propName));
        if (parts.width) edge.width = *parts.width;
        if (parts.style) edge.style = *parts.style;
        if (parts.color) edge.color = *parts.color;
    };

    parseAndApplyEdge("border-top", border.top);
    parseAndApplyEdge("border-right", border.right);
    parseAndApplyEdge("border-bottom", border.bottom);
    parseAndApplyEdge("border-left", border.left);

    // Individual radius properties
    if (style->has("border-top-left-radius"))
        border.radius.topLeft = parseCSSValue(style->getString("border-top-left-radius"));
    if (style->has("border-top-right-radius"))
        border.radius.topRight = parseCSSValue(style->getString("border-top-right-radius"));
    if (style->has("border-bottom-right-radius"))
        border.radius.bottomRight = parseCSSValue(style->getString("border-bottom-right-radius"));
    if (style->has("border-bottom-left-radius"))
        border.radius.bottomLeft = parseCSSValue(style->getString("border-bottom-left-radius"));

    if (style->has("visibility")) {
        widgetStyle.visibility = parseVisibility(style->getString("visibility"));
    }

    auto &flex = widgetStyle.flex;
    if (style->has("justify-content")) {
        flex.justifyContent = parseJustifyContent(style->getString("justify-content"));
    }
    if (style->has("align-items")) {
        flex.alignItems = parseAlignItems(style->getString("align-items"));
    }
    if (style->has("align-content")) {
        flex.alignContent = parseAlignContent(style->getString("align-content"));
    }
    if (style->has("flex-direction")) {
        flex.direction = parseFlexDirection(style->getString("flex-direction"));
    }
    if (style->has("flex-wrap")) {
        flex.wrap = parseFlexWrap(style->getString("flex-wrap"));
    }
    if (style->has("gap")) {
        flex.gap = parseGap(style->getString("gap"));
    }

    // Flex item properties
    if (style->has("align-self")) {
        flex.alignSelf = parseAlignItems(style->getString("align-self"));
    }
    if (style->has("flex")) {
        auto shorthand = parseFlexShorthand(style->getString("flex"));
        flex.flexGrow = shorthand.grow;
        flex.flexShrink = shorthand.shrink;
        if (shorthand.basis) flex.flexBasis = *shorthand.basis;
    }
    if (style->has("flex-grow")) {
        flex.flexGrow = std::stof(style->getString("flex-grow"));
    }
    if (style->has("flex-shrink")) {
        flex.flexShrink = std::stof(style->getString("flex-shrink"));
    }
    if (style->has("flex-basis")) {
        flex.flexBasis = parseCSSValue(style->getString("flex-basis"));
    }
}

void ContainerWidget::addChild(std::shared_ptr<Widget> &widget) {
    _children.emplace_back(widget);
    widget->setParent(weak_from_this());
}

void ContainerWidget::addStaticChild(IEngine *engine, std::unique_ptr<WidgetHolder> widget) {
    if (_component->reconciliationStarted()) {
        auto oldComponent = _component->reconcilingObject.lock();
        if (oldComponent && widget->hasID()) {
            auto it = oldComponent->staticChildren.find(widget->getID());
            if (it != oldComponent->staticChildren.end()) {
                size_t oldIndex = it->second;
                assert(oldIndex < oldComponent->_children.size() && "Static child index out of bounds");
                _children.emplace_back(std::move(oldComponent->_children[oldIndex]));
                return;
            }
        }
        // If ID not found or no old component, treat as new static child (fallback)
    }
    // Initial render or new static child during reconciliation
    auto cmbx = widget->execute(engine);
    if (widget->isComponent()) {
        childrenComponents.emplace_back(cmbx->component());
    }
    if (widget->hasID()) {
        staticChildren[widget->getID()] = _children.size();
    }
    _children.emplace_back(std::move(cmbx));
}

void ContainerWidget::insertChild(IEngine *engine, std::string id, std::unique_ptr<WidgetHolder> holder) {
    if (_component->reconciliationStarted()) {
        auto reconcileComponent = _component->reconcilingObject.lock();
        if (reconcileComponent && reconcileComponent->insertedChildren.count(id)) {
            size_t index = reconcileComponent->insertedChildren[id];
            assert(index < reconcileComponent->_children.size() && "Insertion ID has invalid child index");
            auto newWidget = _component->reconcileObject(
                reconcileComponent->_children[index], holder);
            insertedChildren[id] = _children.size();
            _children.emplace_back(newWidget);
            return;
        }
        // If not found in an old component, treat as new child
    }
    // Handle both initial render and new child during reconciliation
    if (insertedChildren.count(id) == 0) {
        auto widget = holder->execute(engine);
        widget->setParent(weak_from_this());
        insertedChildren[id] = _children.size();
        _children.emplace_back(std::move(widget));
    } else {
        auto newWidget = _component->reconcileObject(_children[insertedChildren[id]], holder);
        newWidget->setParent(weak_from_this());
        _children[insertedChildren[id]] = newWidget;
    }
}

void ContainerWidget::replaceChild(size_t index, const SharedWidget &widget) {
    //Freeing the widget and its children;
    _children[index]->resetPointer();
    _children[index] = widget;
}

void ContainerWidget::removeChild(size_t index) {
    //freeing the pointer
    _children[index]->resetPointer();
    auto it = _children.begin();
    std::advance(it, index);
    _children.erase(std::move(it));
}

void ContainerWidget::replaceChildren(vector<std::shared_ptr<Widget> > vector) {
    for (const auto &child: _children) {
        child->resetPointer();
    }
    _children = std::move(vector);
}

void ContainerWidget::insertChild(size_t position, std::shared_ptr<Widget> widget) {
    // Ensure position is within bounds
    if (position >= _children.size()) {
        _children.push_back(std::move(widget));
        return;
    }
    _children.insert(_children.begin() + position, std::move(widget));
}

void TextWidget::insertChild(std::string &id, const std::string &text) {
    children.emplace_back(text);
    insertedChildren[id] = children.size() - 1;
}
