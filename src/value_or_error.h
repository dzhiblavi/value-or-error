#ifndef VOE_HEADER
#define VOE_HEADER

#include <algorithm>
#include <concepts>
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <limits>

#include <iostream>

namespace voe {

template <typename ValueType, typename... ErrorTypes>
class [[nodiscard]] ValueOrError;

namespace detail_ {

template <typename From, typename To>
struct PropagateConstHolder { using type = std::remove_const_t<To>; };

template <typename From, typename To>
struct PropagateConstHolder<const From, To> { using type = const To; };

template <typename From, typename To>
struct PropagateConstHolder<const From&, To> { using type = const To; };

template <typename From, typename To>
struct PropagateConstHolder<const From&&, To> { using type = const To; };

template <typename From, typename To>
using PropagateConst = typename PropagateConstHolder<From, To>::type;

template <typename... Errors>
static constexpr bool AllDecayed = (... && std::is_same_v<Errors, std::decay_t<Errors>>);

template <typename Type, typename... Types>
static constexpr bool TypesContain = (... || std::is_same_v<Type, Types>);

template <size_t Index, typename... Types>
struct IndexToTypeHolder;

template <size_t Index, typename... Types>
using IndexToType = typename IndexToTypeHolder<Index, Types...>::type;

template <size_t Index, typename Type, typename... Types>
struct IndexToTypeHolder<Index, Type, Types...> { using type = IndexToType<Index - 1, Types...>; };

template <typename Type, typename... Types>
struct IndexToTypeHolder<0, Type, Types...> { using type = Type; };

template <typename Type, typename... Types>
struct TypeToIndexHolder;

template <typename Type, typename... Types>
static constexpr size_t TypeToIndex =
  TypesContain<Type, Types...> ? TypeToIndexHolder<Type, Types...>::value : size_t(-1);

template <typename Type, typename Skip, typename... Types>
struct TypeToIndexHolder<Type, Skip, Types...> {
  static constexpr size_t value{1ull + TypeToIndex<Type, Types...>};
};

template <typename Type, typename... Types>
struct TypeToIndexHolder<Type, Type, Types...> { static constexpr size_t value{0}; };

template <typename Type>
struct TypeToIndexHolder<Type> { static constexpr size_t value{size_t(-1)}; };

template <typename... Types>
struct AllUniqueHolder;

template <typename... Types>
static constexpr bool AllUnique = AllUniqueHolder<Types...>::value;

template <>
struct AllUniqueHolder<> { static constexpr bool value = true; };

template <typename Type, typename... Types>
struct AllUniqueHolder<Type, Types...> {
  static constexpr bool value = !TypesContain<Type, Types...> && AllUnique<Types...>;
};

template <
  typename FromTemplate,
  template <typename...> class ToTemplate,
  typename... HeadArgs>
struct TransferTemplateHolder;

template <
  template <typename...> class FromTemplate,
  template <typename...> class ToTemplate,
  typename... Args, typename... HeadArgs>
struct TransferTemplateHolder<FromTemplate<Args...>, ToTemplate, HeadArgs...> {
  using type = ToTemplate<HeadArgs..., Args...>;
};

template <typename FromTemplate, template <typename...> class ToTemplate, typename... HeadArgs>
using TransferTemplate =
  typename TransferTemplateHolder<FromTemplate, ToTemplate, HeadArgs...>::type;

template <typename Callable, typename... Types>
struct VisitInvokeResultHolder;

template <typename Callable, typename T, typename... Types>
struct VisitInvokeResultHolder<Callable, T, Types...> {
  using type = std::invoke_result_t<Callable, T>;
};

template <typename Callable>
struct VisitInvokeResultHolder<Callable> {
  using type = std::invoke_result_t<Callable>;
};

template <typename Callable, typename... Types>
using VisitInvokeResult = typename VisitInvokeResultHolder<Callable, Types...>::type;

template <typename... Types>
struct VariadicHolder;

template <typename... Ts>
struct ConcatHolder;

template <typename... Ts>
using Concat = typename ConcatHolder<Ts...>::type;

template <>
struct ConcatHolder<> { using type = VariadicHolder<>; };

template <typename... Ts1, typename... Ts2>
struct ConcatHolder<VariadicHolder<Ts1...>, VariadicHolder<Ts2...>> {
  using type = VariadicHolder<Ts1..., Ts2...>;
};

template <typename... Types, typename... Ts>
struct ConcatHolder<VariadicHolder<Types...>, Ts...> {
  using type = Concat<VariadicHolder<Types...>, Concat<Ts...>>;
};

template <typename Ts>
struct RemoveDuplicatesHolder;

template <typename... Ts>
using RemoveDuplicates = typename RemoveDuplicatesHolder<Ts...>::type;

template <>
struct RemoveDuplicatesHolder<VariadicHolder<>> { using type = VariadicHolder<>; };

template <typename T, typename... Ts>
struct RemoveDuplicatesHolder<VariadicHolder<T, Ts...>> {
  using Rest = RemoveDuplicates<VariadicHolder<Ts...>>;
  using type = std::conditional_t<TypesContain<T, Ts...>, Rest, Concat<VariadicHolder<T>, Rest>>;
};

template <typename... Ts>
using Union = RemoveDuplicates<Concat<Ts...>>;

template <typename T>
struct ErrorTypesHolder { using type = VariadicHolder<T>; };

template <typename ValueType, typename... ErrorTypes>
struct ErrorTypesHolder<ValueOrError<ValueType, ErrorTypes...>> {
  using type = VariadicHolder<ErrorTypes...>;
};

template <typename T>
using ErrorTypes = typename ErrorTypesHolder<T>::type;

template <typename SourceHolder, typename Accumulator, typename... Remove>
struct RemoveTypesImpl;

template <typename Variadic, typename... Remove>
using RemoveTypes = typename RemoveTypesImpl<Variadic, VariadicHolder<>, Remove...>::type;

template <typename SourceType, typename... SourceTypes, typename... AccTypes, typename... Remove>
struct RemoveTypesImpl<
  VariadicHolder<SourceType, SourceTypes...>,
  VariadicHolder<AccTypes...>, Remove...>
{
  using type = std::conditional_t<
    TypesContain<SourceType, Remove...>,
    typename RemoveTypesImpl<
      VariadicHolder<SourceTypes...>,
      VariadicHolder<AccTypes...>, Remove...>::type,
    typename RemoveTypesImpl<
      VariadicHolder<SourceTypes...>,
      VariadicHolder<AccTypes..., SourceType>, Remove...>::type
  >;
};

template <typename... AccTypes, typename... Remove>
struct RemoveTypesImpl<
  VariadicHolder<>,
  VariadicHolder<AccTypes...>, Remove...>
{
  using type = VariadicHolder<AccTypes...>;
};

template <typename HolderA, typename HolderB>
struct SubsetOf;

template <typename... TypesA, typename... TypesB>
struct SubsetOf<VariadicHolder<TypesA...>, VariadicHolder<TypesB...>> {
  static constexpr bool value = (... && TypesContain<TypesA, TypesB...>);
};

template <typename... TypesFrom>
struct IndexMapping {
  template <typename... TypesTo>
  struct MapTo {
    static constexpr size_t indices[sizeof...(TypesFrom)] ={
      TypeToIndex<TypesFrom, TypesTo...>...
    };
  };

