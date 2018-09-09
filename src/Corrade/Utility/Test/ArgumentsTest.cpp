#include <type_traits>

namespace Implementation {

template<class T> constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type someFuncThatReturnsOne() {
    return T(1);
}
template<class T> constexpr typename std::enable_if<std::is_integral<T>::value, T>::type someFuncThatReturnsOne() {
    return {};
}

}

template<class T> struct Color4 {
    constexpr Color4(T, T = Implementation::someFuncThatReturnsOne<T>()) {}
};

int main() {
    constexpr Color4<float> a{1.0f};
}
