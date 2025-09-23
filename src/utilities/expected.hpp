
#pragma once

#include <cassert>
#include <exception>
#include <functional>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

// ---------------------------------------------
// unexpect_t and unexpect
// ---------------------------------------------
struct unexpect_t {
  explicit constexpr unexpect_t() = default;
};
inline constexpr unexpect_t unexpect{};

// ---------------------------------------------
// unexpected<E>
// ---------------------------------------------
template <class E>
class unexpected {
 public:
  using error_type = E;

  static_assert(!std::is_void_v<E>, "unexpected<void> is ill-formed");

  constexpr explicit unexpected(const E& e) noexcept(
      std::is_nothrow_copy_constructible_v<E>)
      : err_(e) {}

  constexpr explicit unexpected(E&& e) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : err_(std::move(e)) {}

  template <class... Args,
            std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
  constexpr explicit unexpected(std::in_place_t, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<E, Args...>)
      : err_(std::forward<Args>(args)...) {}

  template <class U,
            class... Args,
            std::enable_if_t<
                std::is_constructible_v<E, std::initializer_list<U>&, Args...>,
                int> = 0>
  constexpr explicit unexpected(
      std::in_place_t,
      std::initializer_list<U> il,
      Args&&... args) noexcept(std::
                                   is_nothrow_constructible_v<
                                       E,
                                       std::initializer_list<U>&,
                                       Args...>)
      : err_(il, std::forward<Args>(args)...) {}

  constexpr const E& error() const& noexcept { return err_; }
  constexpr E& error() & noexcept { return err_; }
  constexpr const E&& error() const&& noexcept { return std::move(err_); }
  constexpr E&& error() && noexcept { return std::move(err_); }

  // Comparisons
  friend constexpr bool operator==(const unexpected& a, const unexpected& b) {
    return a.err_ == b.err_;
  }

 private:
  E err_;
};

template <class E>
[[nodiscard]] constexpr unexpected<std::decay_t<E>> make_unexpected(
    E&& e) noexcept(std::is_nothrow_constructible_v<std::decay_t<E>, E&&>) {
  return unexpected<std::decay_t<E>>(std::forward<E>(e));
}

// ---------------------------------------------
// bad_expected_access<E>
// ---------------------------------------------
template <class E = void>
class bad_expected_access : public std::exception {
 public:
  using error_type = E;

  explicit bad_expected_access(const E& e) noexcept(
      std::is_nothrow_copy_constructible_v<E>)
      : err_(e) {}
  explicit bad_expected_access(E&& e) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : err_(std::move(e)) {}

  const char* what() const noexcept override { return "bad expected access"; }
  const E& error() const& noexcept { return err_; }

 private:
  E err_;
};

template <>
class bad_expected_access<void> : public std::exception {
 public:
  const char* what() const noexcept override { return "bad expected access"; }
};

// ---------------------------------------------
// expected<T, E> — primary
// ---------------------------------------------
namespace __exp_detail {

template <class T, class E>
union storage {
  unsigned char dummy_;
  T val_;
  E err_;

  constexpr storage() noexcept : dummy_() {}
  template <class... Args>
  constexpr explicit storage(std::in_place_index_t<0>, Args&&... args)
      : val_(std::forward<Args>(args)...) {}
  template <class... Args>
  constexpr explicit storage(std::in_place_index_t<1>, Args&&... args)
      : err_(std::forward<Args>(args)...) {}

  ~storage() {}  // non-trivial handled by expected
};

}  // namespace __exp_detail

template <class T, class E>
class expected {
  static_assert(!std::is_reference_v<T>,
                "expected<T,E> with reference T is ill-formed");
  static_assert(!std::is_void_v<E>, "expected<T, void> is ill-formed");

 public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  // ------------------ ctors ------------------
  constexpr expected() noexcept(std::is_nothrow_default_constructible_v<T>)
      : has_(true) {
    if constexpr (!std::is_void_v<T>) {
      ::new (std::addressof(sto_.val_)) T();
    }
  }

  // value constructors
  constexpr expected(const T& v) noexcept(
      std::is_nothrow_copy_constructible_v<T>)
      : has_(true) {
    ::new (std::addressof(sto_.val_)) T(v);
  }

  constexpr expected(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>)
      : has_(true) {
    ::new (std::addressof(sto_.val_)) T(std::move(v));
  }

