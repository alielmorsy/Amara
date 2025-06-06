#include <amara/hermes/AmaraEngine.h>
#include <hermes/hermes.h>
#include <amara/hermes/HermesValueWrapper.h>
#include <amara/widgets/TextWidget.h>
#include <amara/widgets/ViewWidget.h>
#include <amara/widgets/ButtonWidget.h>
using namespace amara;
using namespace hermes;

void AmaraEngine::initialize() {
    auto runtimeConfig = vm::RuntimeConfig::Builder()
            .withIntl(true) // Enable internationalization (keep it ON if needed)

            .withGCConfig(
                vm::GCConfig::Builder()
                .withInitHeapSize(512 * 1024) // Start with a more reasonable heap size (512 KB)
                .withMaxHeapSize(8 * 1024 * 1024) // Allow Hermes to use up to 8MB
                .withSanitizeConfig(
                    vm::GCSanitizeConfig::Builder()
                    .withSanitizeRate(0.3) // Reduce overhead; 0.3 might be too aggressive
                    .build()
                )
                .withShouldReleaseUnused(vm::ReleaseUnused::kReleaseUnusedYoungAlways)
                // Free unused memory aggressively
                .withAllocInYoung(true) // Keep young generation allocation for performance
                .withShouldRecordStats(true) // Keep stats for debugging
                .build()
            )
            // Faster startup at the cost of memory
            .withES6Class(true) // Keep ES6 class support
            .withEnableEval(false) // Disable eval() for security & optimization
            .withES6Promise(true) //Enabling promises
            .withMicrotaskQueue(true)
            .build();
    _runtime = facebook::hermes::makeHermesRuntime(runtimeConfig);
    installFunctions();
}

void AmaraEngine::installFunctions() {
    // Internal Functions
    {
        CREATE_HOST_FUNCTION(*_runtime, "beginComponentInit", 1, {
                             auto id=args[0].asString(rt).utf8(rt);
                             this->beginComponent(std::move(id));
                             return Value::undefined();
                             });

        CREATE_HOST_FUNCTION(*_runtime, "endComponent", 1, {
                             this->endComponent();
                             return Value::undefined();
                             });
        CREATE_HOST_FUNCTION(*_runtime, "createElement", 2, {
                             if (count < 2 || !args[0].isString() || !args[1].isObject()) {
                             throw JSError(rt, "Invalid arguments for createElement");
                             }
                             return Value::undefined();
                             });

        CREATE_HOST_FUNCTION(*_runtime, "effect", 1, {
                             return Value::undefined();
                             });

        CREATE_HOST_FUNCTION(*_runtime, "listConciliar", 3, {

                             return Value::undefined();
                             });
    }
    CREATE_HOST_FUNCTION(*_runtime, "render", 1, {
                         if (count == 0) {
                         throw JSINativeException("render requires at least one argument");
                         }
                         auto wrapper=std::make_unique<HermesValueWrapper>(*_runtime,Value(*_runtime,args[0]));
                         prepareRender(std::move(wrapper));
                         return Value::undefined();
                         });

    CREATE_HOST_FUNCTION(*_runtime, "shutdown", 0, {
                         //TODO: Add shutdown mechanism
                         return Value::undefined();
                         });

    CREATE_HOST_FUNCTION(*_runtime, "useState", 1, {
                         return Value::undefined();
                         });

    CREATE_HOST_FUNCTION(*_runtime, "useEffect", 2, {
                         return Value::undefined();
                         });
}

void AmaraEngine::execute(std::string path) {
    auto [binary,size] = platform::readBinaryFileFromStorage(path);
    const auto source = std::make_shared<ByteBuffer>(binary, size);
    _runtime->evaluateJavaScript(source, path);
}


widgets::Widget *AmaraEngine::createWidget(std::string &type, std::unique_ptr<ObjectMap> propsMap) {
    widgets::Widget *widget = nullptr;
    if (type == "view") {
        widget = new widgets::ViewWidget();
    } else if (type == "text") {
        widget = new widgets::TextWidget();
    } else if (type == "button") {
        widget=new widgets::ButtonWidget();
    } else if (type == "image") {
    } else {
        throw JSError(*_runtime, "Unknown component type: " + type);
    }
    return widget;
}

void AmaraEngine::prepareRender(std::unique_ptr<ValueWrapper> wrapper) {
    auto widget = wrapper->callAsFunction({});
    //Do Other stuff here
    widget->asHostObject<void>();
}

std::unique_ptr<ValueWrapper> AmaraEngine::callFunction(std::unique_ptr<ValueWrapper> func,
                                                        std::vector<std::unique_ptr<ValueWrapper> > args) {
}
