#pragma once

#include <type_traits>
#include <memory>

namespace test {

template <typename...>
struct Ts {};

template <typename A, typename B>
struct Pair {};

template <template <typename...> class Template, typename... Args>
struct BindFront {
  template <typename... Ts>
  using TN = Template<Args..., Ts...>;

  template <typename Ts>
  using T1 = TN<Ts>;
};

template <template <typename...> class Template>
struct TransferTo {
  template <typename Ts>
  struct TransferHolder;

  template <template <typename...> class T, typename... Types>
  struct TransferHolder<T<Types...>> { using type = Template<Types...>; };

  template <typename Ts>
  using Transfer = typename TransferHolder<Ts>::type;
};

template <typename Ts1, typename Ts2>
struct ZipHolder;

template <typename... Tx1, typename... Tx2>
struct ZipHolder<Ts<Tx1...>, Ts<Tx2...>> {
  using type = Ts<Pair<Tx1, Tx2>...>;
};

template <typename Ts1, typename Ts2>
using Zip = typename ZipHolder<Ts1, Ts2>::type;

template <typename... Tss>
struct ConcatHolder;

template <typename... Tss>
using Concat = typename ConcatHolder<Tss...>::type;

template <typename... Tx1, typename... Tx2>
struct ConcatHolder<Ts<Tx1...>, Ts<Tx2...>> {
  using type = Ts<Tx1..., Tx2...>;
};

template <typename Ts, typename... Tss>
struct ConcatHolder<Ts, Tss...> {
  using type = Concat<Ts, Concat<Tss...>>;
};

template <typename Ts1, typename Ts2>
struct CrossHolder;

template <typename Ts1, typename Ts2>
using Cross = typename CrossHolder<Ts1, Ts2>::type;

template <typename T>
struct CrossHolder<Ts<>, T> { using type = Ts<>; };

template <typename T, typename... Tx1, typename... Tx2>
struct CrossHolder<Ts<T, Tx1...>, Ts<Tx2...>> {
  using type = Concat<Ts<Pair<T, Tx2>...>, Cross<Ts<Tx1...>, Ts<Tx2...>>>;
};

template <typename Ts, template <typename> class Predicate>
struct FilterHolder;

template <typename Ts, template <typename> class Predicate>
using Filter = typename FilterHolder<Ts, Predicate>::type;

template <template <typename> class Predicate>
struct FilterHolder<Ts<>, Predicate> { using type = Ts<>; };

template <typename T, typename... Types, template <typename> class Predicate>
struct FilterHolder<Ts<T, Types...>, Predicate> {
  using Rest = Filter<Ts<Types...>, Predicate>;
  using type = std::conditional_t<Predicate<T>::value, Concat<Ts<T>, Rest>, Rest>;
};

template <template <typename...> class Mapper, typename Ts>
struct MapHolder;

template <template <typename...> class Mapper, typename Ts>
using Map = typename MapHolder<Mapper, Ts>::type;

template <template <typename...> class Mapper>
struct MapHolder<Mapper, Ts<>> {
  using type = Ts<>;
};

template <template <typename...> class Mapper, typename T, typename... Tx>
struct MapHolder<Mapper, Ts<T, Tx...>> {
  using type = Concat<Ts<Mapper<T>>, Map<Mapper, Ts<Tx...>>>;
};

template <typename Tss>
struct FlattenHolder;

template <typename... Tss>
struct FlattenHolder<Ts<Tss...>> { using type = Concat<Tss...>; };

template <typename Tss>
using Flatten = typename FlattenHolder<Tss>::type;

template <typename Ts>
struct PowersetHolder;

template <typename Ts>
using Powerset = typename PowersetHolder<Ts>::type;

template <>
struct PowersetHolder<Ts<>> { using type = Ts<Ts<>>; };

template <typename T, typename... Tx>
struct PowersetHolder<Ts<T, Tx...>> {
  template <typename List>
  using Mapper = Concat<Ts<T>, List>;

  using PowersetRest = Powerset<Ts<Tx...>>;
  using type = Concat<PowersetRest, Map<Mapper, PowersetRest>>;
};

template <typename Functor, typename... Types>
void InstantiateAndCall(Ts<Types...>) {
  ( Functor::call(static_cast<Types*>(nullptr)), ... );
};

namespace ut {

static_assert(std::is_same_v<Ts<>, Concat<Ts<>, Ts<>>>);
static_assert(std::is_same_v<Ts<int, int, int>, Concat<Ts<int, int, int>, Ts<>>>);
static_assert(std::is_same_v<Ts<int, int, int>, Concat<Ts<>, Ts<int, int, int>>>);
static_assert(std::is_same_v<
      Ts<int, int, int, float, float, float>,
      Concat<Ts<int, int, int>, Ts<float, float, float>>>);

static_assert(std::is_same_v<Ts<>, Map<Ts, Ts<>>>);
static_assert(std::is_same_v<
      Ts<Ts<int>, Ts<float>, Ts<short>>,
      Map<Ts, Ts<int, float, short>>>);

static_assert(std::is_same_v<Powerset<Ts<>>, Ts<Ts<>>>);
static_assert(std::is_same_v<Powerset<Ts<int>>, Ts<Ts<>, Ts<int>>>);
static_assert(std::is_same_v<
    Powerset<Ts<int, float>>,
    Ts<Ts<>, Ts<float>, Ts<int>, Ts<int, float>>>);

static_assert(std::is_same_v<Cross<Ts<>, Ts<>>, Ts<>>);
static_assert(std::is_same_v<Cross<Ts<int>, Ts<>>, Ts<>>);
static_assert(std::is_same_v<Cross<Ts<>, Ts<int>>, Ts<>>);
static_assert(std::is_same_v<Cross<Ts<int>, Ts<char>>, Ts<Pair<int, char>>>);
static_assert(std::is_same_v<
    Cross<
      Ts<int, char>,
      Ts<float, short>>,
    Ts<
      Pair<int, float>,
      Pair<int, short>,
      Pair<char, float>,
      Pair<char, short>>>);

}  // namespace ut

}  // namespace test

