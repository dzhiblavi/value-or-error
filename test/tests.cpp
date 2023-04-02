#include <gtest/gtest.h>
#include <limits>
#include <type_traits>

#include "value_or_error.h"
#include "remember_op.h"
#include "tools.h"
#include "voe_tools.h"

namespace voe {

template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename ValueType, typename VisitType, typename... ErrorTypes>
struct VisitChecker {
  static void Check(ValueOrError<ValueType, ErrorTypes...>& voe) {
    if constexpr (std::is_same_v<void, VisitType>) {
      EXPECT_DEATH(
          voe.Visit(overloaded{[]() {}, [](auto&&) {}}),
          "Visit\\(\\) called on an empty object");
    } else {
      voe.Visit(overloaded{
          []() { FAIL() << "Unexpected visit of (void)"; },
          [](auto&&) { FAIL() << "Unexpected visit of other type"; },
          [](VisitType&) {},
      });
    }
  }
};

template <typename VisitType, typename... ErrorTypes>
struct VisitChecker<void, VisitType, ErrorTypes...> {
  static void Check(ValueOrError<void, ErrorTypes...>& voe) {
    if constexpr (std::is_same_v<void, VisitType>) {
      voe.Visit(overloaded{
          [](auto&&) { FAIL() << "Unexpected visit of some error type"; },
          [](void) {},
      });
    } else {
      voe.Visit(overloaded{
          []() { FAIL() << "Unexpected visit of (void)"; },
          [](auto&&) { FAIL() << "Unexpected visit of other type"; },
          [](VisitType&) {},
      });
    }
  }
};

struct DefaultConstructorTest {
  template <typename ValueType, typename... ErrorTypes>
  static void call(ValueOrError<ValueType, ErrorTypes...>*) {
    test::OpCollector collector;
    ValueOrError<ValueType, ErrorTypes...> voe;

    EXPECT_TRUE(collector.Equal(/* empty */));
    EXPECT_TRUE(voe.IsEmpty());
    EXPECT_FALSE(voe.HasValue());
    EXPECT_FALSE(voe.HasAnyError());
    EXPECT_FALSE(... || voe.template HasError<ErrorTypes>());
    VisitChecker<ValueType, void, ErrorTypes...>::Check(voe);
  }
};

struct ValueConstructorTest {
  template <typename ValueType, typename... ErrorTypes>
  static void call(ValueOrError<ValueType, ErrorTypes...>*) {
    test::OpCollector collector;
    ValueOrError<ValueType, ErrorTypes...> voe(ValueType{});

    EXPECT_TRUE(collector.Equal(
        test::Op(test::Create, ValueType::Idx),
        test::Op(test::CONSTRUCT_MOVE, ValueType::Idx),
        test::Op(test::Destroy, ValueType::Idx)));

    EXPECT_FALSE(voe.IsEmpty());
    EXPECT_TRUE(voe.HasValue());
    EXPECT_FALSE(voe.HasAnyError());
    EXPECT_FALSE(... || voe.template HasError<ErrorTypes>());
    EXPECT_EQ(ValueType{}, voe.GetValue());
    VisitChecker<ValueType, ValueType, ErrorTypes...>::Check(voe);
  }
};

struct MakeErrorTest {
  template <typename ValueType, typename... ErrorTypes>
  static void call(ValueOrError<ValueType, ErrorTypes...>* ptr) {
    using ValueOrError = ValueOrError<ValueType, ErrorTypes...>;
    ( TestMakeError(ptr, static_cast<ErrorTypes*>(nullptr)), ... );
  }

  template <typename ValueType, typename... ErrorTypes, typename ErrorType>
  static void TestMakeError(ValueOrError<ValueType, ErrorTypes...>*, ErrorType*) {
    static constexpr size_t error_index = detail_::TypeToIndex<ErrorType, ErrorTypes...>;

    test::OpCollector collector;
    ValueOrError<ValueType, ErrorTypes...> voe{MakeError<ErrorType>()};

    EXPECT_TRUE(collector.Equal(
        test::Op(test::Create, ErrorType::Idx),
        test::Op(test::CONSTRUCT_MOVE, ErrorType::Idx),
        test::Op(test::Destroy, ErrorType::Idx)));

    EXPECT_TRUE(voe.HasAnyError());
    EXPECT_EQ(error_index, voe.GetErrorIndex());
    EXPECT_TRUE(voe.template HasError<ErrorType>());
    EXPECT_EQ(1, (... + int(voe.template HasError<ErrorTypes>())));
    EXPECT_EQ(ErrorType{}, voe.template GetError<ErrorType>());
    VisitChecker<ValueType, ErrorType, ErrorTypes...>::Check(voe);
  }
};

struct CreateBase {
  template <typename... Types>
  struct First;
  template <typename T, typename... Ts>
  struct First<T, Ts...> { using type = T; };

