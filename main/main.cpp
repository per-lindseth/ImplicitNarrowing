#include <cassert>
#include <concepts>
#include <format>
#include <iostream>

using namespace std;

template <typename T>
concept Num = integral<T> || floating_point<T>;

// can we say: void func(T t, U u) {t = u;} 
template<typename T, typename U>
concept can_Narrow = (sizeof(T) < sizeof(U)) // Too small
    || (integral<T> && floating_point<U>) // can round
    || ((sizeof(T) == sizeof(U)) // the U value can be too large
        && ((floating_point<T> && integral<U>) || (signed_integral<T> != signed_integral<U>)));

template<Num T, Num U>
constexpr bool will_narrow(U u)
{
    if constexpr (!can_Narrow<T, U>) return false;
    if constexpr (signed_integral<T> && unsigned_integral<U>)
        if (numeric_limits<T>::max() < u)
            return true;
    if constexpr (unsigned_integral<T> && signed_integral<U>)
        if (u < 0)
            return true;

    T t = u;
    return (t != u);
}

class BadValue {};

template<Num T, Num U>
constexpr T convert_to(U u)
{
    if (will_narrow<T>(u)) throw BadValue();

    return T(u);
}

template<Num T>
class Number
{
    T val{};
public:
    constexpr Number() = default;
    template<Num U>
    constexpr Number(const U u) : val(convert_to<T>(u)) {}

    template<Num U>
    constexpr void operator=(const U u) { val = convert_to<T>(u); }

    operator T() { return val; }
    T Get() { return val; }
};

int main()
{
    // An integer converted to a type with a smaller size 
    short x1 = 1'000'000; // assuming short is 16 bits
    assert(x1 == 16'960);
    try
    {
        Number<short> n1;
        n1 = 1'000'000;
        assert(false);
    }
    catch (const BadValue& e) {}
    catch (...) {}

    // An unsigned int with a representation interpreted as a (large) negative int after conversion to signed
    short x2 = 0b1000'0000'0000'0000u;
    assert(x2 == -32'768);
    try
    {
        Number<short> n2;
        n2 = 0b1000'0000'0000'0000u;
        assert(false);
    }
    catch (const BadValue& e) {}
    catch (...) {}

    // An negative integer interpreted as a (large) positive value after conversion to an unsigned
    unsigned x3 = -2;
    assert(x3 == 4'294'967'294);
    try
    {
        Number<unsigned> n3;
        n3 = -2;
        assert(false);
    }
    catch (const BadValue& e) {}
    catch (...) {}

    // A floating-point value with a decimal part truncated to its integer part when converted to int
    unsigned x4 = 7.8;
    assert(x4 == 7);

    cout << format("x4 = {}\n", x4);
    try
    {
        Number<unsigned> n4;
        n4 = 7.8;
        assert(false);
    }
    catch (const BadValue& e) {}
    catch (...) {}
}