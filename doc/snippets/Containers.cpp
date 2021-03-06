/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <cstdio>
#include <string>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif

#include "Corrade/Containers/Array.h"
#include "Corrade/Containers/EnumSet.hpp"
#include "Corrade/Containers/LinkedList.h"
#include "Corrade/Containers/Optional.h"
#include "Corrade/Containers/ScopedExit.h"
#include "Corrade/Containers/StaticArray.h"
#include "Corrade/Containers/StridedArrayView.h"
#include "Corrade/Utility/Debug.h"

using namespace Corrade;

namespace Other {
/* [EnumSet-usage] */
enum class Feature: unsigned int {
    Fast = 1 << 0,
    Cheap = 1 << 1,
    Tested = 1 << 2,
    Popular = 1 << 3
};

typedef Containers::EnumSet<Feature> Features;
CORRADE_ENUMSET_OPERATORS(Features)
/* [EnumSet-usage] */
}

/* [EnumSet-friend] */
class Application {
    private:
        enum class Flag: unsigned int {
            Redraw = 1 << 0,
            Exit = 1 << 1
        };

        typedef Containers::EnumSet<Flag> Flags;
        CORRADE_ENUMSET_FRIEND_OPERATORS(Flags)
};

CORRADE_ENUMSET_OPERATORS(Application::Flags)
/* [EnumSet-friend] */

/* [EnumSet-templated] */
namespace Implementation {
    enum class ObjectFlag: unsigned int {
        Dirty = 1 << 0,
        Marked = 1 << 1
    };

    typedef Containers::EnumSet<ObjectFlag> ObjectFlags;
    CORRADE_ENUMSET_OPERATORS(ObjectFlags)
}

template<class T> class Object {
    public:
        typedef Implementation::ObjectFlag Flag;
        typedef Implementation::ObjectFlags Flags;
};
/* [EnumSet-templated] */

enum class Feature: unsigned int;
typedef Containers::EnumSet<Feature> Features;
Utility::Debug& operator<<(Utility::Debug& debug, Features value);
/* [enumSetDebugOutput] */
enum class Feature: unsigned int {
    Fast = 1 << 0,
    Cheap = 1 << 1,
    Tested = 1 << 2,
    Popular = 1 << 3
};

// already defined to print values as e.g. Feature::Fast and Features(0xabcd)
// for unknown values
Utility::Debug& operator<<(Utility::Debug&, Feature);

typedef Containers::EnumSet<Feature> Features;
CORRADE_ENUMSET_OPERATORS(Features)

Utility::Debug& operator<<(Utility::Debug& debug, Features value) {
    return Containers::enumSetDebugOutput(debug, value, "Features{}", {
        Feature::Fast,
        Feature::Cheap,
        Feature::Tested,
        Feature::Popular});
}
/* [enumSetDebugOutput] */

namespace LL1 {
class Object;
/* [LinkedList-list-pointer] */
class ObjectGroup: public Containers::LinkedList<Object> {
    // ...
};

class Object: public Containers::LinkedListItem<Object, ObjectGroup> {
    public:
        ObjectGroup* group() { return list(); }

    // ...
};
/* [LinkedList-list-pointer] */
}

namespace LL2 {
class Object;
/* [LinkedList-private-inheritance] */
class ObjectGroup: private Containers::LinkedList<Object> {
    friend Containers::LinkedList<Object>;
    friend Containers::LinkedListItem<Object, ObjectGroup>;

    public:
        Object* firstObject() { return first(); }
        Object* lastObject() { return last(); }

    // ...
};

class Object: private Containers::LinkedListItem<Object, ObjectGroup> {
    friend Containers::LinkedList<Object>;
    friend Containers::LinkedListItem<Object, ObjectGroup>;

    public:
        ObjectGroup* group() { return list(); }
        Object* previousObject() { return previous(); }
        Object* nextObject() { return next(); }

    // ...
};
/* [LinkedList-private-inheritance] */
}

