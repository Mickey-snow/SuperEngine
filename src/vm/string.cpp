// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#include "vm/string.hpp"

#include "vm/object.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <format>
#include <stdexcept>

namespace serilang {

namespace {

enum class StringMethodKind { Substr, Prefix, Suffix, Length };

class StringMethod : public IObject {
 public:
  static constexpr inline ObjType objtype = ObjType::BoundMethod;

  StringMethod(const String* self, StringMethodKind kind)
      : self_(self), kind_(kind) {}

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override {
    visitor.MarkSub(const_cast<String*>(self_));
  }

  std::string Str() const override { return Desc(); }
  std::string Desc() const override {
    return std::format("<bound method str.{}>", MethodName());
  }

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) override {
    if (nkwargs != 0) {
      vm.Error(f, std::format("str.{} does not accept keyword arguments",
                              MethodName()));
      return;
    }

    const size_t base = f.op_stack.size() - nargs - 2 * nkwargs - 1;

    auto read_int = [&](size_t offset,
                        std::string_view what) -> std::optional<int> {
      Value& v = f.op_stack[base + 1 + offset];
      if (auto* iv = v.Get_if<int>())
        return *iv;
      vm.Error(f,
               std::format("{} must be integer, but got: {}", what, v.Desc()));
      return std::nullopt;
    };

    auto clamp_index = [&](int raw, bool allow_end) -> std::optional<size_t> {
      const int len = static_cast<int>(self_->str_.size());
      int idx = raw < 0 ? len + raw : raw;
      if (idx < 0 || idx > len || (!allow_end && idx == len)) {
        vm.Error(f, std::format("string index '{}' out of range", raw));
        return std::nullopt;
      }
      return static_cast<size_t>(idx);
    };

    auto ensure_non_negative =
        [&](int value, std::string_view what) -> std::optional<size_t> {
      if (value < 0) {
        vm.Error(f, std::format("{} must be non-negative", what));
        return std::nullopt;
      }
      return static_cast<size_t>(value);
    };

    std::string result;

    switch (kind_) {
      case StringMethodKind::Substr: {
        if (nargs == 0 || nargs > 2) {
          vm.Error(f, std::format("str.substr expects 1 or 2 arguments, got {}",
                                  nargs));
          return;
        }

        auto start_raw = read_int(0, "substr start");
        if (!start_raw)
          return;
        auto start_idx = clamp_index(*start_raw, /*allow_end=*/true);
        if (!start_idx)
          return;

        size_t count = std::string::npos;
        if (nargs == 2) {
          auto len_raw = read_int(1, "substr length");
          if (!len_raw)
            return;
          auto len = ensure_non_negative(*len_raw, "substr length");
          if (!len)
            return;
          count = *len;
        }

        try {
          result = self_->str_.substr(*start_idx, count);
        } catch (const std::out_of_range&) {
          vm.Error(f,
                   std::format("string index '{}' out of range", *start_raw));
          return;
        }
        break;
      }

      case StringMethodKind::Prefix: {
        if (nargs != 1) {
          vm.Error(f,
                   std::format("str.prefix expects exactly 1 argument, got {}",
                               nargs));
          return;
        }
        auto count_raw = read_int(0, "prefix length");
        if (!count_raw)
          return;
        auto len = ensure_non_negative(*count_raw, "prefix length");
        if (!len)
          return;
        if (*len >= self_->str_.size())
          result = self_->str_;
        else
          result = self_->str_.substr(0, *len);

        break;
      }

      case StringMethodKind::Suffix: {
        if (nargs != 1) {
          vm.Error(f,
                   std::format("str.suffix expects exactly 1 argument, got {}",
                               nargs));
          return;
        }
        auto count_raw = read_int(0, "suffix length");
        if (!count_raw)
          return;
        auto len = ensure_non_negative(*count_raw, "suffix length");
        if (!len)
          return;
        if (*len >= self_->str_.size())
          result = self_->str_;
        else
          result = self_->str_.substr(self_->str_.size() - *len);

        break;
      }

      case StringMethodKind::Length: {
        if (nargs != 0) {
          vm.Error(f,
                   std::format("str.len expects no argument, got {}", nargs));
          return;
        }
        f.op_stack.resize(base + 1);
        f.op_stack.back() = Value(static_cast<int>(self_->str_.length()));
        return;
      }
    }

    f.op_stack.resize(base + 1);
    String* str = vm.gc_->Allocate<String>(std::move(result));
    f.op_stack.back() = Value(str);
  }

 private:
  std::string_view MethodName() const {
    switch (kind_) {
      case StringMethodKind::Substr:
        return "substr";
      case StringMethodKind::Prefix:
        return "prefix";
      case StringMethodKind::Suffix:
        return "suffix";
      case StringMethodKind::Length:
        return "len";
    }
    return "";
  }

  const String* self_;
  StringMethodKind kind_;
};

}  // namespace

// -----------------------------------------------------------------------

TempValue String::Member(std::string_view mem) {
  if (mem == "substr")
    return std::make_unique<StringMethod>(this, StringMethodKind::Substr);
  if (mem == "prefix")
    return std::make_unique<StringMethod>(this, StringMethodKind::Prefix);
  if (mem == "suffix")
    return std::make_unique<StringMethod>(this, StringMethodKind::Suffix);
  if (mem == "len")
    return std::make_unique<StringMethod>(this, StringMethodKind::Length);

  return IObject::Member(mem);
}

void String::GetItem(VM& vm, Fiber& f) {
  Value idx_val = std::move(f.op_stack.back());
  f.op_stack.pop_back();

  auto* idx_ptr = idx_val.Get_if<int>();
  if (!idx_ptr) {
    vm.Error(f, "string index must be integer, but got: " + idx_val.Desc());
    return;
  }

  const int len = static_cast<int>(str_.size());
  int index = *idx_ptr < 0 ? len + *idx_ptr : *idx_ptr;
  if (index < 0 || index >= len) {
    vm.Error(f, "string index '" + idx_val.Str() + "' out of range");
    return;
  }

  String* result =
      vm.gc_->Allocate<String>(str_.substr(static_cast<size_t>(index), 1));
  f.op_stack.back() = Value(result);
}

std::string String::Str() const { return str_; }
std::string String::Desc() const { return std::format("<str: {}>", str_); }

void String::MarkRoots(GCVisitor& visitor) {
  // nothing
}

std::optional<bool> String::Bool() const { return !str_.empty(); }

}  // namespace serilang
