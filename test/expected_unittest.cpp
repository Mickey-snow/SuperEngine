
#include "utilities/expected.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

TEST(Expected_Basics, DefaultConstructionHasValue) {
  expected<int, std::string> e;
  EXPECT_TRUE(e.has_value());
  EXPECT_TRUE(static_cast<bool>(e));
  EXPECT_EQ(e.value(), 0);  // int default-constructed to 0
}

TEST(Expected_Basics, ConstructWithValueAndError) {
  expected<int, std::string> v{42};
  EXPECT_TRUE(v.has_value());
  EXPECT_EQ(v.value(), 42);

  expected<int, std::string> err{unexpected<std::string>("nope")};
  EXPECT_FALSE(err.has_value());
  EXPECT_EQ(err.error(), "nope");
}

TEST(Expected_Throwing, ValueThrowsWhenError) {
  expected<int, std::string> e(unexpect, std::string("boom"));
  EXPECT_FALSE(e.has_value());
  EXPECT_THROW(
      {
        try {
          (void)e.value();
        } catch (const bad_expected_access<std::string>& ex) {
          EXPECT_EQ(ex.error(), "boom");
          throw;
        }
      },
      bad_expected_access<std::string>);
}

TEST(Expected_Observers, ValueOr) {
  expected<int, std::string> a{7};
  expected<int, std::string> b(unexpect, "err");
  EXPECT_EQ(a.value_or(99), 7);
  EXPECT_EQ(std::move(a).value_or(88), 7);
  EXPECT_EQ(b.value_or(99), 99);
  EXPECT_EQ(std::move(b).value_or(77), 77);
}

TEST(Expected_Modifiers, EmplaceAndAssignments) {
  expected<std::string, std::string> e(unexpect, "err");
  auto& ref = e.emplace(3, 'x');  // "xxx"
  EXPECT_TRUE(e.has_value());
  EXPECT_EQ(ref, "xxx");
  EXPECT_EQ(e.value(), "xxx");

  e = std::string("abc");
  EXPECT_TRUE(e.has_value());
  EXPECT_EQ(e.value(), "abc");

  e = unexpected<std::string>("ng");
  EXPECT_FALSE(e.has_value());
  EXPECT_EQ(e.error(), "ng");

  e = unexpected<std::string>("ng2");
  EXPECT_FALSE(e.has_value());
  EXPECT_EQ(e.error(), "ng2");
}

TEST(Expected_Modifiers, Swap) {
  expected<int, std::string> a{1};
  expected<int, std::string> b(unexpect, "E");
  swap(a, b);
  EXPECT_FALSE(a.has_value());
  EXPECT_EQ(a.error(), "E");
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ(b.value(), 1);
}

TEST(Expected_Comparisons, EqCompareOnValueAndError) {
  expected<int, std::string> a{3}, b{3}, c{4};
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);

  expected<int, std::string> e1(unexpect, "x"), e2(unexpect, "x"),
      e3(unexpect, "y");
  EXPECT_TRUE(e1 == e2);
  EXPECT_FALSE(e1 == e3);

  EXPECT_FALSE(a == e1);
  EXPECT_FALSE(e1 == a);
}

TEST(Expected_Monads, Transform) {
  expected<int, std::string> v{5};
  expected<int, std::string> e(unexpect, "bad");

  auto tv = v.transform([](int x) { return x * x; });
  auto te = e.transform([](int x) { return x * x; });
  EXPECT_TRUE(tv.has_value());
  EXPECT_EQ(tv.value(), 25);
  EXPECT_FALSE(te.has_value());
  EXPECT_EQ(te.error(), "bad");
}

