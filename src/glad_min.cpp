#include "glad_min.h"

#include <cstdio>

#define GLAD_MIN_DEFINE(ret, name, args) PFN_gl##name gl##name = nullptr;
GLAD_MIN_FUNCTION_LIST(GLAD_MIN_DEFINE)
#undef GLAD_MIN_DEFINE

bool gladLoadGLLoader(GLADloadproc load)
{
    if (!load) return false;
    bool ok = true;

#define GLAD_MIN_LOAD(ret, name, args)                                       \
    gl##name = (PFN_gl##name)load("gl" #name);                               \
    if (!gl##name) {                                                         \
        std::fprintf(stderr, "[gl] missing entry point: gl%s\n", #name);     \
        ok = false;                                                          \
    }
    GLAD_MIN_FUNCTION_LIST(GLAD_MIN_LOAD)
#undef GLAD_MIN_LOAD

    return ok;
}