int main() {

{
/* [Array-usage] */
// Create default-initialized array with 5 integers and set them to some value
Containers::Array<int> a{5};
int b = 0;
for(auto& i: a) i = b++; // a = {0, 1, 2, 3, 4}

// Create array from given values
Containers::Array<int> c{Containers::InPlaceInit, {3, 18, -157, 0}};
c[3] = 25; // b = {3, 18, -157, 25}
/* [Array-usage] */
}

{
/* [Array-initialization] */
// These are equivalent
Containers::Array<int> a1{5};
Containers::Array<int> a2{Containers::DefaultInit, 5};

// Array of 100 zeros
Containers::Array<int> b{Containers::ValueInit, 100};

// Array of type with no default constructor
struct Vec3 {
    explicit Vec3(float, float, float) {}
};
Containers::Array<Vec3> c{Containers::DirectInit, 5, 5.2f, 0.4f, 1.0f};

// Manual construction of each element
struct Foo {
    explicit Foo(int) {}
};
Containers::Array<Foo> d{Containers::NoInit, 5};
int index = 0;
for(Foo& f: d) new(&f) Foo(index++);
/* [Array-initialization] */
}

/* [Array-wrapping] */
{
    int* data = reinterpret_cast<int*>(std::malloc(25*sizeof(int)));

    // Will call std::free() on destruction
    Containers::Array<int> array{data, 25,
        [](int* data, std::size_t) { std::free(data); }};
}
/* [Array-wrapping] */

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
{
typedef std::uint64_t GLuint;
void* glMapNamedBuffer(GLuint, int);
void glUnmapNamedBuffer(GLuint);
#define GL_READ_WRITE 0
std::size_t bufferSize{};
/* [Array-deleter] */
class UnmapBuffer {
    public:
        explicit UnmapBuffer(GLuint id): _id{id} {}
        void operator()(char*, std::size_t) { glUnmapNamedBuffer(_id); }

    private:
        GLuint _id;
};

GLuint buffer;
char* data = reinterpret_cast<char*>(glMapNamedBuffer(buffer, GL_READ_WRITE));

// Will unmap the buffer on destruction
Containers::Array<char, UnmapBuffer> array{data, bufferSize, UnmapBuffer{buffer}};
/* [Array-deleter] */
}
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

{
/* [Array-arrayView] */
Containers::Array<std::uint32_t> data;

Containers::ArrayView<std::uint32_t> a{data};
auto b = Containers::arrayView(data);
/* [Array-arrayView] */
static_cast<void>(a);
static_cast<void>(b);
}

{
/* [Array-arrayView-const] */
const Containers::Array<std::uint32_t> data;

Containers::ArrayView<const std::uint32_t> a{data};
auto b = Containers::arrayView(data);
/* [Array-arrayView-const] */
static_cast<void>(a);
static_cast<void>(b);
}

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
{
/* [ArrayView-usage] */
// `a` gets implicitly converted to const array view
void printArray(Containers::ArrayView<const float> values);
Containers::Array<float> a;
printArray(a);

// Wrapping compile-time array with size information
constexpr const int data[]{ 5, 17, -36, 185 };
Containers::ArrayView<const int> b = data; // b.size() == 4

// Wrapping general array with size information
const int* data2;
Containers::ArrayView<const int> c{data2, 3};
/* [ArrayView-usage] */
static_cast<void>(b);
}
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

{
/* [ArrayView-void-usage] */
Containers::Array<int> a(5);

Containers::ArrayView<const void> b(a); // b.size() == 20
/* [ArrayView-void-usage] */
static_cast<void>(b);
}

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
{
/* [arrayView] */
std::uint32_t* data;

Containers::ArrayView<std::uint32_t> a{data, 5};
auto b = Containers::arrayView(data, 5);
/* [arrayView] */
static_cast<void>(b);
}
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

{
/* [arrayView-array] */
std::uint32_t data[15];

Containers::ArrayView<std::uint32_t> a{data};
auto b = Containers::arrayView(data);
/* [arrayView-array] */
static_cast<void>(b);
}

{
/* [arrayView-StaticArrayView] */
std::uint32_t data[15];

Containers::ArrayView<std::uint32_t> a{data};
auto b = Containers::arrayView(data);
/* [arrayView-StaticArrayView] */
static_cast<void>(b);
}

{
/* [arrayCast] */
std::int32_t data[15];
auto a = Containers::arrayView(data); // a.size() == 15
auto b = Containers::arrayCast<char>(a); // b.size() == 60
/* [arrayCast] */
static_cast<void>(b);
}

{
/* [arraySize] */
std::int32_t a[5];

std::size_t size = Containers::arraySize(a); // size == 5
/* [arraySize] */
static_cast<void>(size);
}

{
/* [StaticArrayView-usage] */
Containers::ArrayView<int> data;

// Take elements 7 to 11
Containers::StaticArrayView<5, int> fiveInts = data.slice<5>(7);

// Convert back to ArrayView
Containers::ArrayView<int> fiveInts2 = data; // fiveInts2.size() == 5
Containers::ArrayView<int> threeInts = data.slice(2, 5);
/* [StaticArrayView-usage] */
static_cast<void>(fiveInts);
static_cast<void>(fiveInts2);
static_cast<void>(threeInts);
}

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
{
/* [staticArrayView] */
std::uint32_t* data;

Containers::StaticArrayView<5, std::uint32_t> a{data};
auto b = Containers::staticArrayView<5>(data);
/* [staticArrayView] */
static_cast<void>(b);
}
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

{
/* [staticArrayView-array] */
std::uint32_t data[15];

Containers::StaticArrayView<15, std::uint32_t> a{data};
auto b = Containers::staticArrayView(data);
/* [staticArrayView-array] */
static_cast<void>(b);
}

{
/* [arrayCast-StaticArrayView] */
std::int32_t data[15];
auto a = Containers::staticArrayView(data); // a.size() == 15
Containers::StaticArrayView<60, char> b = Containers::arrayCast<char>(a);
/* [arrayCast-StaticArrayView] */
static_cast<void>(b);
}

{
/* [arrayCast-StaticArrayView-array] */
std::int32_t data[15];
auto a = Containers::arrayCast<char>(data); // a.size() == 60
/* [arrayCast-StaticArrayView-array] */
static_cast<void>(a);
}

{
/* [enumSetDebugOutput-usage] */
// prints Feature::Fast|Feature::Cheap
Utility::Debug{} << (Feature::Fast|Feature::Cheap);

// prints Feature::Popular|Feature(0xdead)
Utility::Debug{} << (Feature::Popular|Feature(0xdead));

// prints Features{}
Utility::Debug{} << Features{};
/* [enumSetDebugOutput-usage] */
}

{
/* [LinkedList-usage] */
class Object: public Containers::LinkedListItem<Object> {
    // ...
};

Object a, b, c;

Containers::LinkedList<Object> list;
list.insert(&a);
list.insert(&b);
list.insert(&c);

list.cut(&b);
/* [LinkedList-usage] */

#if defined(__GNUC__) || defined( __clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
/* [LinkedList-traversal] */
for(Object& o: list) {
    // ...
}
/* [LinkedList-traversal] */
#if defined(__GNUC__) || defined( __clang__)
#pragma GCC diagnostic pop
#endif

/* [LinkedList-traversal-classic] */
for(Object* i = list.first(); i; i = i->next()) {
    // ...
}
/* [LinkedList-traversal-classic] */

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
{
Object *item, *before;
Containers::LinkedList<Object> list;
/* [LinkedList-move] */
if(item != before) {
    list.cut(item);
    list.move(item, before);
}
/* [LinkedList-move] */

/* [LinkedList-erase] */
list.cut(item);
delete item;
/* [LinkedList-erase] */
}
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

{
/* [LinkedListItem-usage] */
class Item: public Containers::LinkedListItem<Item> {
    // ...
};
/* [LinkedListItem-usage] */
}

{
/* [optional] */
std::string value;

auto a = Containers::Optional<std::string>{value};
auto b = Containers::optional(value);
/* [optional] */
}

#ifdef __linux__
/* [ScopedExit-usage] */
{
    int fd = open("file.dat", O_RDONLY);
    Containers::ScopedExit e{fd, close};
} // fclose(f) gets called at the end of the scope
/* [ScopedExit-usage] */
#endif

/* [ScopedExit-lambda] */
FILE* f{};

{
    f = fopen("file.dat", "r");
    Containers::ScopedExit e{&f, [](FILE** f) {
        fclose(*f);
        *f = nullptr;
    }};
}

// f is nullptr again
/* [ScopedExit-lambda] */

/* [ScopedExit-returning-lambda] */
{
    auto closer = [](FILE* f) {
        return fclose(f) != 0;
    };

    FILE* f = fopen("file.dat", "r");
    Containers::ScopedExit e{f, static_cast<bool(*)(FILE*)>(closer)};
}
/* [ScopedExit-returning-lambda] */

{
/* [StaticArray-usage] */
// Create default-initialized array with 5 integers and set them to some value
Containers::StaticArray<5, int> a;
int b = 0;
for(auto& i: a) i = b++; // a = {0, 1, 2, 3, 4}

// Create array from given values
Containers::StaticArray<4, int> c{3, 18, -157, 0};
c[3] = 25; // b = {3, 18, -157, 25}
/* [StaticArray-usage] */
}

{
/* [StaticArray-initialization] */
// These two are equivalent
Containers::StaticArray<5, int> a1;
Containers::StaticArray<5, int> a2{Containers::DefaultInit};

// Array of 100 zeros
Containers::StaticArray<100, int> b{Containers::ValueInit};

// Array of 4 values initialized in-place (these two are equivalent)
Containers::StaticArray<4, int> c1{3, 18, -157, 0};
Containers::StaticArray<4, int> c2{Containers::InPlaceInit, 3, 18, -157, 0};

// Array of type with no default constructor
struct Vec3 {
    explicit Vec3(float, float, float) {}
};
Containers::StaticArray<5, Vec3> d{Containers::DirectInit, 5.2f, 0.4f, 1.0f};

// Manual construction of each element
struct Foo {
    explicit Foo(int) {}
};
Containers::StaticArray<5, Foo> e{Containers::NoInit};
int index = 0;
for(Foo& f: e) new(&f) Foo(index++);
/* [StaticArray-initialization] */
}

{
/* [StaticArray-arrayView] */
Containers::StaticArray<5, std::uint32_t> data;

Containers::ArrayView<std::uint32_t> a{data};
auto b = Containers::arrayView(data);
/* [StaticArray-arrayView] */
static_cast<void>(a);
static_cast<void>(b);
}

{
/* [StaticArray-arrayView-const] */
const Containers::StaticArray<5, std::uint32_t> data;

Containers::ArrayView<const std::uint32_t> a{data};
auto b = Containers::arrayView(data);
/* [StaticArray-arrayView-const] */
static_cast<void>(a);
static_cast<void>(b);
}

{
/* [StaticArray-staticArrayView] */
Containers::StaticArray<5, std::uint32_t> data;

Containers::StaticArrayView<5, std::uint32_t> a{data};
auto b = Containers::staticArrayView(data);
/* [StaticArray-staticArrayView] */
static_cast<void>(a);
static_cast<void>(b);
}

{
/* [StaticArray-staticArrayView-const] */
const Containers::StaticArray<5, std::uint32_t> data;

Containers::StaticArrayView<5, const std::uint32_t> a{data};
auto b = Containers::staticArrayView(data);
/* [StaticArray-staticArrayView-const] */
static_cast<void>(a);
static_cast<void>(b);
}

{
/* [StridedArrayView-usage] */
struct Position {
    float x, y;
};

Position positions[]{{-0.5f, -0.5f}, { 0.5f, -0.5f}, { 0.0f,  0.5f}};

Containers::StridedArrayView<float> horizontalPositions{
    &positions[0].x, Containers::arraySize(positions), sizeof(Position)};

/* Move to the right */
for(float& x: horizontalPositions) x += 3.0f;
/* [StridedArrayView-usage] */
}

{

/* [StridedArrayView-usage-conversion] */
int data[] { 1, 42, 1337, -69 };

Containers::StridedArrayView<int> view1{data, 4, sizeof(int)};
Containers::StridedArrayView<int> view2 = data;
/* [StridedArrayView-usage-conversion] */
static_cast<void>(view2);
}

}