  template <typename... TypesTo>
  struct MapTo<VariadicHolder<TypesTo...>> : public MapTo<TypesTo...> {};
};

template <typename... TypesFrom>
struct IndexMapping<VariadicHolder<TypesFrom...>> : public IndexMapping<TypesFrom...> {};

template <uint64_t NumVariants, std::integral... Types>
struct MinimalSizedIndexTypeHolder;

template <uint64_t NumVariants, std::integral Type, std::integral... Types>
  requires (NumVariants <= static_cast<uint64_t>(std::numeric_limits<Type>::max()))
struct MinimalSizedIndexTypeHolder<NumVariants, Type, Types...> {
  using type = Type;
};

template <uint64_t NumVariants, std::integral Type, std::integral... Types>
  requires (NumVariants > static_cast<uint64_t>(std::numeric_limits<Type>::max()))
struct MinimalSizedIndexTypeHolder<NumVariants, Type, Types...> {
  using type = typename MinimalSizedIndexTypeHolder<NumVariants, Types...>::type;
};

template <size_t NumVariants>
using MinimalSizedIndexType =
  typename MinimalSizedIndexTypeHolder<NumVariants, uint8_t, uint16_t, uint32_t, uint64_t>::type;

template <typename FromVariadic, typename ToVariadic>
struct ConvertibleHolder : public std::false_type {};

template <
  typename FromValueType, typename... FromErrorTypes,
  typename ValueType, typename... ErrorTypes>
struct ConvertibleHolder<
  VariadicHolder<FromValueType, FromErrorTypes...>,
  VariadicHolder<ValueType, ErrorTypes...>>
{
  static constexpr bool value = (
      std::is_same_v<void, FromValueType> ||
      std::is_same_v<ValueType, void> ||
      std::is_same_v<ValueType, FromValueType>
    ) && (
      SubsetOf<
        VariadicHolder<FromErrorTypes...>,
        VariadicHolder<ErrorTypes...>>::value
    );
};

template <typename FromVariadic, typename ToVariadic>
static constexpr bool Convertible = ConvertibleHolder<FromVariadic, ToVariadic>::value;

template <bool IsTriviallyDestructible, typename Type>
struct DestructorFunctor {
  static constexpr void Call(void* ptr) noexcept { static_cast<Type*>(ptr)->~Type(); }
};

template <typename Type>
struct DestructorFunctor<true, Type> {
  static constexpr void Call(void*) noexcept {}
};

template <typename... Types>
struct DestructorFunctorArray {
  using DestructorFunction = void (*)(void*) noexcept;

  static constexpr DestructorFunction array[sizeof...(Types)] = {
    DestructorFunctor<std::is_trivially_destructible_v<Types>, Types>::Call...
  };

  static constexpr void Call(void* ptr, size_t index) noexcept {
    array[index](ptr);
  }
};

template <typename Callable, typename Type>
struct CallableFunctor {
  using PointerType = PropagateConst<Type, void>;

  static constexpr decltype(auto) Call(Callable callable, PointerType* ptr) {
    return std::forward<Callable>(callable)(*static_cast<Type*>(ptr));
  }
};

template <typename FromVoid, typename Callable, typename... Types>
struct CallableFunctorArray {
  using ResultType = VisitInvokeResult<Callable, Types...>;
  using CallFunction = ResultType (*)(Callable, void*);

  static constexpr CallFunction array[sizeof...(Types)] = {
    CallableFunctor<Callable, PropagateConst<FromVoid, Types>>::Call...
  };

  template <typename F>
  static constexpr void Call(F&& func, FromVoid* ptr, size_t index) {
    array[index](std::forward<F>(func), ptr);
  }
};

template <typename Type>
struct CopyConstructorFunctor {
  using FromType = PropagateConst<Type, void>;

  static constexpr void Call(FromType* from, void* to)
    noexcept(std::is_nothrow_copy_constructible_v<Type>)
  {
    if constexpr (!std::is_same_v<void, std::decay_t<Type>>) {
      new (to) std::remove_cv_t<Type>(*static_cast<Type*>(from));
    }
  }
};

template <typename FromVoid, typename... Types>
  requires std::is_same_v<void, std::remove_cv_t<FromVoid>>
struct CopyConstructorFunctorArray {
  using CopyConstructorFunction = void (*)(FromVoid*, void*);

  static constexpr CopyConstructorFunction array[sizeof...(Types)] = {
    CopyConstructorFunctor<PropagateConst<FromVoid, Types>>::Call...
  };