  // unexpected constructors
  constexpr expected(const unexpected<E>& u) noexcept(
      std::is_nothrow_copy_constructible_v<E>)
      : has_(false) {
    ::new (std::addressof(sto_.err_)) E(u.error());
  }

  constexpr expected(unexpected<E>&& u) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : has_(false) {
    ::new (std::addressof(sto_.err_)) E(std::move(u.error()));
  }

  // in_place value
  template <class... Args,
            std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
  constexpr explicit expected(std::in_place_t, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>)
      : has_(true) {
    ::new (std::addressof(sto_.val_)) T(std::forward<Args>(args)...);
  }

  template <class U,
            class... Args,
            std::enable_if_t<
                std::is_constructible_v<T, std::initializer_list<U>&, Args...>,
                int> = 0>
  constexpr explicit expected(
      std::in_place_t,
      std::initializer_list<U> il,
      Args&&... args) noexcept(std::
                                   is_nothrow_constructible_v<
                                       T,
                                       std::initializer_list<U>&,
                                       Args...>)
      : has_(true) {
    ::new (std::addressof(sto_.val_)) T(il, std::forward<Args>(args)...);
  }

  // unexpect (error in-place)
  template <class... Args,
            std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
  constexpr explicit expected(unexpect_t, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<E, Args...>)
      : has_(false) {
    ::new (std::addressof(sto_.err_)) E(std::forward<Args>(args)...);
  }

  template <class U,
            class... Args,
            std::enable_if_t<
                std::is_constructible_v<E, std::initializer_list<U>&, Args...>,
                int> = 0>
  constexpr explicit expected(
      unexpect_t,
      std::initializer_list<U> il,
      Args&&... args) noexcept(std::
                                   is_nothrow_constructible_v<
                                       E,
                                       std::initializer_list<U>&,
                                       Args...>)
      : has_(false) {
    ::new (std::addressof(sto_.err_)) E(il, std::forward<Args>(args)...);
  }

  // copy/move
  constexpr expected(const expected& other) noexcept(
      std::conjunction_v<std::is_nothrow_copy_constructible<T>,
                         std::is_nothrow_copy_constructible<E>>)
      : has_(other.has_) {
    if (has_) {
      ::new (std::addressof(sto_.val_)) T(other.value());
    } else {
      ::new (std::addressof(sto_.err_)) E(other.error());
    }
  }

  constexpr expected(expected&& other) noexcept(
      std::conjunction_v<std::is_nothrow_move_constructible<T>,
                         std::is_nothrow_move_constructible<E>>)
      : has_(other.has_) {
    if (has_) {
      ::new (std::addressof(sto_.val_)) T(std::move(other.value()));
    } else {
      ::new (std::addressof(sto_.err_)) E(std::move(other.error()));
    }
  }

  ~expected() { destroy(); }

  // ------------------ assignment ------------------
  expected& operator=(const expected& rhs) noexcept(
      std::conjunction_v<std::is_nothrow_copy_constructible<T>,
                         std::is_nothrow_copy_constructible<E>,
                         std::is_nothrow_move_assignable<T>,
                         std::is_nothrow_move_assignable<E>>) {
    if (this == &rhs)
      return *this;
    if (has_ && rhs.has_) {
      value() = rhs.value();
    } else if (!has_ && !rhs.has_) {
      error() = rhs.error();
    } else if (rhs.has_) {
      // move from error -> value
      destroy_error();
      ::new (std::addressof(sto_.val_)) T(rhs.value());
      has_ = true;
    } else {
      // move from value -> error
      destroy_value();
      ::new (std::addressof(sto_.err_)) E(rhs.error());
      has_ = false;
    }
    return *this;
  }

  expected& operator=(expected&& rhs) noexcept(
      std::conjunction_v<std::is_nothrow_move_constructible<T>,
                         std::is_nothrow_move_constructible<E>,
                         std::is_nothrow_move_assignable<T>,
                         std::is_nothrow_move_assignable<E>>) {
    if (this == &rhs)
      return *this;
    if (has_ && rhs.has_) {
      value() = std::move(rhs.value());
    } else if (!has_ && !rhs.has_) {
      error() = std::move(rhs.error());
    } else if (rhs.has_) {
      destroy_error();
      ::new (std::addressof(sto_.val_)) T(std::move(rhs.value()));
      has_ = true;
    } else {
      destroy_value();
      ::new (std::addressof(sto_.err_)) E(std::move(rhs.error()));
      has_ = false;
    }
    return *this;
  }