  struct EmptyMarker {};

  template <typename T>
  struct ValueMarker {};

  template <typename T>
  struct ErrorMarker {};

  template <typename ValueType, typename... ErrorTypes>
  static ValueOrError<ValueType, ErrorTypes...> Create(
      ValueOrError<ValueType, ErrorTypes...>*, EmptyMarker)
  { return {}; }

  template <typename ValueType, typename... ErrorTypes, typename T>
  static ValueOrError<ValueType, ErrorTypes...> Create(
      ValueOrError<ValueType, ErrorTypes...>*, ValueMarker<T>)
  { return T{}; }

  template <typename ValueType, typename... ErrorTypes, typename T>
  static ValueOrError<ValueType, ErrorTypes...> Create(
      ValueOrError<ValueType, ErrorTypes...>*, ErrorMarker<T>)
  { return MakeError<T>(); }

  template <typename ValueType, typename... ErrorTypes>
  static ValueOrError<ValueType, ErrorTypes...> CreateEmpty(
      ValueOrError<ValueType, ErrorTypes...>* ptr)
  { return Create(ptr, EmptyMarker{}); }

  template <typename ValueType, typename... ErrorTypes, typename T>
  static ValueOrError<ValueType, ErrorTypes...> CreateOther(
      ValueOrError<ValueType, ErrorTypes...>*, ValueMarker<T>)
  { return MakeError<typename First<ErrorTypes...>::type>(); }

  template <typename ValueType, typename... ErrorTypes, typename T>
  static ValueOrError<ValueType, ErrorTypes...> CreateOther(
      ValueOrError<ValueType, ErrorTypes...>*, ErrorMarker<T>)
  { return ValueType{}; }

  template <typename ValueType, typename... ErrorTypes, typename T>
  static constexpr int OtherIndex(
      ValueOrError<ValueType, ErrorTypes...>*, ValueMarker<T>)
  { return First<ErrorTypes...>::type::Idx; }

  template <typename ValueType, typename... ErrorTypes, typename T>
  static constexpr int OtherIndex(
      ValueOrError<ValueType, ErrorTypes...>*, ErrorMarker<T>)
  { return ValueType::Idx; }
};

struct ConstructorsTest : CreateBase {
  template <typename ValueType1, typename... ErrorTypes1,
            typename ValueType2, typename... ErrorTypes2>
  static void call(
      test::Pair<
        ValueOrError<ValueType1, ErrorTypes1...>,
        ValueOrError<ValueType2, ErrorTypes2...>>* p)
  {
    using ValueOrError1 = ValueOrError<ValueType1, ErrorTypes1...>;
    using ValueOrError2 = ValueOrError<ValueType2, ErrorTypes2...>;

    static constexpr bool both_non_void =
      !std::is_same_v<void, ValueType1> && !std::is_same_v<void, ValueType2>;

    // VoE(Empty): No operations expected
    TestConstructFrom(p, EmptyMarker{});

    if constexpr (both_non_void) {
      // VoE(ValueTypeVoE{}): Copy/Move construction expected
      TestConstructFrom(p, ValueMarker<ValueType1>{});
    }

    // VoE(ErrorTypeVoE{}): Copy/Move construction expected
    ( TestConstructFrom(p, ErrorMarker<ErrorTypes1>{}), ... );
  }

  static void Check(test::OpCollector& collector, test::Type, EmptyMarker) {
    EXPECT_TRUE(collector.Equal());
  }

  template <typename T>
  static void Check(test::OpCollector& collector, test::Type type, ValueMarker<T>) {
    EXPECT_TRUE(collector.Equal(test::Op(type, T::Idx)));
  }

  template <typename T>
  static void Check(test::OpCollector& collector, test::Type type, ErrorMarker<T>) {
    EXPECT_TRUE(collector.Equal(test::Op(type, T::Idx)));
  }

