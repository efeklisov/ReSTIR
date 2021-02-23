#include <iterator>
#include <functional>

namespace hd {
    template <class... F>
    struct overload : F... {
        overload(F... f) : F(f)... {}  
    };

    template <class... F>
    auto make_overload(F... f) {
        return overload<F...>(f...);   
    }
}