  // assign from T
  template <class U = T,
            std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
  expected& operator=(U&& v) noexcept(std::is_nothrow_constructible_v<T, U&&> &&
                                      std::is_nothrow_move_assignable_v<T>) {
    if (has_) {
      value() = std::forward<U>(v);
    } else {
      destroy_error();
      ::new (std::addressof(sto_.val_)) T(std::forward<U>(v));
      has_ = true;
    }
    return *this;
  }

  // assign from unexpected<E>
  expected& operator=(const unexpected<E>& u) noexcept(
      std::is_nothrow_copy_constructible_v<E> &&
      std::is_nothrow_move_assignable_v<E>) {
    if (has_) {
      destroy_value();
      ::new (std::addressof(sto_.err_)) E(u.error());
      has_ = false;
    } else {
      error() = u.error();
    }
    return *this;
  }

  expected& operator=(unexpected<E>&& u) noexcept(
      std::is_nothrow_move_constructible_v<E> &&
      std::is_nothrow_move_assignable_v<E>) {
    if (has_) {
      destroy_value();
      ::new (std::addressof(sto_.err_)) E(std::move(u.error()));
      has_ = false;
    } else {
      error() = std::move(u.error());
    }
    return *this;
  }

  // ------------------ emplace & swap ------------------
  template <class... Args>
  T& emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    if (!has_) {
      destroy_error();
    } else {
      destroy_value();
    }
    ::new (std::addressof(sto_.val_)) T(std::forward<Args>(args)...);
    has_ = true;
    return value();
  }

  void swap(expected& other) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                      std::is_nothrow_move_constructible_v<E> &&
                                      std::is_nothrow_swappable_v<T> &&
                                      std::is_nothrow_swappable_v<E>) {
    using std::swap;
    if (has_ && other.has_) {
      swap(value(), other.value());
    } else if (!has_ && !other.has_) {
      swap(error(), other.error());
    } else if (has_ && !other.has_) {
      expected tmp(unexpect, std::move(other.error()));
      other.destroy_error();
      ::new (std::addressof(other.sto_.val_)) T(std::move(value()));
      other.has_ = true;
      destroy_value();
      ::new (std::addressof(sto_.err_)) E(std::move(tmp.error()));
      has_ = false;
    } else {  // !has_ && other.has_
      other.swap(*this);
    }
  }

  // ------------------ observers ------------------
  constexpr bool has_value() const noexcept { return has_; }
  constexpr explicit operator bool() const noexcept { return has_; }

  constexpr T* operator->() noexcept {
    assert(has_);
    return std::addressof(sto_.val_);
  }
  constexpr const T* operator->() const noexcept {
    assert(has_);
    return std::addressof(sto_.val_);
  }

  constexpr T& operator*() & noexcept {
    assert(has_);
    return sto_.val_;
  }
  constexpr const T& operator*() const& noexcept {
    assert(has_);
    return sto_.val_;
  }
  constexpr T&& operator*() && noexcept {
    assert(has_);
    return std::move(sto_.val_);
  }
  constexpr const T&& operator*() const&& noexcept {
    assert(has_);
    return std::move(sto_.val_);
  }

  constexpr T& value() & {
    if (!has_)
      throw bad_expected_access<E>(error());
    return sto_.val_;
  }
  constexpr const T& value() const& {
    if (!has_)
      throw bad_expected_access<E>(error());
    return sto_.val_;
  }
  constexpr T&& value() && {
    if (!has_)
      throw bad_expected_access<E>(std::move(error()));
    return std::move(sto_.val_);
  }
  constexpr const T&& value() const&& {
    if (!has_)
      throw bad_expected_access<E>(std::move(error()));
    return std::move(sto_.val_);
  }

  constexpr E& error() & noexcept {
    assert(!has_);
    return sto_.err_;
  }
  constexpr const E& error() const& noexcept {
    assert(!has_);
    return sto_.err_;
  }
  constexpr E&& error() && noexcept {
    assert(!has_);
    return std::move(sto_.err_);
  }
  constexpr const E&& error() const&& noexcept {
    assert(!has_);
    return std::move(sto_.err_);
  }

  template <class U>
  constexpr T value_or(U&& u) const& {
    if (has_)
      return sto_.val_;
    return static_cast<T>(std::forward<U>(u));
  }
  template <class U>
  constexpr T value_or(U&& u) && {
    if (has_)
      return std::move(sto_.val_);
    return static_cast<T>(std::forward<U>(u));
  }

  // ------------------ monadic ops ------------------
  // transform: F(T) -> U
  template <class F>
  constexpr auto transform(F&& f) const& -> expected<
      std::decay_t<decltype(std::invoke(f, std::declval<const T&>()))>,
      E> {
    using U = std::decay_t<decltype(std::invoke(f, std::declval<const T&>()))>;
    if (has_)
      return expected<U, E>(std::in_place,
                            std::invoke(std::forward<F>(f), value()));
    return expected<U, E>(unexpect, error());
  }

  template <class F>
  constexpr auto transform(F&& f) && -> expected<
      std::decay_t<decltype(std::invoke(f, std::declval<T&&>()))>,
      E> {
    using U = std::decay_t<decltype(std::invoke(f, std::declval<T&&>()))>;
    if (has_)
      return expected<U, E>(
          std::in_place,
          std::invoke(std::forward<F>(f), std::move(*this).value()));
    return expected<U, E>(unexpect, std::move(*this).error());
  }

  // and_then: F(T) -> expected<U,E>
  template <class F>
  constexpr auto and_then(
      F&& f) const& -> decltype(std::invoke(std::forward<F>(f),
                                            std::declval<const T&>())) {
    using Ret =
        decltype(std::invoke(std::forward<F>(f), std::declval<const T&>()));
    if (has_)
      return std::invoke(std::forward<F>(f), value());
    return Ret(unexpect, error());
  }

  template <class F>
  constexpr auto and_then(
      F&& f) && -> decltype(std::invoke(std::forward<F>(f),
                                        std::declval<T&&>())) {
    using Ret = decltype(std::invoke(std::forward<F>(f), std::declval<T&&>()));
    if (has_)
      return std::invoke(std::forward<F>(f), std::move(*this).value());
    return Ret(unexpect, std::move(*this).error());
  }

  // or_else: G(E) -> expected<T, E>  (or -> some compatible expected)
  template <class G>
  constexpr expected or_else(G&& g) const& {
    if (has_)
      return *this;
    return std::forward<G>(g)(error());
  }

  template <class G>
  constexpr expected or_else(G&& g) && {
    if (has_)
      return std::move(*this);
    return std::forward<G>(g)(std::move(error()));
  }

  // ------------------ comparisons ------------------
  template <class U, class V>
    requires std::equality_comparable<U> && std::equality_comparable<V>
  friend constexpr bool operator==(expected<T, E> const& a,
                                   expected<U, V> const& b) {
    if (a.has_ != b.has_)
      return false;
    return a.has_ ? (a.value() == b.value()) : (a.error() == b.error());
  }

 private:
  void destroy_value() noexcept {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      if (has_)
        sto_.val_.~T();
    }
  }
  void destroy_error() noexcept {
    if constexpr (!std::is_trivially_destructible_v<E>) {
      if (!has_)
        sto_.err_.~E();
    }
  }
  void destroy() noexcept {
    if (has_)
      destroy_value();
    else
      destroy_error();
  }

  __exp_detail::storage<T, E> sto_;
  bool has_;
};