  template <typename ValueType1, typename... ErrorTypes1,
            typename ValueType2, typename... ErrorTypes2,
            typename ConstructAs>
  static void TestConstructFrom(
      test::Pair<
        ValueOrError<ValueType1, ErrorTypes1...>,
        ValueOrError<ValueType2, ErrorTypes2...>>* p, ConstructAs as)
  {
    using ValueOrError1 = ValueOrError<ValueType1, ErrorTypes1...>;
    using ValueOrError2 = ValueOrError<ValueType2, ErrorTypes2...>;
    {
      ValueOrError1 voe1{Create(static_cast<ValueOrError1*>(nullptr), as)};
      test::OpCollector collector;
      ValueOrError2 voe2{voe1};
      Check(collector, test::CONSTRUCT_COPY, as);
    }
    {
      ValueOrError1 voe1{Create(static_cast<ValueOrError1*>(nullptr), as)};
      const ValueOrError1& voe1_cref = voe1;
      test::OpCollector collector;
      ValueOrError2 voe2{voe1_cref};
      Check(collector, test::CONSTRUCT_COPY_CONST, as);
    }
    {
      ValueOrError1 voe1{Create(static_cast<ValueOrError1*>(nullptr), as)};
      test::OpCollector collector;
      ValueOrError2 voe2{std::move(voe1)};
      Check(collector, test::CONSTRUCT_MOVE, as);
    }
    {
      ValueOrError1 voe1{Create(static_cast<ValueOrError1*>(nullptr), as)};
      const ValueOrError1&& voe1_crref = std::move(voe1);
      test::OpCollector collector;
      ValueOrError2 voe2{std::move(voe1_crref)};
      Check(collector, test::CONSTRUCT_MOVE_CONST, as);
    }
  }
};

struct AssignmentsTest : CreateBase {
  template <typename ValueType1, typename... ErrorTypes1,
            typename ValueType2, typename... ErrorTypes2>
  static void call(
      test::Pair<
        ValueOrError<ValueType1, ErrorTypes1...>,
        ValueOrError<ValueType2, ErrorTypes2...>>* p)
  {
    using ValueOrError1 = ValueOrError<ValueType1, ErrorTypes1...>;
    using ValueOrError2 = ValueOrError<ValueType2, ErrorTypes2...>;
    ValueOrError1* p1{nullptr};
    ValueOrError2* p2{nullptr};

    // Empty <- Empty
    // Assign empty to empty voe
    // No constructors or assignments expected
    TestAssign(
        /* from = */ CreateEmpty(p1),
        /* to =   */ CreateEmpty(p2),
        -1, test::None, 0);

    if constexpr (!std::is_same_v<void, ValueType2>) {
      // X <- Empty
      // Assign to voe with some value from voe with no value
      // No constructors or assignments expected
      // `to' destructor call expected
      TestAssign(
          /* from = */ CreateEmpty(p1),
          /* to =   */ Create(p2, ValueMarker<ValueType2>{}),
          -1, test::None, 0,
          test::Op(test::Destroy, ValueType2::Idx));
    }
    ( TestAssign(
        /* from = */ CreateEmpty(p1),
        /* to =   */ Create(p2, ErrorMarker<ErrorTypes2>{}),
        -1, test::None, 0,
        test::Op(test::Destroy, ErrorTypes2::Idx)), ... );

    if constexpr (!std::is_same_v<void, ValueType1> && !std::is_same_v<void, ValueType2>) {
      // Empty <- X
      // Assign voe with some value to empty voe
      // Constructor call expected
      TestAssign(
          /* from = */ Create(p1, ValueMarker<ValueType1>{}),
          /* to =   */ CreateEmpty(p2),
          ValueType1::Idx, test::CONSTRUCT_COPY, 1);
    }
    ( TestAssign(
        /* from = */ Create(p1, ErrorMarker<ErrorTypes1>{}),
        /* to =   */ CreateEmpty(p2),
        ErrorTypes1::Idx, test::CONSTRUCT_COPY, 1), ... );

    // Empty(void) <- X(non-void)
    // Assign voe with some value to empty voe (that cannot have value)
    // Death expected
    if constexpr (!std::is_same_v<void, ValueType1> && std::is_same_v<void, ValueType2>) {
      EXPECT_DEATH(
          TestAssign(
            /* from = */ Create(p1, ValueMarker<ValueType1>{}),
            /* to =   */ CreateEmpty(p2),
            ValueType1::Idx, test::CONSTRUCT_COPY, 1),
          "this_phys_index != size_t\\(-1\\)");
    }

    // X <- X
    // Assign to voe with some value from voe with same kind of value
    // Assignment(from) operator expected
    if constexpr (!std::is_same_v<void, ValueType1> && !std::is_same_v<void, ValueType2>) {
      TestAssign(
          /* from = */ Create(p1, ValueMarker<ValueType1>{}),
          /* to =   */ Create(p2, ValueMarker<ValueType2>{}),
          ValueType1::Idx, test::ASSIGN_COPY, 1);
    }
    ( TestAssign(
        /* from = */ Create(p1, ErrorMarker<ErrorTypes1>{}),
        /* to =   */ Create(p2, ErrorMarker<ErrorTypes1>{}),
        ErrorTypes1::Idx, test::ASSIGN_COPY, 1), ... );

    if constexpr (!std::is_same_v<void, ValueType1> && !std::is_same_v<void, ValueType2>) {
      if constexpr (sizeof...(ErrorTypes1) > 0) {
        // X <- Y
        // Assign to voe with some value from voe with other kind of value
        // Destructor(to) + Constructor(from) operator expected
        TestAssign(
            /* from = */ CreateOther(p1, ValueMarker<ValueType1>{}),
            /* to =   */ Create(p2, ValueMarker<ValueType2>{}),
            OtherIndex(p1, ValueMarker<ValueType1>{}),
            test::CONSTRUCT_COPY, 1, test::Op(test::Destroy, ValueType2::Idx));
        ( TestAssign(
            /* from = */ CreateOther(p1, ErrorMarker<ErrorTypes1>{}),
            /* to =   */ Create(p2, ErrorMarker<ErrorTypes1>{}),
            OtherIndex(p1, ErrorMarker<ErrorTypes1>{}),
            test::CONSTRUCT_COPY, 1, test::Op(test::Destroy, ErrorTypes1::Idx)), ... );
      }
    }
  }

