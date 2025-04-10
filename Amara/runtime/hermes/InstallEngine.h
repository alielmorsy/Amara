#ifndef DEFINE_H
#define DEFINE_H
#include <jsi/jsi.h>
#include <hermes/hermes.h>

#include "Engine.h"
using namespace facebook::jsi;
using namespace facebook::hermes;

std::unique_ptr<HermesEngine> installEngine();

void installGlobalElements(Runtime&);

#endif