// ---------------------------------------------
// expected<void, E> specialization
// ---------------------------------------------
template <class E>
class expected<void, E> {
  static_assert(!std::is_void_v<E>, "expected<void, void> is ill-formed");

 public:
  using value_type = void;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  constexpr expected() noexcept : has_(true) {}

  constexpr expected(unexpect_t, const E& e) noexcept(
      std::is_nothrow_copy_constructible_v<E>)
      : has_(false) {
    ::new (std::addressof(err_)) E(e);
  }

  constexpr expected(unexpect_t,
                     E&& e) noexcept(std::is_nothrow_move_constructible_v<E>)
      : has_(false) {
    ::new (std::addressof(err_)) E(std::move(e));
  }

  constexpr expected(const unexpected<E>& u) noexcept(
      std::is_nothrow_copy_constructible_v<E>)
      : expected(unexpect, u.error()) {}

  constexpr expected(unexpected<E>&& u) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : expected(unexpect, std::move(u.error())) {}

  constexpr expected(const expected& o) noexcept(
      std::is_nothrow_copy_constructible_v<E>)
      : has_(o.has_) {
    if (!has_)
      ::new (std::addressof(err_)) E(o.error());
  }

  constexpr expected(expected&& o) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : has_(o.has_) {
    if (!has_)
      ::new (std::addressof(err_)) E(std::move(o.error()));
  }

  ~expected() { destroy(); }

