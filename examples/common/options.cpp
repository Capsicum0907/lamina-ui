#include "common/options.hpp"

#include <algorithm>

namespace lamina::examples {

bool take_flag(int& argc, char** argv, std::string_view flag) {
    char** end = argv + argc;
    char** kept = std::remove_if(argv + std::min(argc, 1), end, [flag](const char* a) { return a == flag; });
    const bool found = kept != end;
    argc = static_cast<int>(kept - argv);
    return found;
}

}