  template <typename... Ops>
  static void Check(test::OpCollector& collector, test::Type baseop, int index, Ops... ops) {
    if (baseop == test::None) {
      EXPECT_TRUE(collector.Equal(ops...));
    } else {
      EXPECT_TRUE(collector.Equal(ops..., test::Op(baseop, index)));
    }
  }

  template <typename ValueType1, typename... ErrorTypes1,
            typename ValueType2, typename... ErrorTypes2, typename... Ops>
  static void TestAssign(
      ValueOrError<ValueType1, ErrorTypes1...> v_from,
      ValueOrError<ValueType2, ErrorTypes2...> v_to, int index_from,
      test::Type baseop, int step, Ops... ops_prefix)
  {
    using ValueOrError1 = ValueOrError<ValueType1, ErrorTypes1...>;
    using ValueOrError2 = ValueOrError<ValueType2, ErrorTypes2...>;
    {
      ValueOrError2 voe{v_to};
      ValueOrError1 from{v_from};
      test::OpCollector collector;
      voe = from;
      Check(collector, static_cast<test::Type>(baseop + 0 * step), index_from, ops_prefix...);
    }
    {
      ValueOrError2 voe{v_to};
      ValueOrError1 from{v_from};
      const ValueOrError1& from_cref = from;
      test::OpCollector collector;
      voe = from_cref;
      Check(collector, static_cast<test::Type>(baseop + 1 * step), index_from, ops_prefix...);
    }
    {
      ValueOrError2 voe{v_to};
      ValueOrError1 from{v_from};
      test::OpCollector collector;
      voe = std::move(from);
      Check(collector, static_cast<test::Type>(baseop + 2 * step), index_from, ops_prefix...);
    }
    {
      ValueOrError2 voe{v_to};
      ValueOrError1 from{v_from};
      const ValueOrError1&& from_crref = std::move(from);
      test::OpCollector collector;
      voe = std::move(from_crref);
      Check(collector, static_cast<test::Type>(baseop + 3 * step), index_from, ops_prefix...);
    }
  }
};

TEST(DefaultConstructorTest, Correctness) {
  using Types = test::Concat<
    test::Types<test::RememberLastOp<0>, test::RememberLastOp<0>, short, float>,
    test::Types<test::RememberLastOp<0>, test::RememberLastOp<0>, char, short>,
    test::Types<test::RememberLastOp<0>, test::RememberLastOp<0>>
  >;
  test::InstantiateAndCall<DefaultConstructorTest>(Types{});
}

TEST(ValueConstructorTest, Correctness) {
  using Types = test::Types<
    test::RememberLastOp<0>,
    test::RememberLastOp<0>,
    int, char, std::string, const char*
  >;
  test::InstantiateAndCall<ValueConstructorTest>(Types{});
}

TEST(MakeErrorTest, Correctness) {
  using Types = test::TypesWithErrors<
    test::RememberLastOp<0>,
    test::RememberLastOp<0>,
    test::RememberLastOp<1>,
    test::RememberLastOp<2>
  >;
  test::InstantiateAndCall<MakeErrorTest>(Types{});
}

TEST(ConstructorsTest, CorrectTypeAndOperation) {
  using Types = test::AllConvertiblePairs<
    test::RememberLastOp<0>,
    test::RememberLastOp<0>,
    test::RememberLastOp<1>,
    test::RememberLastOp<2>
  >;
  test::InstantiateAndCall<ConstructorsTest>(Types{});
}

TEST(AssignmentsTest, CorrectTypeAndOperation) {
  using Types = test::AllConvertiblePairs<
    test::RememberLastOp<0>,
    test::RememberLastOp<0>,
    test::RememberLastOp<1>,
    test::RememberLastOp<2>
  >;
  test::InstantiateAndCall<AssignmentsTest>(Types{});
}

}  // namespace voe

