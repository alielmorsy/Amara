//
// Created by Ali Elmorsy on 3/31/2025.
//

#ifndef WIDGETHOSTWRAPPER_H
#define WIDGETHOSTWRAPPER_H

#include "../../ui/Widget.h"
#include <jsi/jsi.h>
class HermesEngine;
#define JSI_FUNCTION(name) Value name(Runtime &, const Value *args, size_t count)
using namespace facebook::jsi;

class WidgetHostWrapper : public HostObject {
public:
    WidgetHostWrapper(HermesEngine *engine, const SharedWidget &widget): engine(engine), nativeWidget(widget) {
    }

    ~WidgetHostWrapper() override = default;

    Value get(Runtime &, const PropNameID &name) override;

    void set(Runtime &, const PropNameID &name, const Value &value) override;

    JSI_FUNCTION(addText);

    JSI_FUNCTION(addChild);

    JSI_FUNCTION(addStaticChild);

    JSI_FUNCTION(insertChild);

    SharedWidget getNativeWidget() const;

private:
    HermesEngine *engine;
    std::weak_ptr<Widget> nativeWidget;
};


#endif //WIDGETHOSTWRAPPER_H