  static constexpr void Call(FromVoid* from, void* to, size_t index) {
    array[index](from, to);
  }
};

template <typename Type>
struct CopyAssignmentFunctor {
  using FromType = PropagateConst<Type, void>;

  static constexpr void Call(FromType* from, void* to)
    noexcept(std::is_nothrow_copy_assignable_v<Type>)
  {
    if constexpr (!std::is_same_v<void, std::decay_t<Type>>) {
      *static_cast<std::remove_cv_t<Type>*>(to) = *static_cast<Type*>(from);
    }
  }
};

template <typename FromVoid, typename... Types>
  requires std::is_same_v<void, std::remove_cv_t<FromVoid>>
struct CopyAssignmentFunctorArray {
  using CopyAssignmentFunction = void (*)(FromVoid*, void*);

  static constexpr CopyAssignmentFunction array[sizeof...(Types)] = {
    CopyAssignmentFunctor<PropagateConst<FromVoid, Types>>::Call...
  };

  static constexpr void Call(FromVoid* from, void* to, size_t index) {
    array[index](from, to);
  }
};

template <typename Type>
struct MoveConstructorFunctor {
  using FromType = PropagateConst<Type, void>;

  static constexpr void Call(FromType* from, void* to)
    noexcept(std::is_nothrow_move_constructible_v<Type>)
  {
    if constexpr (!std::is_same_v<void, std::decay_t<Type>>) {
      new (to) std::remove_cv_t<Type>(std::move(*static_cast<Type*>(from)));
    }
  }
};

template <typename FromVoid, typename... Types>
  requires std::is_same_v<void, std::remove_cv_t<FromVoid>>
struct MoveConstructorFunctorArray {
  using MoveConstructorFunction = void (*)(FromVoid*, void*);

  static constexpr MoveConstructorFunction array[sizeof...(Types)] = {
    MoveConstructorFunctor<PropagateConst<FromVoid, Types>>::Call...
  };

  static constexpr void Call(FromVoid* from, void* to, size_t index) {
    array[index](from, to);
  }
};

template <typename Type>
struct MoveAssignmentFunctor {
  using FromType = PropagateConst<Type, void>;

  static constexpr void Call(FromType* from, void* to)
    noexcept(std::is_nothrow_move_assignable_v<Type>)
  {
    if constexpr (!std::is_same_v<void, std::decay_t<Type>>) {
      *static_cast<std::remove_cv_t<Type>*>(to) = std::move(*static_cast<Type*>(from));
    }
  }
};

template <typename FromVoid, typename... Types>
  requires std::is_same_v<void, std::remove_cv_t<FromVoid>>
struct MoveAssignmentFunctorArray {
  using MoveAssignmentFunction = void (*)(FromVoid*, void*);

  static constexpr MoveAssignmentFunction array[sizeof...(Types)] = {
    MoveAssignmentFunctor<PropagateConst<FromVoid, Types>>::Call...
  };

  static constexpr void Call(FromVoid* from, void* to, size_t index) {
    array[index](from, to);
  }
};

template <
  typename Ref,
  template <class...> class OnLvalueReference,
  template <class...> class OnRvalueReference,
  typename... Args
> struct ReferenceSelectorHolder;

template <
  typename T,
  template <class...> class OnLvalueReference,
  template <class...> class OnRvalueReference,
  typename... Args
> struct ReferenceSelectorHolder<T&, OnLvalueReference, OnRvalueReference, Args...> {
  using type = OnLvalueReference<Args...>;
};

template <
  typename T,
  template <class...> class OnLvalueReference,
  template <class...> class OnRvalueReference,
  typename... Args
> struct ReferenceSelectorHolder<T&&, OnLvalueReference, OnRvalueReference, Args...> {
  using type = OnRvalueReference<Args...>;
};

template <
  typename Ref,
  template <class...> class OnLvalueReference,
  template <class...> class OnRvalueReference,
  typename... Args
> using ReferenceSelector =
  typename ReferenceSelectorHolder<Ref, OnLvalueReference, OnRvalueReference, Args...>::type;

template <typename... Types>
struct VariantStorage {
  static constexpr size_t StorageSize = std::max({0ul, sizeof(Types)...});

  alignas(Types...) std::byte data[StorageSize];
  MinimalSizedIndexType<1 + sizeof...(Types)> index{0};
};

template <typename Type>
struct ValueTypeWrapper {};

template <typename... Stored>
struct TraitsBase : public VariantStorage<Stored...> {
 public:
  using StorageType = VariantStorage<Stored...>;
  using DestructorArray = DestructorFunctorArray<Stored...>;

  template <typename FromVoid>
  using CopyConstructorArray = CopyConstructorFunctorArray<FromVoid, Stored...>;
  template <typename FromVoid>
  using MoveConstructorArray = MoveConstructorFunctorArray<FromVoid, Stored...>;
  template <typename FromVoid>
  using CopyAssignmentArray = CopyAssignmentFunctorArray<FromVoid, Stored...>;
  template <typename FromVoid>
  using MoveAssignmentArray = MoveAssignmentFunctorArray<FromVoid, Stored...>;
  template <typename Callable, typename FromVoid>
  using VisitArray = CallableFunctorArray<FromVoid, Callable, Stored...>;

  template <typename Ref, typename FromVoid>
  using ConstructorRefSelector =
    ReferenceSelector<Ref, CopyConstructorArray, MoveConstructorArray, FromVoid>;
  template <typename Ref, typename FromVoid>
  using AssignmentRefSelector =
    ReferenceSelector<Ref, CopyAssignmentArray, MoveAssignmentArray, FromVoid>;

  const auto& LogicalIndex() const noexcept { return StorageType::index; }
  auto& LogicalIndex() noexcept { return StorageType::index; }
  auto PhysicalIndex() const noexcept { return LogicalToPhysicalIndex(LogicalIndex()); }

