#include "console.hpp"

#include <cassert>
#include <optional>

#include <utf8proc.h>

static bool is_newline(int codepoint) {
  const utf8proc_property_t *property = utf8proc_get_property(codepoint);
  switch (property->category) {
  case UTF8PROC_CATEGORY_ZL: /* Separator, line */
  case UTF8PROC_CATEGORY_ZP: /* Separator, paragraph */
    return true;
  case UTF8PROC_CATEGORY_CC: /* Other, control */
    switch (property->boundclass) {
    case UTF8PROC_BOUNDCLASS_CR: // carriage return - \r
    case UTF8PROC_BOUNDCLASS_LF: // line feed - \n
      return true;
    default:
      break;
    }
    break;
  default:
    break;
  }
  return false;
}

static int get_character_width(int codepoint) {
  const utf8proc_property_t *property = utf8proc_get_property(codepoint);
  if (property->category == UTF8PROC_CATEGORY_CO) { // Private Use Area.
    return 1; // Width would otherwise be zero.
  }
  switch (property->charwidth) {
  default:
    return (int)property->charwidth;
  case 2:
    return 1;
  }
}

struct split {
  const unsigned char *break_point;
  int break_width;
  bool add_line_break;
};

struct FormattedPrinter {
  int fp_next_raw() {
    int codepoint;
    utf8proc_ssize_t len = utf8proc_iterate(string, end - string, &codepoint);
    assert(len >= 0);
    string += len;
    return codepoint;
  }

  color::RGBA fp_next_rgba() {
    int r = fp_next_raw();
    int g = fp_next_raw();
    int b = fp_next_raw();

    return color::RGBA{(uint8_t)r, (uint8_t)g, (uint8_t)b, 255};
  }

  int fp_peek() const {
    FormattedPrinter temp = *this;
    return temp.fp_next_raw();
  }

  split next_split_(int max_width, int can_split) const {
    FormattedPrinter it = *this;
    // The break point and width of the line.
    const unsigned char *break_point = it.end;
    int break_width = 0;
    // The current line width.
    int char_width = 0;
    bool separating = false; // True if the last iteration was breakable.
    while (it.string != it.end) {
      int codepoint = it.fp_peek();
      const utf8proc_property_t *property = utf8proc_get_property(codepoint);
      if (can_split && char_width > 0) {
        switch (property->category) {
        default:
          if (char_width + get_character_width(codepoint) > max_width) {
            // The next character would go over the max width, so return now.
            if (break_point != it.end) {
              // Use latest line break if one exists.
              return {break_point, break_width, true};
            } else {
              // Force a line break here.
              return {it.string, char_width, true};
            }
          }
          separating = false;
          break;
        case UTF8PROC_CATEGORY_PD: // Punctuation, dash
          if (char_width + get_character_width(codepoint) > max_width) {
            return {it.string, char_width, true};
          } else {
            char_width += get_character_width(codepoint);
            it.fp_next_raw();
            break_point = it.string;
            break_width = char_width;
            separating = true;
            continue;
          }
          break;
        case UTF8PROC_CATEGORY_ZS: // Separator, space
          if (!separating) {
            break_point = it.string;
            break_width = char_width;
            separating = true;
          }
          break;
        }
      }
      if (is_newline(codepoint)) {
        // Always break on newlines.
        return {it.string, char_width, false};
      }
      char_width += get_character_width(codepoint);
      it.fp_next_raw();
    }
    // Return end of iteration.
    return {it.string, char_width, false};
  }

  const unsigned char *__restrict string;
  const unsigned char *end;
  color::RGBA fg;
  color::RGBA bg;
  const color::RGBA default_fg;
  const color::RGBA default_bg;
};

struct PrintParams {

