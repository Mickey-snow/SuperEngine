
#include <gtest/gtest.h>

#include "systems/base/text_layout.hpp"
#include "utilities/string_utilities.hpp"

class TextLayoutEngineTest : public ::testing::Test {
 protected:
  static constexpr char kinsoku_char = 0x21;

  static constexpr int font_size = 24;
  static constexpr int ruby_size = 0;
  static constexpr int x_spacing = 2, y_spacing = 3;
  static constexpr int x_win_size = 4, y_win_size = 3;

  // - x_window_size_in_chars = 4  => normal_width = 4*(24+2) = 104
  // - extended_width = normal_width + default_font_size = 104 + 24 = 128
  static constexpr int win_height =
      y_win_size * (font_size + y_spacing + ruby_size);
  static constexpr int win_width_normal = x_win_size * (font_size + x_spacing);
  static constexpr int win_width_extended = win_width_normal + font_size;

  TextLayout engine =
      TextLayout(win_height, win_width_normal, win_width_extended);

  void SetUp() override {
    engine.font_size = font_size;
    engine.ruby_font_size = ruby_size;
    engine.x_spacing = x_spacing;
    engine.y_spacing = y_spacing;
  }
};

TEST_F(TextLayoutEngineTest, ChecksNormalWidth) {
  EXPECT_THROW(TextLayout(10, 10, 9), std::invalid_argument);
  EXPECT_NO_THROW(TextLayout(10, 10, 10));
}

TEST_F(TextLayoutEngineTest, WrappingWidth) {
  {
    const int w = engine.WidthFor(static_cast<int>('A'));  // 65
    EXPECT_EQ(w, 13) << "wrapping width for western char should be half width";
  }
  {  // Any codepoint >= 127 should be treated full-width.
    const int w = engine.WidthFor(0x3042 /* 'あ' */);
    EXPECT_EQ(w, 26) << "wrapping width for non-ascii should be full width";
  }
}

TEST_F(TextLayoutEngineTest, LineHeight) {
  // line_height = font_size + y_spacing + ruby_size
  EXPECT_EQ(engine.GetLineHeight(), ruby_size + font_size + y_spacing);
}

TEST_F(TextLayoutEngineTest, IsFull) {
  engine.insertion_y = 0;
  EXPECT_FALSE(engine.IsFull());

  engine.insertion_y = win_height - 1;
  EXPECT_FALSE(engine.IsFull());

  engine.insertion_y = win_height;
  EXPECT_TRUE(engine.IsFull());
}

TEST_F(TextLayoutEngineTest, HardBreak) {
  engine.SetIndent(50);

  // Change current insertion to something else.
  engine.insertion_x = 5;
  engine.insertion_y = 10;

  engine.HardBreak();
  EXPECT_EQ(engine.insertion_x, 50) << "should jump back to indent_pixels";
  EXPECT_EQ(engine.insertion_y, 10 + engine.GetLineHeight());
}

TEST_F(TextLayoutEngineTest, LineBreak_KinsokuSpace) {
  constexpr int space = 0x20;
  const int cw = engine.WidthFor(space);

  engine.insertion_x = win_width_extended - cw;

  EXPECT_FALSE(engine.MustLineBreakFor(space, /*rest=*/std::string_view{}));
}

TEST_F(TextLayoutEngineTest, LineBreak_ImmediateOverflow) {
  // Non-kinsoku ASCII 'A'
  const int A = static_cast<int>('A');
  const int cw = engine.WidthFor(A);  // 13
  engine.insertion_x = win_width_normal - cw + 1;

  EXPECT_TRUE(engine.MustLineBreakFor(A, /*rest=*/std::string_view{}));
}

TEST_F(TextLayoutEngineTest, LineBreak_PeekOverKinsoku) {
  const int A = static_cast<int>('A');
  const int cw = engine.WidthFor(A);  // 13
  engine.insertion_x = win_width_normal - cw;
  // After putting 'A', final_x == normal_width == 104 (fits).
  // Two more spaces (kinsoku) add 13 + 13 = 26, so 104 + 26 = 130 >
  // extended(128) → break.

  EXPECT_TRUE(IsKinsoku(kinsoku_char));
  std::string rest(2, kinsoku_char);
  EXPECT_TRUE(engine.MustLineBreakFor(A, rest))
      << "Current char fits normally, but following kinsoku (spaces) push "
         "beyond extended width -> should line break";
}

TEST_F(TextLayoutEngineTest, LineBreak_PeekWithinExtended) {
  const int A = static_cast<int>('A');
  const int cw = engine.WidthFor(A);  // 13
  engine.insertion_x = win_width_normal - cw;

  std::string rest(1, kinsoku_char);
  EXPECT_FALSE(engine.MustLineBreakFor(A, rest))
      << "Current char fits normally, and a single space keeps us within "
         "extended width -> no break";
}
