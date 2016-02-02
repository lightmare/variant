#include "catch.hpp"
#include "variant.hpp"

#include <type_traits>

// helper macros for concise checks, will be expanded in failure output
#define nothrow_default(type, ...)      (std::is_nothrow_default_constructible<type<__VA_ARGS__>>::value)
#define nothrow_copy(type, ...)         (std::is_nothrow_copy_constructible<type<__VA_ARGS__>>::value)
#define nothrow_move(type, ...)         (std::is_nothrow_move_constructible<type<__VA_ARGS__>>::value)
#define nothrow_destruct(type, ...)     (mapbox::util::detail::is_nothrow_destructible<type<__VA_ARGS__>>::value)

#undef CHECK_NOFAIL
#define CHECK_NOFAIL CHECK

namespace // internal
{

struct throwing_default_ctor
{
    throwing_default_ctor() noexcept(false)
    { throw std::runtime_error("throwing_default_ctor"); }
};

struct throwing_copy_ctor
{
    throwing_copy_ctor() = default;
    throwing_copy_ctor(throwing_copy_ctor && ) = default;
    throwing_copy_ctor(throwing_copy_ctor const& ) noexcept(false)
    { throw std::runtime_error("throwing_copy_ctor"); }
};

struct throwing_move_ctor
{
    throwing_move_ctor() = default;
    throwing_move_ctor(throwing_move_ctor const& ) = default;
    throwing_move_ctor(throwing_move_ctor && ) noexcept(false)
    { throw std::runtime_error("throwing_copy_ctor"); }
};

struct throwing_dtor
{
    throwing_dtor() = default;
    throwing_dtor(throwing_dtor const& ) = default;
    throwing_dtor(throwing_dtor && ) = default;
    ~throwing_dtor() noexcept(false)
    { throw std::runtime_error("throwing_dtor"); }
};

} // namespace

TEST_CASE("exception-specification : simple variant")
{
    // variant delegates noexcept to tuple in some cases; that's why we check
    // tuple as well as variant here -- when a check on variant fails, seeing
    // what the tuple's noexcept is can help us locate the culprit
    using std::tuple;
    using mapbox::util::variant;

    SECTION("default constructor")
    {
        CHECK_NOFAIL(nothrow_default(tuple, int, float) == true);
        CHECK_NOFAIL(nothrow_default(tuple, int, throwing_default_ctor) == false);
        CHECK_NOFAIL(nothrow_default(tuple, throwing_default_ctor, int) == false);
        CHECK_NOFAIL(nothrow_default(tuple, throwing_default_ctor) == false);
        CHECK_NOFAIL(nothrow_default(tuple, throwing_copy_ctor, throwing_move_ctor) == true);
        CHECK_NOFAIL(nothrow_default(tuple, throwing_default_ctor, throwing_copy_ctor, throwing_move_ctor) == false);

        CHECK(nothrow_default(variant, int, float) == true);
        CHECK(nothrow_default(variant, int, throwing_default_ctor) == true);
        CHECK(nothrow_default(variant, throwing_default_ctor, int) == false);
        CHECK(nothrow_default(variant, throwing_default_ctor) == false);
        CHECK(nothrow_default(variant, throwing_copy_ctor, throwing_move_ctor) == true);
        CHECK(nothrow_default(variant, throwing_default_ctor, throwing_copy_ctor, throwing_move_ctor) == false);
    }

    SECTION("copy constructor")
    {
        CHECK_NOFAIL(nothrow_copy(tuple, int, float) == true);
        CHECK_NOFAIL(nothrow_copy(tuple, int, throwing_copy_ctor) == false);
        CHECK_NOFAIL(nothrow_copy(tuple, throwing_copy_ctor) == false);
        CHECK_NOFAIL(nothrow_copy(tuple, throwing_default_ctor, throwing_move_ctor) == true);
        CHECK_NOFAIL(nothrow_copy(tuple, throwing_default_ctor, throwing_copy_ctor, throwing_move_ctor) == false);

        CHECK(nothrow_copy(variant, int, float) == true);
        CHECK(nothrow_copy(variant, int, throwing_copy_ctor) == false);
        CHECK(nothrow_copy(variant, throwing_copy_ctor) == false);
        CHECK(nothrow_copy(variant, throwing_default_ctor, throwing_move_ctor) == true);
        CHECK(nothrow_copy(variant, throwing_default_ctor, throwing_copy_ctor, throwing_move_ctor) == false);
    }

    SECTION("move constructor")
    {
        CHECK_NOFAIL(nothrow_move(tuple, int, float) == true);
        CHECK_NOFAIL(nothrow_move(tuple, int, throwing_move_ctor) == false);
        CHECK_NOFAIL(nothrow_move(tuple, throwing_move_ctor) == false);
        CHECK_NOFAIL(nothrow_move(tuple, throwing_default_ctor, throwing_copy_ctor) == true);
        CHECK_NOFAIL(nothrow_move(tuple, throwing_default_ctor, throwing_copy_ctor, throwing_move_ctor) == false);

        CHECK(nothrow_move(variant, int, float) == true);
        CHECK(nothrow_move(variant, int, throwing_move_ctor) == false);
        CHECK(nothrow_move(variant, throwing_move_ctor) == false);
        CHECK(nothrow_move(variant, throwing_default_ctor, throwing_copy_ctor) == true);
        CHECK(nothrow_move(variant, throwing_default_ctor, throwing_copy_ctor, throwing_move_ctor) == false);
    }

    SECTION("destructor")
    {
        CHECK_NOFAIL(nothrow_destruct(tuple, int, float) == true);
        CHECK_NOFAIL(nothrow_destruct(tuple, int, throwing_dtor) == false);
        CHECK_NOFAIL(nothrow_destruct(tuple, throwing_dtor) == false);

        CHECK(nothrow_destruct(variant, int, float) == true);
        CHECK(nothrow_destruct(variant, int, throwing_dtor) == false);
        CHECK(nothrow_destruct(variant, throwing_dtor) == false);
    }
}

