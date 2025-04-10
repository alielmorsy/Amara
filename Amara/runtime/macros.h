#ifndef MACROS_H
#define MACROS_H

#if defined(USE_HERMES)
#define CALL_FUNCTION(pointer,...) static_cast<HermesInvoker*>(pointer.get())->invoke(__VA_ARGS__);
#else
#define CALL_FUNCTION(pointer,...)
#endif
#endif //MACROS_H