  expected& operator=(const expected& rhs) noexcept(
      std::is_nothrow_copy_constructible_v<E> &&
      std::is_nothrow_move_assignable_v<E>) {
    if (this == &rhs)
      return *this;
    if (has_ && rhs.has_)
      return *this;
    if (!has_ && !rhs.has_) {
      error() = rhs.error();
      return *this;
    }
    if (rhs.has_) {
      destroy_error();
      has_ = true;
    } else {
      destroy();
      ::new (std::addressof(err_)) E(rhs.error());
      has_ = false;
    }
    return *this;
  }

  expected& operator=(expected&& rhs) noexcept(
      std::is_nothrow_move_constructible_v<E> &&
      std::is_nothrow_move_assignable_v<E>) {
    if (this == &rhs)
      return *this;
    if (has_ && rhs.has_)
      return *this;
    if (!has_ && !rhs.has_) {
      error() = std::move(rhs.error());
      return *this;
    }
    if (rhs.has_) {
      destroy_error();
      has_ = true;
    } else {
      destroy();
      ::new (std::addressof(err_)) E(std::move(rhs.error()));
      has_ = false;
    }
    return *this;
  }

  constexpr bool has_value() const noexcept { return has_; }
  constexpr explicit operator bool() const noexcept { return has_; }

  // value(): no payload, just check or throw
  constexpr void value() const {
    if (!has_)
      throw bad_expected_access<E>(error());
  }

  constexpr E& error() & noexcept {
    assert(!has_);
    return err_;
  }
  constexpr const E& error() const& noexcept {
    assert(!has_);
    return err_;
  }
  constexpr E&& error() && noexcept {
    assert(!has_);
    return std::move(err_);
  }
  constexpr const E&& error() const&& noexcept {
    assert(!has_);
    return std::move(err_);
  }

  // and_then: F() -> expected<U,E>
  template <class F>
  constexpr auto and_then(
      F&& f) const& -> decltype(std::invoke(std::forward<F>(f))) {
    using Ret = decltype(std::invoke(std::forward<F>(f)));
    if (has_)
      return std::invoke(std::forward<F>(f));
    return Ret(unexpect, error());
  }

  template <class F>
  constexpr auto and_then(
      F&& f) && -> decltype(std::invoke(std::forward<F>(f))) {
    using Ret = decltype(std::invoke(std::forward<F>(f)));
    if (has_)
      return std::invoke(std::forward<F>(f));
    return Ret(unexpect, std::move(error()));
  }

  // transform: F() -> U
  template <class F>
  constexpr auto transform(F&& f) const& -> expected<
      std::decay_t<decltype(std::invoke(std::forward<F>(f)))>,
      E> {
    using U = std::decay_t<decltype(std::invoke(std::forward<F>(f)))>;
    if (has_)
      return expected<U, E>(std::in_place, std::invoke(std::forward<F>(f)));
    return expected<U, E>(unexpect, error());
  }

  // or_else: G(E) -> expected<void,E>
  template <class G>
  constexpr expected or_else(G&& g) const& {
    if (has_)
      return *this;
    return std::forward<G>(g)(error());
  }

  template <class G>
  constexpr expected or_else(G&& g) && {
    if (has_)
      return std::move(*this);
    return std::forward<G>(g)(std::move(error()));
  }

  void swap(expected& other) noexcept(std::is_nothrow_move_constructible_v<E> &&
                                      std::is_nothrow_swappable_v<E>) {
    using std::swap;
    if (has_ && other.has_)
      return;
    if (!has_ && !other.has_) {
      swap(error(), other.error());
      return;
    }

    if (has_ && !other.has_) {
      ::new (std::addressof(err_)) E(std::move(other.error()));
      has_ = false;
      other.destroy_error();
      other.has_ = true;
    } else {  // !has_ && other.has_
      other.swap(*this);
    }
  }

  friend constexpr bool operator==(const expected& a, const expected& b) {
    if (a.has_ != b.has_)
      return false;
    if (a.has_)
      return true;
    return a.error() == b.error();
  }

 private:
  void destroy_error() noexcept {
    if constexpr (!std::is_trivially_destructible_v<E>) {
      if (!has_)
        err_.~E();
    }
  }
  void destroy() noexcept {
    if (!has_)
      destroy_error();
  }

  union {
    unsigned char dummy_;
    E err_;
  };
  bool has_;
};

// Non-member swap
template <class T, class E>
void swap(expected<T, E>& a, expected<T, E>& b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}