  const auto& Data() const noexcept { return StorageType::data; }
  auto& Data() noexcept { return StorageType::data; }

  static constexpr size_t LogicalEmptyIndex() noexcept { return 0; }
  static constexpr size_t LogicalToPhysicalIndex(size_t index) noexcept { return index - 1; }
  static constexpr size_t PhysicalToLogicalIndex(size_t index) noexcept { return index + 1; }

  /**
   * @return whether this object neither holds a value nor an error (is empty)
   */
  constexpr bool IsEmpty() const noexcept {
    return LogicalIndex() == LogicalEmptyIndex();
  }

 private:
  using StorageType::index;
  using StorageType::data;
};

template <typename... Types>
struct Traits;

template <typename ValueType, typename... ErrorTypes>
struct Traits<ValueType, ErrorTypes...> : public TraitsBase<ValueType, ErrorTypes...> {
  using Base = TraitsBase<ValueType, ErrorTypes...>;

  using StoredTypes = VariadicHolder<ValueTypeWrapper<ValueType>, ErrorTypes...>;
  using StoredErrorTypes = VariadicHolder<ErrorTypes...>;

  static constexpr size_t LogicalValueIndex() noexcept { return 1; }
  static constexpr size_t LogicalFirstErrorIndex() noexcept { return 2; }

  template <typename ErrorType> requires TypesContain<ErrorType, ErrorTypes...>
  static constexpr size_t LogicalErrorIndex() noexcept {
    return LogicalFirstErrorIndex() + TypeToIndex<ErrorType, ErrorTypes...>;
  }
};

template <typename... ErrorTypes>
struct Traits<void, ErrorTypes...> : public TraitsBase<ErrorTypes...> {
  using Base = TraitsBase<ErrorTypes...>;

  using StoredTypes = VariadicHolder<ErrorTypes...>;
  using StoredErrorTypes = VariadicHolder<ErrorTypes...>;

  static constexpr size_t LogicalFirstErrorIndex() noexcept { return 1; }

  template <typename ErrorType> requires TypesContain<ErrorType, ErrorTypes...>
  static constexpr size_t LogicalErrorIndex() noexcept {
    return LogicalFirstErrorIndex() + TypeToIndex<ErrorType, ErrorTypes...>;
  }
};

template <bool AllTriviallyDestructible, typename... Types>
struct DestructorHolder : public Traits<Types...> {
  using Base = Traits<Types...>;

  ~DestructorHolder() noexcept {
    DestroyImpl();
  }

  /**
   * @brief Clears the object. The state after method is applied is Empty.
   */
  void Clear() noexcept {
    DestroyImpl();
    Base::LogicalIndex() = Base::LogicalEmptyIndex();
  }

 private:
  void DestroyImpl() noexcept {
    if (Base::IsEmpty()) {
      return;
    }
    Base::DestructorArray::Call(Base::Data(), Base::PhysicalIndex());
  }
};

template <typename... Types>
struct DestructorHolder<true, Types...> : public Traits<Types...> {
  using Base = Traits<Types...>;

  ~DestructorHolder() noexcept = default;

  /**
   * @brief Clears the object. The state after method is applied is Empty.
   */
  void Clear() noexcept {
    Base::LogicalIndex() = Base::LogicalEmptyIndex();
  }
};

template <typename... Types>
using DestructorImpl = DestructorHolder<
  (... && (std::is_trivially_destructible_v<Types> || std::is_same_v<void, Types>)), Types...>;

template <typename ValueType, typename... ErrorTypes>
struct GetValueImpl : public DestructorImpl<ValueType, ErrorTypes...> {
  using Base = DestructorImpl<ValueType, ErrorTypes...>;

  /**
   * @return whether the object holds a value.
   */
  constexpr bool HasValue() const noexcept {
    return Base::LogicalIndex() == Base::LogicalValueIndex();
  }

  /**
   * @return a reference to underlying value
   * @exception UB is HasValue() == false
   */
  ValueType& GetValue() & noexcept {
    assert(HasValue() && "GetValue() called on object with no value");
    return reinterpret_cast<ValueType&>(Base::Data());
  }

  /**
   * @return moved underlying value
   * @exception UB is HasValue() == false
   */
  ValueType&& GetValue() && noexcept {
    assert(HasValue() && "GetValue() called on object with no value");
    return reinterpret_cast<ValueType&&>(Base::Data());
  }

  /**
   * @return a const reference to underlying value
   * @exception UB is HasValue() == false
   */
  const ValueType& GetValue() const& noexcept {
    assert(HasValue() && "GetValue() called on object with no value");
    return reinterpret_cast<const ValueType&>(Base::Data());
  }
};

template <typename... ErrorTypes>
struct GetValueImpl<void, ErrorTypes...> : public DestructorImpl<void, ErrorTypes...> {
  using Base = DestructorImpl<void, ErrorTypes...>;

  /**
   * @return false
   * @note ValueOrError<void, ...> never holds a value
   */
  constexpr bool HasValue() const noexcept { return false; }
};

template <typename ValueType, typename... ErrorTypes>
struct GetErrorImpl : public GetValueImpl<ValueType, ErrorTypes...> {
  using Base = GetValueImpl<ValueType, ErrorTypes...>;

  /**
   * @brief Get Error type by its index in ErrorTypes... list
   */
  template <size_t Index>
  using ErrorType = IndexToType<Index, ErrorTypes...>;

  /**
   * @return whether the object holds any error
   */
  constexpr bool HasAnyError() const noexcept {
    return Base::LogicalIndex() >= Base::LogicalFirstErrorIndex();
  }

  /**
   * @return the index of underlying error in ErrorTypes... list
   * @exception UB if !HasAnyError()
   */
  size_t GetErrorIndex() const noexcept {
    assert(HasAnyError() && "GetErrorIndex() called on object with no error");
    return Base::LogicalIndex() - Base::LogicalFirstErrorIndex();
  }