  int printn_internal_(std::string_view str) {
    if (str.empty()) {
      return 0;
    }
    FormattedPrinter printer = {
        (const unsigned char *)str.begin(),
        (const unsigned char *)str.end(),
        fg,
        bg,
        fg,
        bg,
    };
    if (!can_split && alignment == Console::Alignment::RIGHT) {
      x -= console.get_width() - 1;
      width = console.get_width();
    }
    // Expand the width/height of 0 to the edge of the console.
    width = width ? width : console.get_width() - x;
    height = height ? height : console.get_height() - y;
    // Print bounding box.
    int left = x;
    int right = x + width;
    int top = y;
    int bottom = y + height;
    assert(width > 0);
    assert(height > 0);
    while (printer.string != printer.end && top < bottom &&
           top < console.get_height()) {
      int codepoint = printer.fp_peek();
      const utf8proc_property_t *property = utf8proc_get_property(codepoint);
      // Check for newlines.
      if (is_newline(codepoint)) {
        if (property->category == UTF8PROC_CATEGORY_ZP) {
          top += 2;
        } else {
          top += 1;
        }
        printer.fp_next_raw();
        continue;
      }
      // Get the next line of characters.
      auto [line_break, line_width, add_line_break] =
          printer.next_split_(width, can_split);
      // Set cursor_x from alignment.
      int cursor_x = 0;
      switch (alignment) {
      default:
      case Console::Alignment::LEFT:
        cursor_x = left;
        break;
      case Console::Alignment::RIGHT:
        cursor_x = right - line_width;
        break;
      case Console::Alignment::CENTER:
        if (can_split) {
          cursor_x = left + (width - line_width) / 2;
        } else {
          cursor_x = left - (line_width / 2); // Deprecated.
        }
        break;
      }
      // True clipping area.  Prevent drawing outside of these bounds.
      int clip_left = left;
      int clip_right = right;
      if (!can_split) {
        // Bounds are ignored if splitting is off.
        clip_left = 0;
        clip_right = console.get_width();
      }
      while (printer.string < line_break) {
        // Iterate over a line of characters.
        codepoint = printer.fp_next_raw();
        if (get_character_width(codepoint) == 0) {
          continue;
        }
        if (clip_left <= cursor_x && cursor_x < clip_right) {
          // Actually render this line of characters.
          auto fg = printer.fg.a ? std::optional(printer.fg) : std::nullopt;
          auto bg = printer.bg.a ? std::optional(printer.bg) : std::nullopt;
          console.put_rgba(cursor_x, top, codepoint, fg, bg);
        }
        cursor_x += get_character_width(codepoint);
      }
      // Ignore any extra spaces.
      while (printer.string != printer.end) {
        // Separator, space
        codepoint = printer.fp_peek();
        if (utf8proc_get_property(codepoint)->category !=
            UTF8PROC_CATEGORY_ZS) {
          break;
        }
        printer.fp_next_raw();
      }
      // If there was an automatic split earlier then the top is moved down.
      if (add_line_break) {
        top += 1;
      }
    }
    return std::min(top, bottom) - y + 1;
  }

  Console &console; // Can not be NULL.
  int x;            // Cursor starting position.
  int y;
  int width;
  int height;
  const color::RGBA fg;
  const color::RGBA bg;
  Console::Alignment alignment;
  bool can_split; // In general `can_split = false` is deprecated.
};

void Console::print(const std::array<int, 2> &xy, std::string_view str,
                    std::optional<color::RGBA> fg,
                    std::optional<color::RGBA> bg, Alignment alignment) {
  PrintParams params = {
      *this,
      xy[0],
      xy[1],
      w,
      h,
      fg ? fg.value() : color::RGBA{255, 255, 255, 0},
      bg ? bg.value() : color::RGBA{255, 255, 255, 0},
      alignment,
      false,
  };
  params.printn_internal_(str);
};

int Console::print_rect(const std::array<int, 4> &rect, std::string_view str) {

  PrintParams params = {
      *this,
      rect[0],
      rect[1],
      rect[2],
      rect[3],
      {255, 255, 255, 0},
      {255, 255, 255, 0},
      Alignment::CENTER,
      true,
  };
  return params.printn_internal_(str);
}

void Console::draw_rect(const std::array<int, 4> &rect, int ch,
                        std::optional<color::RGBA> fg,
                        std::optional<color::RGBA> bg) {
  assert(0 <= rect[0] && rect[0] + rect[2] <= w);
  assert(0 <= rect[1] && rect[1] + rect[3] <= h);
  for (int console_y = rect[1]; console_y < rect[1] + rect[3]; console_y++) {
    for (int console_x = rect[0]; console_x < rect[0] + rect[2]; console_x++) {
      put_rgba(console_x, console_y, ch, fg, bg);
    }
  }
}

void Console::draw_frame(const std::array<int, 4> &rect,
                         const std::array<int, 9> &decoration,
                         std::optional<color::RGBA> fg,
                         std::optional<color::RGBA> bg) {
  const int left = rect[0];
  const int right = rect[0] + rect[2] - 1;
  const int top = rect[1];
  const int bottom = rect[1] + rect[3] - 1;
  put_rgba(left, top, decoration[0], fg, bg);
  draw_rect({rect[0] + 1, rect[1], rect[2] - 2, 1}, decoration[1], fg, bg);
  put_rgba(right, top, decoration[2], fg, bg);
  draw_rect({rect[0], rect[1] + 1, 1, rect[3] - 2}, decoration[3], fg, bg);
  draw_rect({rect[0] + 1, rect[1] + 1, rect[2] - 2, rect[3] - 2}, decoration[4],
            fg, bg);
  draw_rect({right, rect[1] + 1, 1, rect[3] - 2}, decoration[5], fg, bg);
  put_rgba(left, bottom, decoration[6], fg, bg);
  draw_rect({rect[0] + 1, bottom, rect[2] - 2, 1}, decoration[7], fg, bg);
  put_rgba(right, bottom, decoration[8], fg, bg);
}

