#include "InstallEngine.h"

using namespace hermes;

std::unique_ptr<HermesEngine> installEngine() {
    auto runtimeConfig = vm::RuntimeConfig::Builder()
        .withIntl(true)  // Enable internationalization (keep it ON if needed)
        .withGCConfig(
            vm::GCConfig::Builder()
                .withInitHeapSize(512 * 1024)   // Start with a more reasonable heap size (512 KB)
                .withMaxHeapSize(8 * 1024 * 1024)  // Allow Hermes to use up to 8MB
                .withSanitizeConfig(
                    ::vm::GCSanitizeConfig::Builder()
                        .withSanitizeRate(0.3)  // Reduce overhead; 0.3 might be too aggressive
                        .build()
                )
                .withShouldReleaseUnused(vm::ReleaseUnused::kReleaseUnusedYoungAlways)  // Free unused memory aggressively
                .withAllocInYoung(true)  // Keep young generation allocation for performance
                .withShouldRecordStats(true)  // Keep stats for debugging
                .build()
        )
          // Faster startup at the cost of memory
        .withES6Class(true)  // Keep ES6 class support
        .withEnableEval(false)  // Disable eval() for security & optimization
        .build();


    auto runtime = makeHermesRuntime(runtimeConfig);
    auto engine = std::make_unique<HermesEngine>(std::move(runtime));
    engine->installFunctions();
    return engine;
}