  /**
   * @return whether this object holds an error with the specified type
   */
  template <typename ErrorType>
    requires detail_::TypesContain<ErrorType, ErrorTypes...>
  bool HasError() const noexcept {
    return Base::LogicalIndex() == Base::template LogicalErrorIndex<ErrorType>();
  }

  /**
   * @return reference to underlying error of the specified type
   * @exception UB if !HasError<ErrorType>()
   */
  template <typename ErrorType>
    requires detail_::TypesContain<ErrorType, ErrorTypes...>
  ErrorType& GetError() & noexcept {
    assert(HasError<ErrorType>() && "GetError<E>() called on object with no error E");
    return reinterpret_cast<ErrorType&>(Base::Data());
  }

  /**
   * @return rvalue reference to the underlying error object of the specified type
   * @exception UB if !HasError<ErrorType>()
   */
  template <typename ErrorType>
    requires detail_::TypesContain<ErrorType, ErrorTypes...>
  ErrorType&& GetError() && noexcept {
    assert(HasError<ErrorType>() && "GetError<E>() called on object with no error E");
    return reinterpret_cast<ErrorType&&>(Base::Data());
  }

  /**
   * @return const reference to underlying error object of the specified type
   * @exception UB if !HasError<ErrorType>()
   */
  template <typename ErrorType>
    requires detail_::TypesContain<ErrorType, ErrorTypes...>
  const ErrorType& GetError() const& noexcept {
    assert(HasError<ErrorType>() && "GetError<E>() called on object with no error E");
    return reinterpret_cast<const ErrorType&>(Base::Data());
  }

  /**
   * @return const rvalue reference to underlying error object of the specified type
   * @exception UB if !HasError<ErrorType>()
   */
  template <typename ErrorType>
    requires detail_::TypesContain<ErrorType, ErrorTypes...>
  const ErrorType&& GetError() const&& noexcept {
    assert(HasError<ErrorType>() && "GetError<E>() called on object with no error E");
    return reinterpret_cast<const ErrorType&&>(Base::Data());
  }

  /**
   * @return reference to underlying error with type ErrorType<Index>
   * @exception UB if !HasError<ErrorType<Index>>()
   */
  template <size_t Index>
  ErrorType<Index>& GetError() & noexcept {
    assert(HasError<ErrorType<Index>>() && "GetError<I>() called on object with no error E[I]");
    return reinterpret_cast<ErrorType<Index>&>(Base::Data());
  }

  /**
   * @return rvalue reference to underlying error with type ErrorType<Index>
   * @exception UB if !HasError<ErrorType<Index>>()
   */
  template <size_t Index>
  ErrorType<Index>&& GetError() && noexcept {
    assert(HasError<ErrorType<Index>>() && "GetError<I>() called on object with no error E[I]");
    return reinterpret_cast<ErrorType<Index>&>(Base::Data());
  }

  /**
   * @return const reference to underlying error with type ErrorType<Index>
   * @exception UB if !HasError<ErrorType<Index>>()
   */
  template <size_t Index>
  const ErrorType<Index>& GetError() const& noexcept {
    assert(HasError<ErrorType<Index>>() && "GetError<I>() called on object with no error E[I]");
    return reinterpret_cast<const ErrorType<Index>&>(Base::Data());
  }

  /**
   * @return const rvalue reference to underlying error with type ErrorType<Index>
   * @exception UB if !HasError<ErrorType<Index>>()
   */
  template <size_t Index>
  const ErrorType<Index>&& GetError() const&& noexcept {
    assert(HasError<ErrorType<Index>>() && "GetError<I>() called on object with no error E[I]");
    return reinterpret_cast<const ErrorType<Index>&&>(Base::Data());
  }
};

template <typename ValueType, typename... ErrorTypes>
struct SetErrorImpl : public GetErrorImpl<ValueType, ErrorTypes...> {
  using Base = GetErrorImpl<ValueType, ErrorTypes...>;

  /**
   * @brief Sets the error with the specified value
   */
  template <typename ErrorType>
    requires detail_::TypesContain<ErrorType, ErrorTypes...>
  void SetError(ErrorType&& error) & {
    Base::Clear();
    Base::LogicalIndex() = Base::template LogicalErrorIndex<ErrorType>();
    new (Base::Data()) ErrorType(std::forward<ErrorType>(error));
  }

  /**
   * @brief Sets the error by constructing it inplace via forwarding constructor arguments
   */
  template <typename ErrorType, typename... Args>
  void EmplaceError(Args&&... args) & {
    Base::Clear();
    Base::LogicalIndex() = Base::template LogicalErrorIndex<ErrorType>();
    new (Base::Data()) ErrorType(std::forward<Args>(args)...);
  }
};

template <typename ValueType, typename... ErrorTypes>
struct ValueConstructorImpl : public SetErrorImpl<ValueType, ErrorTypes...> {
  using Base = SetErrorImpl<ValueType, ErrorTypes...>;

  /**
   * @brief Constructs the object as holing a value using the specified value object
   */
  template <typename FromType>
    requires std::same_as<ValueType, std::decay_t<FromType>>
  void ValueConstruct(FromType&& from)
    noexcept(std::is_nothrow_copy_constructible_v<ValueType>)
  {
    Base::LogicalIndex() = Base::LogicalValueIndex();
    new (Base::Data()) ValueType(std::forward<FromType>(from));
  }
};

template <typename ValueType, typename... ErrorTypes>
struct ConstructorsImpl : public ValueConstructorImpl<ValueType, ErrorTypes...> {
  using Base = ValueConstructorImpl<ValueType, ErrorTypes...>;

 protected:
  template <typename From>
  void Construct(From&& from) {
    if (from.IsEmpty()) {
      return;
    }
    Base::LogicalIndex() = from.LogicalIndex();
    Base
      ::template ConstructorRefSelector<decltype(from), detail_::PropagateConst<From, void>>
      ::Call(from.Data(), Base::Data(), Base::PhysicalIndex());
  }

