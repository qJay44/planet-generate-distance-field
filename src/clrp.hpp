#pragma once

namespace clrp {
enum class ATTRIBUTE : u16 {
  DEFAULT,
  BOLD,
  ITALICS,
  UNDERLINED,
  BLINK,
  REVERSE = 7,
  HIDDEN,
};

enum class FG : u16 {
  DEFAULT = 39,
  BLACK = 30,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  LIGHT_GRAY,
  DARK_GRAY = 90,
  LIGHT_RED,
  LIGHT_GREEN,
  LIGHT_YELLOW,
  LIGHT_BLUE,
  LIGHT_MAGENTA,
  LIGHT_CYAN,
  WHITE,
};

enum class BG : u16 {
  DEFAULT = 49,
  BLACK = 40,
  RED = 41,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  LIGHT_GRAY,
  DARK_GRAY = 100,
  LIGHT_RED,
  LIGHT_GREEN,
  LIGHT_YELLOW,
  LIGHT_BLUE,
  LIGHT_MAGENTA,
  LIGHT_CYAN,
  WHITE,
};

enum class RESET : u16 {
  ALL,
  BOLD = 21,
  DIM,
  UNDERLINED = 24,
  BLINK,
  REVERSE = 27,
  HIDDEN,
};

struct clrp_t {
  ATTRIBUTE attr = ATTRIBUTE::DEFAULT;
  FG fg = FG::DEFAULT;
  BG bg = BG::DEFAULT;
  RESET reset = RESET::ALL;
};

[[nodiscard]]
std::string format(
  const std::string& str,
  ATTRIBUTE attr = ATTRIBUTE::DEFAULT,
  FG fg = FG::DEFAULT,
  BG bg = BG::DEFAULT,
  RESET reset = RESET::ALL
);

[[nodiscard]]
std::string format(const std::string& str, clrp_t clrp);

// @return string with %s
[[nodiscard]]
std::string prepare( //
  ATTRIBUTE attr = ATTRIBUTE::DEFAULT,
  FG fg = FG::DEFAULT,
  BG bg = BG::DEFAULT,
  RESET reset = RESET::ALL
);

// @return string with %s
[[nodiscard]]
std::string prepare(clrp_t clrp);
} // namespace clrp