namespace // internal
{

template <typename T> struct wrapped_alternative;
template <typename T> using wrap = mapbox::util::recursive_wrapper<wrapped_alternative<T>>;
template <typename T> using recursive_variant = mapbox::util::variant<wrap<T>>;

template <typename T>
struct wrapped_alternative : T
{
    recursive_variant<T> var;
};

} // namespace

namespace mapbox { namespace util {

template <typename T>
struct recursive_wrapper_traits<T,
    typename std::enable_if< std::is_base_of<throwing_dtor, T>::value >::type>
{
    static constexpr bool is_nothrow_destructible = false;
};

}} // namespace

TEST_CASE("exception-specification : recursive variant")
{
    SECTION("default constructor")
    {
        // default-constructed wrapper allocates new T (default-constructed)
        CHECK(nothrow_default(recursive_variant, throwing_default_ctor) == false);
        CHECK(nothrow_default(recursive_variant, throwing_copy_ctor) == false);
        CHECK(nothrow_default(recursive_variant, throwing_move_ctor) == false);
        CHECK(nothrow_default(recursive_variant, throwing_dtor) == false);
    }

    SECTION("copy constructor")
    {
        // wrapper copy allocates new T (copy-constructed)
        CHECK(nothrow_copy(recursive_variant, throwing_default_ctor) == false);
        CHECK(nothrow_copy(recursive_variant, throwing_copy_ctor) == false);
        CHECK(nothrow_copy(recursive_variant, throwing_move_ctor) == false);
        CHECK(nothrow_copy(recursive_variant, throwing_dtor) == false);
    }

    SECTION("move constructor")
    {
        // wrapper move doesn't move the wrapped value, only pointer
        CHECK(nothrow_move(recursive_variant, throwing_default_ctor) == true);
        CHECK(nothrow_move(recursive_variant, throwing_copy_ctor) == true);
        CHECK(nothrow_move(recursive_variant, throwing_move_ctor) == true);
        // is_nothrow_move_constructible check probably involves destruction
        // of the source, which is why this check fails (Clang 3.6, GCC 4.8)
        CHECK_NOFAIL(nothrow_move(recursive_variant, throwing_dtor) == true);
    }

    SECTION("destructor")
    {
        CHECK(nothrow_destruct(recursive_variant, throwing_default_ctor) == true);
        CHECK(nothrow_destruct(recursive_variant, throwing_copy_ctor) == true);
        CHECK(nothrow_destruct(recursive_variant, throwing_move_ctor) == true);
        CHECK(nothrow_destruct(recursive_variant, throwing_dtor) == false);
    }
}