  template <typename From, typename FromValueType, typename... FromErrorTypes>
  void ConvertConstruct(
      From&& from,
      ConstructorsImpl<FromValueType, FromErrorTypes...>*)
  {
    if (from.IsEmpty()) {
      return;
    }
    using FromType = ConstructorsImpl<FromValueType, FromErrorTypes...>;
    using PhysicalIndexMapping =
      typename IndexMapping<typename FromType::StoredTypes>
      ::template MapTo<typename Base::StoredTypes>;

    const size_t this_phys_index = PhysicalIndexMapping::indices[from.PhysicalIndex()];
    assert(
        this_phys_index != size_t(-1) &&
        "Conversion constructor from ValueOrError<X, ...> to ValueOrError<void, ...>"
        " is trying to drop a value");

    FromType
      ::template ConstructorRefSelector<decltype(from), detail_::PropagateConst<From, void>>
      ::Call(from.Data(), Base::Data(), from.PhysicalIndex());
    Base::LogicalIndex() = Base::PhysicalToLogicalIndex(this_phys_index);
  }
};

template <typename ValueType, typename... ErrorTypes>
struct AssignmentsImpl : public ConstructorsImpl<ValueType, ErrorTypes...> {
  using Base = ConstructorsImpl<ValueType, ErrorTypes...>;

  template <typename Assignee>
    requires std::is_same_v<std::decay_t<Assignee>, ValueOrError<ValueType, ErrorTypes...>>
  void Assign(Assignee&& rhs) & noexcept {
    if (this == &rhs) {
      return;
    }
    if (rhs.IsEmpty()) {
      Base::Clear();
      return;
    }
    if (Base::LogicalIndex() == rhs.LogicalIndex()) {
      Base
        ::template AssignmentRefSelector<decltype(rhs), detail_::PropagateConst<Assignee, void>>
        ::Call(rhs.Data(), Base::Data(), Base::PhysicalIndex());
      return;
    }
    Base::Clear();
    Base::LogicalIndex() = rhs.LogicalIndex();
    Base
      ::template ConstructorRefSelector<decltype(rhs), detail_::PropagateConst<Assignee, void>>
      ::Call(rhs.Data(), Base::Data(), Base::PhysicalIndex());
  }

  template <typename Convert, typename FromValueType, typename... FromErrorTypes>
  void ConvertAssign(
      Convert&& rhs,
      ValueOrError<FromValueType, FromErrorTypes...>*) & noexcept
  {
    if (rhs.IsEmpty()) {
      Base::Clear();
      return;
    }
    using RhsType = ValueOrError<FromValueType, FromErrorTypes...>;
    using PhysicalIndexMapping =
        typename IndexMapping<typename RhsType::StoredTypes>
        ::template MapTo<typename Base::StoredTypes>;

    const size_t rhs_phys_index = rhs.PhysicalIndex();
    const size_t this_phys_index = PhysicalIndexMapping::indices[rhs_phys_index];
    assert(
        this_phys_index != size_t(-1) &&
        "Conversion assignment of ValueOrError<X, ...> to ValueOrError<void, ...>"
        " is trying to drop a value");

    if (Base::PhysicalIndex() == this_phys_index) {
      RhsType
        ::template AssignmentRefSelector<decltype(rhs), detail_::PropagateConst<Convert, void>>
        ::Call(rhs.Data(), Base::Data(), rhs_phys_index);
      return;
    }

    Base::Clear();
    Base::LogicalIndex() = Base::PhysicalToLogicalIndex(this_phys_index);
    RhsType
      ::template ConstructorRefSelector<decltype(rhs), detail_::PropagateConst<Convert, void>>
      ::Call(rhs.Data(), Base::Data(), rhs_phys_index);
  }
};

template <typename ValueType, typename... ErrorTypes>
struct DiscardErrorImpl : public AssignmentsImpl<ValueType, ErrorTypes...> {
  using Base = AssignmentsImpl<ValueType, ErrorTypes...>;

  template <typename... DiscardedErrors>
  using ResultType = TransferTemplate<
    RemoveTypes<VariadicHolder<ErrorTypes...>, DiscardedErrors...>,
    ValueOrError, ValueType>;

  /**
   * @brief Transforms the type by removing the specified error types
   *
   * In all scenarios, this object will become empty as the value or error will be moved out.
   * - If the object was empty, the result is empty.
   * - If the object held a value, the result holds the moved value.
   * - If the object held an error, the result holds it in case the resulting type has this error.
   * - If the object held an error which type is being removed, the result is empty.
   */
  template <typename... DiscardedErrors>
  ResultType<DiscardedErrors...> DiscardErrors() {
    using ResultType = ResultType<DiscardedErrors...>;
    ResultType result;

    if (Base::IsEmpty()) {
      return result;
    }

    using PhysicalIndexMapping =
        typename IndexMapping<typename Base::StoredTypes>
        ::template MapTo<typename ResultType::StoredTypes>;

    const size_t result_phys_index = PhysicalIndexMapping::indices[Base::PhysicalIndex()];
    if (result_phys_index == size_t(-1)) {
      return result;
    }

    Base
      ::template MoveConstructorArray<void>
      ::Call(Base::Data(), result.Data(), Base::PhysicalIndex());
    Base::Clear();

    result.LogicalIndex() = ResultType::PhysicalToLogicalIndex(result_phys_index);
    return result;
  }
};

template <typename ValueType, typename... ErrorTypes>
struct DiscardValueImpl : public DiscardErrorImpl<ValueType, ErrorTypes...> {
  using Base = DiscardErrorImpl<ValueType, ErrorTypes...>;

