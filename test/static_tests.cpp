#include <gtest/gtest.h>
#include <limits>
#include <type_traits>

#include "value_or_error.h"

namespace voe::detail_ {

TEST(PropagateConstTest, Correctness) {
  static_assert(std::is_same_v<void, PropagateConst<int, void>>);
  static_assert(std::is_same_v<const int, PropagateConst<const void, int>>);
  static_assert(std::is_same_v<const int, PropagateConst<const int&, int>>);
  static_assert(std::is_same_v<const int, PropagateConst<const int&&, int>>);
  static_assert(std::is_same_v<int* const, PropagateConst<const void, int*>>);
}

TEST(IndexToTypeTest, Correctness) {
  static_assert(std::is_same_v<int, IndexToType<0, int>>);
  static_assert(std::is_same_v<int, IndexToType<0, int, float, char>>);
  static_assert(std::is_same_v<float, IndexToType<1, int, float, char>>);
  static_assert(std::is_same_v<char, IndexToType<2, int, float, char>>);
}

TEST(TypeToIndexTest, Correctness) {
  static_assert(0 == TypeToIndex<int, int>);
  static_assert(0 == TypeToIndex<int, int, float, char>);
  static_assert(1 == TypeToIndex<float, int, float, char>);
  static_assert(2 == TypeToIndex<char, int, float, char>);
  static_assert(size_t(-1) == TypeToIndex<char, int, float>);
}

TEST(AllDecayedTest, Correctness) {
  static_assert(AllDecayed<>);
  static_assert(AllDecayed<int, float*, void>);
  static_assert(!AllDecayed<int&>);
  static_assert(!AllDecayed<int[]>);
  static_assert(!AllDecayed<int, float[], std::string, int*>);
  static_assert(!AllDecayed<int, float[], std::string&, int*>);
}

TEST(AllUniqueTest, Correctness) {
  static_assert(AllUnique<>);
  static_assert(AllUnique<int, float*, void>);
  static_assert(!AllUnique<int, int>);
  static_assert(!AllUnique<int, float,int>);
  static_assert(!AllUnique<int, float, float>);
  static_assert(!AllUnique<float, float, int>);
  static_assert(!AllUnique<float, int, float, int, float>);
}

TEST(RemoveTypesTest, Correctness) {
  static_assert(std::is_same_v<VariadicHolder<>, RemoveTypes<VariadicHolder<>>>);
  static_assert(std::is_same_v<VariadicHolder<int>, RemoveTypes<VariadicHolder<int>, bool>>);
  static_assert(std::is_same_v<VariadicHolder<>, RemoveTypes<VariadicHolder<int>, int>>);
  static_assert(
      std::is_same_v<
        VariadicHolder<float, char>,
        RemoveTypes<VariadicHolder<int, float, short, char>, int, short>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, char, int, void*>, int>,
        VariadicHolder<bool, char, void*>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, int, char, void*>, int>,
        VariadicHolder<bool, char, void*>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, char, void*, int>, int>,
        VariadicHolder<bool, char, void*>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, char, int, void*>, int, void*>,
        VariadicHolder<bool, char>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, char, void*, int>, int, void*>,
        VariadicHolder<bool, char>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, int, char, void*>, int, void*>,
        VariadicHolder<bool, char>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, int, void*, char>, int, void*>,
        VariadicHolder<bool, char>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, void*, char, int>, int, void*>,
        VariadicHolder<bool, char>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, void*, int, char>, int, void*>,
        VariadicHolder<bool, char>>);
  static_assert(
    std::is_same_v<
        RemoveTypes<VariadicHolder<bool, void*, int, char>, int, void*, char>,
        VariadicHolder<bool>>);
}

TEST(ConcatTest, Correctness) {
  static_assert(std::is_same_v<Concat<>, VariadicHolder<>>);
  static_assert(std::is_same_v<Concat<VariadicHolder<>>, VariadicHolder<>>);
  static_assert(std::is_same_v<Concat<VariadicHolder<>, VariadicHolder<>>, VariadicHolder<>>);
  static_assert(std::is_same_v<Concat<VariadicHolder<int>, VariadicHolder<>>, VariadicHolder<int>>);
  static_assert(
    std::is_same_v<
        Concat<VariadicHolder<int, float>, VariadicHolder<float, char>>,
        VariadicHolder<int, float, float, char>>);
  static_assert(
    std::is_same_v<
        Concat<VariadicHolder<int, float>, VariadicHolder<>, VariadicHolder<float, char>>,
        VariadicHolder<int, float, float, char>>);
}

TEST(RemoveDuplicatesTest, Correctness) {
  static_assert(std::is_same_v<RemoveDuplicates<VariadicHolder<>>, VariadicHolder<>>);
  static_assert(
    std::is_same_v<
        RemoveDuplicates<VariadicHolder<int, float, char>>,
        VariadicHolder<int, float, char>>);
  static_assert(
    std::is_same_v<
        RemoveDuplicates<VariadicHolder<int, int>>,
        VariadicHolder<int>>);
  static_assert(
    std::is_same_v<
        RemoveDuplicates<VariadicHolder<int, float, int>>,
        VariadicHolder<float, int>>);
  static_assert(
    std::is_same_v<
        RemoveDuplicates<VariadicHolder<float, int, int>>,
        VariadicHolder<float, int>>);
  static_assert(
    std::is_same_v<
        RemoveDuplicates<VariadicHolder<float, int, char, int, char, float>>,
        VariadicHolder<int, char, float>>);
}

TEST(UnionTest, Correctness) {
  static_assert(std::is_same_v<Union<>, VariadicHolder<>>);
  static_assert(std::is_same_v<Union<VariadicHolder<>>, VariadicHolder<>>);
  static_assert(std::is_same_v<Union<VariadicHolder<>, VariadicHolder<>>, VariadicHolder<>>);
  static_assert(std::is_same_v<Union<VariadicHolder<int>, VariadicHolder<>>, VariadicHolder<int>>);
  static_assert(std::is_same_v<Union<VariadicHolder<>, VariadicHolder<int>>, VariadicHolder<int>>);
  static_assert(
    std::is_same_v<
        Union<VariadicHolder<float>, VariadicHolder<int>>,
        VariadicHolder<float, int>>);
  static_assert(
    std::is_same_v<
        Union<VariadicHolder<float>, VariadicHolder<float>>,
        VariadicHolder<float>>);
  static_assert(
    std::is_same_v<
        Union<VariadicHolder<float, int>, VariadicHolder<int, float, char>>,
        VariadicHolder<int, float, char>>);
  static_assert(
    std::is_same_v<
        Union<
          VariadicHolder<float, int>,
          VariadicHolder<>,
          VariadicHolder<int, float, char>,
          VariadicHolder<>,
          VariadicHolder<short, char>,
          VariadicHolder<std::string, float>
        >,
        VariadicHolder<int, short, char, std::string, float>>);
}

TEST(VoEUnionTest, Correctness) {
  static_assert(std::is_same_v<voe::Union<void>, ValueOrError<void>>);
  static_assert(std::is_same_v<voe::Union<void, int, char>, ValueOrError<void, int, char>>);
  static_assert(
    std::is_same_v<
        voe::Union<
          void,
          ValueOrError<void>,
          ValueOrError<void>
        >,
        ValueOrError<void>>);
  static_assert(
    std::is_same_v<
        voe::Union<
          void,
          ValueOrError<char, int>,
          ValueOrError<float, std::string>
        >,
        ValueOrError<void, int, std::string>>);
  static_assert(
    std::is_same_v<
        voe::Union<
          void,
          ValueOrError<char, int, char>,
          ValueOrError<short>,
          ValueOrError<float, std::string, int, short>,
          ValueOrError<float, short>
        >,
        ValueOrError<void, char, std::string, int, short>>);
  static_assert(
    std::is_same_v<
        voe::Union<
          void,
          ValueOrError<char, int, char>,
          int, short,
          ValueOrError<short>,
          ValueOrError<float, std::string, int, short>,
          std::string, int64_t,
          ValueOrError<float, short>
        >,
        ValueOrError<void, char, int, std::string, int64_t, short>>);
}

TEST(SubsetOfTest, Correctness) {
  static_assert(SubsetOf<VariadicHolder<>, VariadicHolder<>>::value);
  static_assert(SubsetOf<VariadicHolder<>, VariadicHolder<int>>::value);
  static_assert(SubsetOf<VariadicHolder<>, VariadicHolder<int, float, std::string>>::value);
  static_assert(SubsetOf<VariadicHolder<int>, VariadicHolder<int>>::value);
  static_assert(SubsetOf<VariadicHolder<int>, VariadicHolder<int, float>>::value);
  static_assert(SubsetOf<VariadicHolder<int>, VariadicHolder<float, int>>::value);
  static_assert(SubsetOf<VariadicHolder<int>, VariadicHolder<float, int, float>>::value);
  static_assert(SubsetOf<VariadicHolder<int, float>, VariadicHolder<float, int, float>>::value);
  static_assert(SubsetOf<VariadicHolder<int, float>, VariadicHolder<char, int, float>>::value);
  static_assert(!SubsetOf<VariadicHolder<int>, VariadicHolder<>>::value);
  static_assert(!SubsetOf<VariadicHolder<int>, VariadicHolder<float>>::value);
  static_assert(!SubsetOf<VariadicHolder<int>, VariadicHolder<float, char>>::value);
  static_assert(!SubsetOf<VariadicHolder<int>, VariadicHolder<std::string, float, char>>::value);
  static_assert(!SubsetOf<VariadicHolder<int>, VariadicHolder<std::string, float, char>>::value);
  static_assert(!SubsetOf<VariadicHolder<int, float, char>, VariadicHolder<>>::value);
  static_assert(!SubsetOf<VariadicHolder<int, float, char>, VariadicHolder<float, int>>::value);
  static_assert(!SubsetOf<VariadicHolder<int, float, char>, VariadicHolder<std::string>>::value);
}

TEST(IndexMappingTest, Correctness) {
  {
    using M = IndexMapping<>::MapTo<>;
    static_assert(0 == sizeof(M::indices));
  }

  {
    using M = IndexMapping<int, float, char, std::string>::MapTo<int, float, char, std::string>;
    static_assert(0 == M::indices[0]);
    static_assert(1 == M::indices[1]);
    static_assert(2 == M::indices[2]);
    static_assert(3 == M::indices[3]);
  }

  {
    using M = IndexMapping<int, float, char, std::string>::MapTo<float, char, std::string, int>;
    static_assert(3 == M::indices[0]);
    static_assert(0 == M::indices[1]);
    static_assert(1 == M::indices[2]);
    static_assert(2 == M::indices[3]);
  }

  {
    using M = IndexMapping<int>::MapTo<ValueTypeWrapper<int>, int>;
    static_assert(1 == M::indices[0]);
  }

  {
    using M = IndexMapping<int, float, short>::MapTo<float, char, int>;
    static_assert(2 == M::indices[0]);
    static_assert(0 == M::indices[1]);
    static_assert(size_t(-1) == M::indices[2]);
  }
}

TEST(MinimalSizedIndexTypeTest, Correctness) {
  static_assert(1 == sizeof(MinimalSizedIndexType<0>));
  static_assert(1 == sizeof(MinimalSizedIndexType<123>));
  static_assert(1 == sizeof(MinimalSizedIndexType<std::numeric_limits<uint8_t>::max()>));
  static_assert(2 == sizeof(MinimalSizedIndexType<1ull << 8>));
  static_assert(2 == sizeof(MinimalSizedIndexType<std::numeric_limits<uint16_t>::max()>));
  static_assert(4 == sizeof(MinimalSizedIndexType<1ull << 16>));
  static_assert(4 == sizeof(MinimalSizedIndexType<std::numeric_limits<uint32_t>::max()>));
  static_assert(8 == sizeof(MinimalSizedIndexType<1ull << 32>));
  static_assert(8 == sizeof(MinimalSizedIndexType<std::numeric_limits<uint64_t>::max()>));
}

TEST(ConvertibleTest, Correctness) {
  static_assert(Convertible<VariadicHolder<void>, VariadicHolder<void>>);
  static_assert(Convertible<VariadicHolder<int>, VariadicHolder<void>>);
  static_assert(Convertible<VariadicHolder<void>, VariadicHolder<int>>);
  static_assert(Convertible<VariadicHolder<int>, VariadicHolder<int>>);
  static_assert(!Convertible<VariadicHolder<int>, VariadicHolder<float>>);
  static_assert(Convertible<VariadicHolder<int, int>, VariadicHolder<int, int, short>>);
  static_assert(Convertible<VariadicHolder<int, int>, VariadicHolder<void, int, short>>);
  static_assert(Convertible<VariadicHolder<int, int>, VariadicHolder<void, short, float, int>>);
  static_assert(!Convertible<VariadicHolder<int, int>, VariadicHolder<float, int, short>>);
  static_assert(Convertible<VariadicHolder<int, char>, VariadicHolder<int, short, char>>);
  static_assert(Convertible<VariadicHolder<void, const char*>, VariadicHolder<int, const char*>>);
  static_assert(!Convertible<VariadicHolder<int, char>, VariadicHolder<int, short>>);
  static_assert(!Convertible<VariadicHolder<void, char>, VariadicHolder<int, short>>);
  static_assert(
      Convertible<VariadicHolder<void, char, int>, VariadicHolder<int, short, char, int>>);
}

TEST(VariantStorageTest, Alignment) {
  static_assert(alignof(ValueOrError<char>) == alignof(char));
  static_assert(alignof(ValueOrError<char, short>) == alignof(short));
  static_assert(alignof(ValueOrError<short, char>) == alignof(short));
  static_assert(alignof(ValueOrError<short, char, void*>) == alignof(void*));
  static_assert(alignof(ValueOrError<void*, short, char>) == alignof(void*));
  static_assert(alignof(ValueOrError<void, char>) == alignof(char));
  static_assert(alignof(ValueOrError<void, char, short>) == alignof(short));
  static_assert(alignof(ValueOrError<void, short, char>) == alignof(short));
  static_assert(alignof(ValueOrError<void, short, char, void*>) == alignof(void*));
  static_assert(alignof(ValueOrError<void, void*, short, char>) == alignof(void*));
}

TEST(VariantStorageTest, Size) {
  struct Chars { char data[7]; };
  static_assert(sizeof(ValueOrError<int, char*>) == 16);
  static_assert(sizeof(ValueOrError<int, bool, char, short>) == 8);
  static_assert(sizeof(ValueOrError<int, Chars>) == 8);
  static_assert(sizeof(ValueOrError<int, char*>) == 16);
  static_assert(sizeof(ValueOrError<int, bool, char, short>) == 8);
}

TEST(ValueOrError, TriviallyDestructible) {
  static_assert(std::is_trivially_destructible_v<ValueOrError<void>>);
  static_assert(std::is_trivially_destructible_v<ValueOrError<int>>);
  static_assert(std::is_trivially_destructible_v<ValueOrError<int, float, char>>);
  static_assert(!std::is_trivially_destructible_v<ValueOrError<int, std::string>>);
  static_assert(!std::is_trivially_destructible_v<ValueOrError<std::vector<int>, float>>);
  static_assert(std::is_trivially_destructible_v<ValueOrError<int, char>>);
  static_assert(!std::is_trivially_destructible_v<ValueOrError<int, std::unique_ptr<int>>>);
}

TEST(ValueConstructorTest, Nothrow) {
  static_assert(std::is_nothrow_constructible_v<ValueOrError<int>, int>);
  static_assert(!std::is_nothrow_constructible_v<ValueOrError<std::string>, std::string>);
}

ValueOrError<int, const char*> ReturnValue() {
  return 42;
}

ValueOrError<int, const char*> ReturnError() {
  return MakeError<const char*>("an error");
}

TEST(GetValueTest, RefQualifiers) {
  {
    ValueOrError<int> verr(42);
    static_assert(std::is_same_v<decltype(std::move(verr).GetValue()), int&&>);
    static_assert(std::is_same_v<decltype(verr.GetValue()), int&>);
    static_assert(std::is_same_v<decltype(ReturnValue().GetValue()), int&&>);
    static_assert(std::is_same_v<
        decltype(static_cast<const ValueOrError<int>&>(verr).GetValue()), const int&>);
  }
  {
    ValueOrError<bool, int> verr = MakeError<int>(42);
    static_assert(std::is_same_v<decltype(std::move(verr).GetError<int>()), int&&>);
    static_assert(std::is_same_v<decltype(verr.GetError<int>()), int&>);
    static_assert(std::is_same_v<
        decltype(static_cast<const ValueOrError<bool, int>&>(verr).GetError<int>()), const int&>);
    static_assert(std::is_same_v<
        decltype(ReturnError().GetError<const char*>()), const char*&&>);
  }
}

}  // namespace voe::detail_