TEST(Expected_Monads, AndThen) {
  auto inc_if_pos = [](int x) -> expected<int, std::string> {
    if (x > 0)
      return x + 1;
    return unexpected<std::string>("nonpos");
  };

  expected<int, std::string> a{1};
  expected<int, std::string> b{0};
  expected<int, std::string> er(unexpect, "e");

  auto ra = a.and_then(inc_if_pos);
  auto rb = b.and_then(inc_if_pos);
  auto rr = er.and_then(inc_if_pos);

  EXPECT_TRUE(ra.has_value());
  EXPECT_EQ(ra.value(), 2);
  EXPECT_FALSE(rb.has_value());
  EXPECT_EQ(rb.error(), "nonpos");
  EXPECT_FALSE(rr.has_value());
  EXPECT_EQ(rr.error(), "e");
}

TEST(Expected_Monads, OrElse) {
  auto recover = [](const std::string& s) -> expected<int, std::string> {
    return static_cast<int>(s.size());  // recover to value
  };

  expected<int, std::string> ok{7};
  expected<int, std::string> ng(unexpect, "oops");

  auto rok = ok.or_else(recover);
  auto rng = ng.or_else(recover);

  EXPECT_TRUE(rok.has_value());
  EXPECT_EQ(rok.value(), 7);  // untouched
  EXPECT_TRUE(rng.has_value());
  EXPECT_EQ(rng.value(), 4);  // "oops".size()
}

TEST(Expected_MoveOnly, UniquePtrValue) {
  expected<std::unique_ptr<int>, std::string> e{std::make_unique<int>(5)};
  ASSERT_TRUE(e.has_value());
  ASSERT_TRUE(e.value());
  EXPECT_EQ(*e.value(), 5);

  expected<std::unique_ptr<int>, std::string> m = std::move(e);
  EXPECT_TRUE(m.has_value());
  ASSERT_TRUE(m.value());
  EXPECT_EQ(*m.value(), 5);

  // moved-from unique_ptr is null but still "has_value()" in the source
  EXPECT_TRUE(e.has_value());
  EXPECT_EQ(e.value(), nullptr);
}

TEST(Unexpected_Basics, ConstructionAndErrorAccess) {
  unexpected<std::string> u("why");
  EXPECT_EQ(u.error(), "why");

  auto u2 = make_unexpected(std::string("boom"));
  EXPECT_EQ(u2.error(), "boom");
}

TEST(Expected_Void, BasicsAndMonads) {
  expected<void, std::string> ok;
  EXPECT_TRUE(ok.has_value());
  EXPECT_NO_THROW(ok.value());  // no payload

  expected<void, std::string> er(unexpect, "ng");
  EXPECT_FALSE(er.has_value());
  EXPECT_THROW(
      {
        try {
          er.value();
        } catch (const bad_expected_access<std::string>& ex) {
          EXPECT_EQ(ex.error(), "ng");
          throw;
        }
      },
      bad_expected_access<std::string>);

  // and_then on void -> expected<int, E>
  auto f = []() -> expected<int, std::string> { return 9; };
  auto r1 = ok.and_then(f);
  auto r2 = er.and_then(f);
  EXPECT_TRUE(r1.has_value());
  EXPECT_EQ(r1.value(), 9);
  EXPECT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), "ng");

  // transform on void -> expected<std::string, E>
  auto r3 = ok.transform([] { return std::string("done"); });
  auto r4 = er.transform([] { return std::string("done"); });
  EXPECT_TRUE(r3.has_value());
  EXPECT_EQ(r3.value(), "done");
  EXPECT_FALSE(r4.has_value());
  EXPECT_EQ(r4.error(), "ng");

  // or_else
  auto recover = [](const std::string& e) -> expected<void, std::string> {
    (void)e;
    return expected<void, std::string>{};  // recover to ok
  };
  auto r5 = er.or_else(recover);
  EXPECT_TRUE(r5.has_value());
}

TEST(Interop_Misc, UnexpectTokenExists) {
  // Simple smoke checks for tokens / types
  static_assert(std::is_default_constructible_v<unexpect_t>,
                "unexpect_t must be default-constructible");
  (void)unexpect;  // should be an inline constexpr object
}