  /**
   * @brief Discards the value from the type
   * @return an object of type ValueOrError<void, ErrorTypes...> with the same state as this
   * @exception UB: this object holds a value
   */
  ValueOrError<void, ErrorTypes...> DiscardValue() const noexcept {
    assert(!Base::HasValue() && "Discarding ValueType on object holding a value");
    return ValueOrError<void, ErrorTypes...>(*this);
  }
};

template <typename... ErrorTypes>
struct DiscardValueImpl<void, ErrorTypes...> : public DiscardErrorImpl<void, ErrorTypes...> {
  using Base = DiscardErrorImpl<void, ErrorTypes...>;

  /**
   * @brief Discards the value from the type
   * @return an object of type ValueOrError<void, ErrorTypes...> with the same state as this
   * @note actually does nothing for VoidOrError instances
   */
  auto& DiscardValue() & noexcept { return *this; }

  /**
   * @brief Discards the value from the type
   * @return an object of type ValueOrError<void, ErrorTypes...> with the same state as this
   * @note actually does nothing for VoidOrError instances
   */
  const auto& DiscardValue() const& noexcept { return *this; }
};

template <typename ValueType, typename... ErrorTypes>
struct VisitImpl : public DiscardValueImpl<ValueType, ErrorTypes...> {
  using Base = DiscardValueImpl<ValueType, ErrorTypes...>;

  /**
   * @brief Visit paradigm implementation for ValueOrError objects
   *
   * For non-void-value ValueOrError objects, the specified functor F will be
   * called as follows:
   * - F(GetValue()) if the object holds a value.
   * - F(GetError<E>()) if the object holds an error of type E.
   *
   * @exception UB if the object is empty (i.e. neither holds a value nor an error)
   */
  template <typename Visitor>
    requires (
      std::invocable<Visitor, ValueType> &&
      (... && std::invocable<Visitor, ErrorTypes>))
  decltype(auto) Visit(Visitor&& visitor) {
    assert(!Base::IsEmpty() && "Visit() called on an empty object");
    return Base
      ::template VisitArray<Visitor, void>
      ::Call(std::forward<Visitor>(visitor), Base::Data(), Base::PhysicalIndex());
  }

  /**
   * @brief Visit paradigm implementation for ValueOrError objects
   * @see Visit, this is a const version of it
   */
  template <typename Visitor>
    requires (
      std::invocable<Visitor, const ValueType> &&
      (... && std::invocable<Visitor, const ErrorTypes>))
  decltype(auto) Visit(Visitor&& visitor) const {
    assert(!Base::IsEmpty() && "Visit() called on an empty object");
    return Base
      ::template VisitArray<Visitor, const void>
      ::Call(std::forward<Visitor>(visitor), Base::Data(), Base::PhysicalIndex());
  }
};

template <typename... ErrorTypes>
struct VisitImpl<void, ErrorTypes...> : public DiscardValueImpl<void, ErrorTypes...> {
  using Base = DiscardValueImpl<void, ErrorTypes...>;

  /**
   * @brief Visit paradigm implementation for ValueOrError objects
   *
   * For void-value ValueOrError objects, the specified functor F will be called as follows:
   * - F() if the object is empty (holds a void value).
   * - F(GetError<E>()) if the object holds an error of type E.
   */
  template <typename Visitor>
    requires (
      std::invocable<Visitor> &&
      (... && std::invocable<Visitor, ErrorTypes>))
  decltype(auto) Visit(Visitor&& visitor) {
    if (!Base::HasAnyError()) {
      return std::forward<Visitor>(visitor)();
    } else {
      return Base
        ::template VisitArray<Visitor, void>
        ::Call(std::forward<Visitor>(visitor), Base::Data(), Base::PhysicalIndex());
    }
  }

