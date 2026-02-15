#include "types.hpp"
#include <algorithm>
#include <locale>
#include <stdexcept>
#include <utility>

namespace TextSimilarity::Core {

namespace {
// Unicode-aware case conversion tables
constexpr std::pair<char32_t, char32_t> uppercase_to_lowercase_basic[] = {
    // ASCII uppercase to lowercase
    {'A', 'a'},
    {'B', 'b'},
    {'C', 'c'},
    {'D', 'd'},
    {'E', 'e'},
    {'F', 'f'},
    {'G', 'g'},
    {'H', 'h'},
    {'I', 'i'},
    {'J', 'j'},
    {'K', 'k'},
    {'L', 'l'},
    {'M', 'm'},
    {'N', 'n'},
    {'O', 'o'},
    {'P', 'p'},
    {'Q', 'q'},
    {'R', 'r'},
    {'S', 's'},
    {'T', 't'},
    {'U', 'u'},
    {'V', 'v'},
    {'W', 'w'},
    {'X', 'x'},
    {'Y', 'y'},
    {'Z', 'z'},

    // Latin-1 Supplement
    {0x00C0, 0x00E0},
    {0x00C1, 0x00E1},
    {0x00C2, 0x00E2},
    {0x00C3, 0x00E3},
    {0x00C4, 0x00E4},
    {0x00C5, 0x00E5},
    {0x00C6, 0x00E6},
    {0x00C7, 0x00E7},
    {0x00C8, 0x00E8},
    {0x00C9, 0x00E9},
    {0x00CA, 0x00EA},
    {0x00CB, 0x00EB},
    {0x00CC, 0x00EC},
    {0x00CD, 0x00ED},
    {0x00CE, 0x00EE},
    {0x00CF, 0x00EF},
    {0x00D0, 0x00F0},
    {0x00D1, 0x00F1},
    {0x00D2, 0x00F2},
    {0x00D3, 0x00F3},
    {0x00D4, 0x00F4},
    {0x00D5, 0x00F5},
    {0x00D6, 0x00F6},
    {0x00D8, 0x00F8},
    {0x00D9, 0x00F9},
    {0x00DA, 0x00FA},
    {0x00DB, 0x00FB},
    {0x00DC, 0x00FC},
    {0x00DD, 0x00FD},
    {0x00DE, 0x00FE},

    // Greek uppercase to lowercase
    {0x0391, 0x03B1},
    {0x0392, 0x03B2},
    {0x0393, 0x03B3},
    {0x0394, 0x03B4},
    {0x0395, 0x03B5},
    {0x0396, 0x03B6},
    {0x0397, 0x03B7},
    {0x0398, 0x03B8},
    {0x0399, 0x03B9},
    {0x039A, 0x03BA},
    {0x039B, 0x03BB},
    {0x039C, 0x03BC},
    {0x039D, 0x03BD},
    {0x039E, 0x03BE},
    {0x039F, 0x03BF},
    {0x03A0, 0x03C0},
    {0x03A1, 0x03C1},
    {0x03A3, 0x03C3},
    {0x03A4, 0x03C4},
    {0x03A5, 0x03C5},
    {0x03A6, 0x03C6},
    {0x03A7, 0x03C7},
    {0x03A8, 0x03C8},
    {0x03A9, 0x03C9},

    // Greek accented letters
    {0x0386, 0x03AC},
    {0x0388, 0x03AD},
    {0x0389, 0x03AE},
    {0x038A, 0x03AF},
    {0x038C, 0x03CC},
    {0x038E, 0x03CD},
    {0x038F, 0x03CE},

    // Cyrillic uppercase to lowercase
    {0x0410, 0x0430},
    {0x0411, 0x0431},
    {0x0412, 0x0432},
    {0x0413, 0x0433},
    {0x0414, 0x0434},
    {0x0415, 0x0435},
    {0x0416, 0x0436},
    {0x0417, 0x0437},
    {0x0418, 0x0438},
    {0x0419, 0x0439},
    {0x041A, 0x043A},
    {0x041B, 0x043B},
    {0x041C, 0x043C},
    {0x041D, 0x043D},
    {0x041E, 0x043E},
    {0x041F, 0x043F},
    {0x0420, 0x0440},
    {0x0421, 0x0441},
    {0x0422, 0x0442},
    {0x0423, 0x0443},
    {0x0424, 0x0444},
    {0x0425, 0x0445},
    {0x0426, 0x0446},
    {0x0427, 0x0447},
    {0x0428, 0x0448},
    {0x0429, 0x0449},
    {0x042A, 0x044A},
    {0x042B, 0x044B},
    {0x042C, 0x044C},
    {0x042D, 0x044D},
    {0x042E, 0x044E},
    {0x042F, 0x044F}};

char32_t unicode_tolower_optimized(char32_t c) noexcept {
  // Fast path for ASCII
  if (c >= 'A' && c <= 'Z') {
    return c + 32;
  }

  // Binary search for efficiency
  const auto *begin = std::begin(uppercase_to_lowercase_basic);
  const auto *end = std::end(uppercase_to_lowercase_basic);

  const auto *found =
      std::lower_bound(begin, end, c, [](const auto &pair, char32_t value) {
        return pair.first < value;
      });

  if (found != end && found->first == c) {
    return found->second;
  }

  // Special cases
  if (c == 0x03C2) { // Final sigma -> regular sigma
    return 0x03C3;
  }

  return c; // No conversion needed
}

char32_t unicode_toupper_optimized(char32_t c) noexcept {
  // Fast path for ASCII
  if (c >= 'a' && c <= 'z') {
    return c - 32;
  }

  // Binary search in reverse
  const auto *begin = std::begin(uppercase_to_lowercase_basic);
  const auto *end = std::end(uppercase_to_lowercase_basic);

  const auto *found = std::find_if(
      begin, end, [c](const auto &pair) { return pair.second == c; });

  if (found != end) {
    return found->first;
  }

  return c; // No conversion needed
}

// UTF-8 decoding function
std::u32string utf8_to_utf32(const std::string &s) {
  std::u32string result;
  result.reserve(s.length());

  for (size_t i = 0; i < s.length();) {
    char32_t c;
    unsigned char c1 = s[i];

    if (c1 < 0x80) {
      c = c1;
      i += 1;
    } else if (c1 < 0xE0) {
      c = ((c1 & 0x1F) << 6) | (s[i + 1] & 0x3F);
      i += 2;
    } else if (c1 < 0xF0) {
      c = ((c1 & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
      i += 3;
    } else {
      c = ((c1 & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) |
          ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
      i += 4;
    }
    result.push_back(c);
  }
  return result;
}

// UTF-8 encoding function
std::string utf32_to_utf8(const std::u32string &s) {
  std::string result;
  result.reserve(s.length() * 4);

  for (char32_t c : s) {
    if (c < 0x80) {
      result.push_back(static_cast<char>(c));
    } else if (c < 0x800) {
      result.push_back(static_cast<char>(0xC0 | (c >> 6)));
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) {
      result.push_back(static_cast<char>(0xE0 | (c >> 12)));
      result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else {
      result.push_back(static_cast<char>(0xF0 | (c >> 18)));
      result.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
  }
  return result;
}
} // namespace

// UnicodeString implementation
UnicodeString::UnicodeString(std::string utf8_string)
    : utf8_string_(std::move(utf8_string)) {
  validate_and_convert();
}

UnicodeString::UnicodeString(std::u32string unicode_string)
    : unicode_string_(std::move(unicode_string)) {
  utf8_string_ = utf32_to_utf8(unicode_string_);
}

void UnicodeString::validate_and_convert() {
  unicode_string_ = utf8_to_utf32(utf8_string_);
}

UnicodeString UnicodeString::to_lower() const {
  std::u32string result;
  result.reserve(unicode_string_.length());

  std::transform(unicode_string_.begin(), unicode_string_.end(),
                 std::back_inserter(result), unicode_tolower_optimized);

  return UnicodeString(std::move(result));
}

UnicodeString UnicodeString::to_upper() const {
  std::u32string result;
  result.reserve(unicode_string_.length());

  std::transform(unicode_string_.begin(), unicode_string_.end(),
                 std::back_inserter(result), unicode_toupper_optimized);

  return UnicodeString(std::move(result));
}

bool UnicodeString::operator==(const UnicodeString &other) const noexcept {
  return unicode_string_ == other.unicode_string_;
}

bool UnicodeString::operator!=(const UnicodeString &other) const noexcept {
  return !(*this == other);
}

} // namespace TextSimilarity::Core