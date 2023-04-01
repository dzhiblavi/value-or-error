#pragma once

#include "tools.h"
#include "value_or_error.h"

namespace test {

template <typename ValueType, typename... ErrorTypes>
using Types = Map<
  TransferTo<BindFront<::voe::ValueOrError, ValueType>::template TN>::template Transfer,
  Powerset<Ts<ErrorTypes...>>
>;

template <typename ValueType, typename ValueOrError>
struct HasAnyErrorPredicate 
  : public std::negation<std::is_same<::voe::ValueOrError<ValueType>, ValueOrError>>
{};

template <typename ValueType, typename... ErrorTypes>
using TypesWithErrors = Filter<
  Types<ValueType, ErrorTypes...>,
  BindFront<HasAnyErrorPredicate, ValueType>::template T1
>;

template <typename ValueOrErrorPair>
struct ConvertiblePredicate;

template <typename ValueOrError1, typename ValueOrError2>
struct ConvertiblePredicate<Pair<ValueOrError1, ValueOrError2>>
  : public voe::detail_::ConvertibleHolder<
      voe::detail_::TransferTemplate<ValueOrError1, voe::detail_::VariadicHolder>,
      voe::detail_::TransferTemplate<ValueOrError2, voe::detail_::VariadicHolder>
    >
{};

template <typename ValueType, typename... ErrorTypes>
  requires (!std::is_same_v<void, ValueType>)
using AllConvertiblePairs = Filter<
  Concat<
    Cross<
      Types<ValueType, ErrorTypes...>,
      Types<ValueType, ErrorTypes...>
    >,
    Cross<
      Types<void, ErrorTypes...>,
      Types<ValueType, ErrorTypes...>
    >,
    Cross<
      Types<ValueType, ErrorTypes...>,
      Types<void, ErrorTypes...>
    >
  >,
  ConvertiblePredicate
>;

namespace ut {

static_assert(std::is_same_v<
    Types<void>,
    test::Ts<::voe::VoidOrError<>>>);
static_assert(std::is_same_v<
    Types<void, int>,
    test::Ts<voe::VoidOrError<>, voe::VoidOrError<int>>>);
static_assert(std::is_same_v<
    Types<void, int, float>,
    test::Ts<
      voe::VoidOrError<>,
      voe::VoidOrError<float>,
      voe::VoidOrError<int>,
      voe::VoidOrError<int, float>>>);
static_assert(std::is_same_v<
    Types<int>,
    test::Ts<voe::ValueOrError<int>>>);
static_assert(std::is_same_v<
    Types<int, float>,
    test::Ts<voe::ValueOrError<int>, voe::ValueOrError<int, float>>>);
static_assert(std::is_same_v<
    Types<void, int, float>,
    test::Ts<
      voe::VoidOrError<>,
      voe::VoidOrError<float>,
      voe::VoidOrError<int>,
      voe::VoidOrError<int, float>>>);

static_assert(std::is_same_v<
    TypesWithErrors<void>,
    test::Ts<>>);
static_assert(std::is_same_v<
    TypesWithErrors<void, int>,
    test::Ts<voe::VoidOrError<int>>>);
static_assert(std::is_same_v<
    TypesWithErrors<void, int, float>,
    test::Ts<
      voe::VoidOrError<float>,
      voe::VoidOrError<int>,
      voe::VoidOrError<int, float>>>);
static_assert(std::is_same_v<
    TypesWithErrors<int>,
    test::Ts<>>);
static_assert(std::is_same_v<
    TypesWithErrors<int, float>,
    test::Ts<voe::ValueOrError<int, float>>>);
static_assert(std::is_same_v<
    TypesWithErrors<void, int, float>,
    test::Ts<
      voe::VoidOrError<float>,
      voe::VoidOrError<int>,
      voe::VoidOrError<int, float>>>);

static_assert(std::is_same_v<
    AllConvertiblePairs<int, char>,
    Ts<
      Pair<voe::ValueOrError<int>, voe::ValueOrError<int>>,
      Pair<voe::ValueOrError<int>, voe::ValueOrError<int, char>>,
      Pair<voe::ValueOrError<int, char>, voe::ValueOrError<int, char>>,
      Pair<voe::ValueOrError<void>, voe::ValueOrError<int>>,
      Pair<voe::ValueOrError<void>, voe::ValueOrError<int, char>>,
      Pair<voe::ValueOrError<void, char>, voe::ValueOrError<int, char>>,
      Pair<voe::ValueOrError<int>, voe::ValueOrError<void>>,
      Pair<voe::ValueOrError<int>, voe::ValueOrError<void, char>>,
      Pair<voe::ValueOrError<int, char>, voe::ValueOrError<void, char>>
    >>);

}  // namespace ut

}  // namespace test

