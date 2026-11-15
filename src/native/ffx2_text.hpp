#pragma once
// FFX-2 game text to UTF-8. Mirrors the English path of FFX-2.exe's own
// converter (0x110760, character lookup 0x1109f0); see Docs/ffx2-addresses.md.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ffx::ffx2text {
// ffx2sjistbl_us.bin is UTF-8 text; entry i is its i-th code point.
struct Charset { std::vector<std::string> glyphs; };

// Splits a UTF-8 charset into entries. Refuses an empty or malformed table.
bool parseCharset(const std::uint8_t* data, std::size_t size, Charset& out);

// The table the game loaded (0x111120 stores it at VA 0xdeade0): the file,
// then uninitialised padding to 16 bytes, then a NUL. Keeps entries up to
// the first NUL or malformed byte, within `capacity`. Refuses fewer than
// MinimumLoadedGlyphs entries.
constexpr std::size_t MinimumLoadedGlyphs = 0x60;
bool parseLoadedCharset(const std::uint8_t* data, std::size_t capacity, Charset& out);

// Decodes at most `length` bytes, stopping at 0x00. Controls produce no text,
// except 0x03, which becomes "\n".
std::string decode(const std::uint8_t* source, std::size_t length, const Charset& charset);
}