static inline uint8_t alpha_blend(int src_c, int src_a, int dst_c, int dst_a,
                                  int out_a) {
  return (uint8_t)(((src_c * src_a) + (dst_c * dst_a * (255 - src_a) / 255)) /
                   out_a);
}

static inline color::RGBA blit_lerp_(const color::RGBA dst,
                                     const color::RGBA src, float interp) {
  uint8_t out_a = (uint8_t)(src.a + dst.a * (255 - src.a) / 255);
  if (out_a == 0) { // This would cause division by zero.
    return dst;     // Ignore alpha compositing and leave dst unchanged.
  }
  uint8_t src_a = (uint8_t)(src.a * interp);
  color::RGBA out = {
      alpha_blend(src.r, src_a, dst.r, dst.a, out_a),
      alpha_blend(src.g, src_a, dst.g, dst.a, out_a),
      alpha_blend(src.b, src_a, dst.b, dst.a, out_a),
      out_a,
  };
  return out;
}

static Console::Tile console_blit_cell_(const Console::Tile &src,
                                        const Console::Tile &dst,
                                        float fg_alpha, float bg_alpha,
                                        const color::RGBA *key_color) {
  if (key_color && key_color->r == src.bg.r && key_color->g == src.bg.g &&
      key_color->b == src.bg.b) {
    return dst; // Source pixel is transparent.
  }
  fg_alpha *= src.fg.a / 255.0f;
  bg_alpha *= src.bg.a / 255.0f;
  if (fg_alpha > 254.5f / 255.0f && bg_alpha > 254.5f / 255.0f) {
    return src; // No alpha. Perform a plain copy.
  }
  auto out = dst;
  out.bg = blit_lerp_(out.bg, src.bg, bg_alpha);
  if (src.ch == ' ') {
    // Source is space, so keep the current glyph.
    out.fg = blit_lerp_(out.fg, src.bg, bg_alpha);
  } else if (out.ch == ' ') {
    // Destination is space, so use the glyph from source.
    out.copyChar(src);
    out.fg = blit_lerp_(out.bg, src.fg, fg_alpha);
  } else if (out.sameChar(src)) {
    out.fg = blit_lerp_(out.fg, src.fg, fg_alpha);
  } else {
    /* Pick the glyph based on foreground_alpha. */
    if (fg_alpha < 0.5f) {
      out.fg = blit_lerp_(out.fg, out.bg, fg_alpha * 2);
    } else {
      out.copyChar(src);
      out.fg = blit_lerp_(out.bg, src.fg, (fg_alpha - 0.5f) * 2);
    }
  }
  return out;
}

void Console::blit(const Console &source, const std::array<int, 2> &dest_xy,
                   std::array<int, 4> source_rect, float foreground_alpha,
                   float background_alpha) {
  auto wSrc = source_rect[2] == 0 ? source.w : source_rect[2];
  assert(wSrc > 0);
  auto hSrc = source_rect[3] == 0 ? source.h : source_rect[3];
  assert(hSrc > 0);
  assert(source_rect[0] + wSrc >= 0);
  assert(source_rect[1] + hSrc >= 0);
  assert(source_rect[0] < w);
  assert(source_rect[1] < h);

  auto minX = std::max(source_rect[0], source_rect[0] - dest_xy[0]);
  auto maxX = std::min(std::min(source_rect[0] + wSrc, source.w),
                       w + source_rect[0] - dest_xy[0]);
  auto minY = std::max(source_rect[1], source_rect[1] - dest_xy[1]);
  auto maxY = std::min(std::min(source_rect[1] + hSrc, source.h),
                       h + source_rect[1] - dest_xy[1]);

  for (int cx = minX; cx < maxX; ++cx) {
    assert(0 <= cx && cx < source.w);
    int dx = cx - source_rect[0] + dest_xy[0];
    assert(0 <= dx && dx < w);
    for (int cy = minY; cy < maxY; ++cy) {
      assert(0 <= cy && cy < source.h);
      int dy = cy - source_rect[1] + dest_xy[1];
      assert(0 <= dy && dy < h);

      at({dx, dy}) = console_blit_cell_(
          source.at({cx, cy}), at({dx, dy}), foreground_alpha, background_alpha,
          (key_color ? &key_color.value() : NULL));
    }
  }
}

void Console::put_rgba(int x, int y, int ch, std::optional<color::RGBA> fg,
                       std::optional<color::RGBA> bg) {
  assert(0 <= x);
  assert(x < w);
  assert(0 <= y);
  assert(y < h);

  auto &tile = at({x, y});
  if (ch > 0) {
    tile.encodeChar(ch);
  }
  if (fg) {
    tile.fg = *fg;
  }
  if (bg) {
    tile.bg = *bg;
  }
}
