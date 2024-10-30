#pragma once

#define USE_STL_VECTOR 0
#define USE_STL_DEQUE 1

#if USE_STL_VECTOR
#include <vector>
#include <algorithm>
namespace lightning::util {
    template<typename T>
    using vector = typename std::vector<T>;

    template<typename T> void erease_unordered(T& v, size_t index) {
        if (v.size() > 1) {
            std::iter_swap(v.begin() + index, v.end() - 1);
            v.pop_back();
        }
        else {
            v.clear();
        }
    }
}
#else 
#include "Vector.h"
namespace lightning::util {
    template<typename T> void erease_unordered(T& v, size_t index) {
        v.erease_unordered(index);
    }
}
#endif

#if USE_STL_DEQUE
#include <deque>
namespace lightning::util {
    template<typename T>
    using deque = typename std::deque<T>;
}
#endif

namespace lightning::util {
  
}

#include "FreeList.h"