  /**
   * @brief Visit paradigm implementation for ValueOrError objects
   * @see Visit, this is a const version of it
   */
  template <typename Visitor>
    requires (
      std::invocable<Visitor> &&
      (... && std::invocable<Visitor, const ErrorTypes>))
  decltype(auto) Visit(Visitor&& visitor) const {
    if (!Base::HasAnyError()) {
      return std::forward<Visitor>(visitor)();
    } else {
      return Base
        ::template VisitArray<Visitor, const void>
        ::Call(std::forward<Visitor>(visitor), Base::Data(), Base::PhysicalIndex());
    }
  }
};

template <typename ValueType, typename... ErrorTypes>
using ValueOrErrorImpl = VisitImpl<ValueType, ErrorTypes...>;

}  // namespace detail_

/**
 * @brief A class representing either value or some error.
 *
 * The object of this class can be in 2 or 3 states, depending on ValueType template parameter:
 * - Empty;
 * - Has value: iff !std::is_same_v<void, ValueType>;
 * - Has error: iff sizeof...(ErrorTypes) > 0.
 *
 * The object can be created as empty or having value. In order to create an object holding an
 * error, one should use voe::MakeError.
 *
 * @exception None (the object does not produce any exceptions)
 */
template <typename ValueType, typename... ErrorTypes>
class [[nodiscard]] ValueOrError
  : public detail_::ValueOrErrorImpl<ValueType, ErrorTypes...>
{
  using Base = detail_::ValueOrErrorImpl<ValueType, ErrorTypes...>;
  using SelfType = ValueOrError<ValueType, ErrorTypes...>;

 public:
  using value_type = ValueType;
  static_assert(detail_::AllDecayed<ValueType, ErrorTypes...>, "All types must be decayed");
  static_assert(detail_::AllUnique<ErrorTypes...>, "Error types must not contain duplicates");

  /**
   * @brief Constructs an empty ValueOrError
   *
   * An empty ValueOrError neither holds error nor value.
   * Calls to such methods as GetError or GetValue will result in UB.
   * HasAnyError, HasError<*>, HasValue will return false;
   */
  ValueOrError() noexcept = default;

  /**
   * @brief Construct a ValueOrError holding a value
   *
   * The resulting object will store the value. HasValue will return true and
   * GetValue will be legal to use.
   *
   * @param from the value to be constructed from
   * @exception only ones thrown by ValueType's related copy/move constructor
   */
  template <typename FromType>
    requires std::same_as<ValueType, std::decay_t<FromType>>
  /* implicit */ ValueOrError(FromType&& from)
    noexcept(std::is_nothrow_copy_constructible_v<ValueType>)
  { Base::ValueConstruct(std::forward<FromType>(from)); }

  ValueOrError(ValueOrError& voe) { Base::Construct(voe); }
  ValueOrError(const ValueOrError& voe) { Base::Construct(voe); }
  ValueOrError(ValueOrError&& voe) { Base::Construct(std::move(voe)); }
  ValueOrError(const ValueOrError&& voe) { Base::Construct(std::move(voe)); }

  /**
   * @brief ValueOrError conversion constructor
   *
   * The ValueOrError template instances are considered convertible iff:
   * - Their respective ValueType parameters are either same or void (one or both);
   * - The ErrorTypes... of from ValueOrError must be a subset of ErrorTypes... of to one.
   *
   * The rules are as follows:
   * - If from is empty, then the resulting object will be empty;
   * - If from holds a value (thus its ValueType is not void), and this type's ValueType is
   *   also not equal to void, than the value will be copied/moved to the resulting object;
   * - If from holds an error, than this error will be copied/moved to the resulting object.
   *
   * @exception (UB) from holds a value, and this type's ValueType is void
   * @exception Any exception thrown from copy or move constructor of respective type
   */
  template <typename FromVoe>
    requires detail_::Convertible<
        detail_::TransferTemplate<std::decay_t<FromVoe>, detail_::VariadicHolder>,
        detail_::VariadicHolder<ValueType, ErrorTypes...>
    >
  /* implicit */ ValueOrError(FromVoe&& from) {
    Base::ConvertConstruct(
        std::forward<FromVoe>(from), static_cast<std::decay_t<FromVoe>*>(nullptr));
  }

  SelfType& operator=(ValueOrError& arg) & { Base::Assign(arg); return *this; }
  SelfType& operator=(const ValueOrError& arg) & { Base::Assign(arg); return *this; }
  SelfType& operator=(ValueOrError&& arg) & { Base::Assign(std::move(arg)); return *this; }
  SelfType& operator=(const ValueOrError&& arg) & { Base::Assign(std::move(arg)); return *this; }

  /**
   * @brief ValueOrError conversion assignment operator
   *
   * See the conversion constructor operator in order to review the definition
   * of convertible ValueOrError template instances. The resulting object
   * will have the same properties as described in conversion constructor.
   *
   * The difference to conversion constructor is that if this object holds a value or an error,
   * it has to be either destroyed before construction of a new value or assigned to a new object,
   * depending on equality of types of objects that are held by lhs and rhs.
   *
   * @exception (UB) rhs holds a value, and this type's ValueType is void
   * @exception Any exception thrown from copy or move constructor of respective type
   */
  template <typename FromVoe>
    requires detail_::Convertible<
        detail_::TransferTemplate<std::decay_t<FromVoe>, detail_::VariadicHolder>,
        detail_::VariadicHolder<ValueType, ErrorTypes...>
    >
  SelfType& operator=(FromVoe&& rhs) & {
    Base::ConvertAssign(std::forward<FromVoe>(rhs), static_cast<std::decay_t<FromVoe>*>(nullptr));
    return *this;
  }

 protected:
  template <typename, typename...>
  friend class ValueOrError;
};

/**
 * @brief A shorter type template alias for void-returning functions
 */
template <typename... ErrorTypes>
using VoidOrError = ValueOrError<void, ErrorTypes...>;

/**
 * @brief A convernient and explicit way to create error objects
 * @param[in] ErrorType the type of an error
 * @param[in] error instance of #ErrorType&&
 * @return an instance of #ValueOrError<void, ErrorType> holding an error
 */
template <typename ErrorType>
VoidOrError<ErrorType> MakeError(ErrorType&& error) {
  VoidOrError<ErrorType> result;
  result.SetError(std::forward<ErrorType>(error));
  return result;
}

/**
 * @brief A convernient and explicit way to create error objects
 * @param[in] ErrorType the type of an error
 * @param[in] Args types of #ErrorType constructor arguments
 * @param[in] args #ErrorType constructor arguments
 * @return an instance of #ValueOrError<void, ErrorType> holding an error
 */
template <typename ErrorType, typename... Args>
VoidOrError<ErrorType> MakeError(Args&&... args) {
  VoidOrError<ErrorType> result;
  result.template EmplaceError<ErrorType>(std::forward<Args>(args)...);
  return result;
}

/**
 * @brief Combine two ValueOrError types
 *
 * For example, consider the following snippet:
 * @code
 * using A = ValueOrError<char, int, short>;
 * A Foo() { ... }
 *
 * using B = ValueOrError<void, long, int>;
 * B Bar() { ... }
 *
 * using C = Union<float, A, B>;
 * static_assert(std::is_same_v<C, ValueOrError<float, int, short, long>>);
 *
 * C Baz() {
 *   RETURN_IF_ERROR(Foo());
 *   RETURN_IF_ERROR(Bar());
 *   return 10.f;
 * }
 * @endcode
 */
template <typename ValueType, typename... VoEOrErrorTypes>
using Union = detail_::TransferTemplate<
  detail_::Union<detail_::ErrorTypes<VoEOrErrorTypes>...>,
  ValueOrError, ValueType
>;

#define RETURN_IF_ERROR(expr)     \
  do {                            \
    auto&& err = (expr);          \
    if (err.HasAnyError()) {      \
      return err.DiscardValue();  \
    }                             \
  } while (0)                     \

#define ASSIGN_OR_RETURN_ERROR(var, expr)  \
  do {                                     \
    auto&& err = (expr);                   \
    if (err.HasAnyError()) {               \
      return err.DiscardValue();           \
    }                                      \
    assert(!err.IsEmpty());                \
    var = std::move(err.GetValue());       \
  } while (0)

}  // namespace voe

#endif  // VOE_HEADER

