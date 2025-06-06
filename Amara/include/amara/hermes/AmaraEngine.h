#ifndef ENGINE_H
#define ENGINE_H
#include <amara/platform/FileManagment.h>


#include "amara/BaseEngine.h"
#include "hermes/hermes.h"
#include "jsi/jsi.h"
#include "amara/hermes/ByteBuffer.h"
using namespace facebook::jsi;

#define CREATE_HOST_FUNCTION(runtime, func_name,paramCount, func_body) \
do { \
    (runtime).global().setProperty((runtime), func_name, \
        facebook::jsi::Function::createFromHostFunction( \
            (runtime), \
            facebook::jsi::PropNameID::forAscii((runtime), func_name), \
            (paramCount), \
            [this](facebook::jsi::Runtime &rt, \
                    const facebook::jsi::Value &thisVal, \
                    const facebook::jsi::Value *args, \
                    size_t count) -> facebook::jsi::Value { \
                    func_body \
                } \
            ) \
        ); \
} while (0)


namespace amara {
    class AmaraEngine : public BaseEngine {
        std::unique_ptr<Runtime> _runtime;

        void initialize() override;

        void installFunctions();

        widgets::Widget* createWidget(std::string &type, std::unique_ptr<ObjectMap> propsMap) override;

        void prepareRender(std::unique_ptr<ValueWrapper> wrapper) override;

    public:
        void execute(std::string path) override;

        std::unique_ptr<ValueWrapper> callFunction(std::unique_ptr<ValueWrapper>,
            std::vector<std::unique_ptr<ValueWrapper>> args) override;
    };
}
#endif //ENGINE_H
