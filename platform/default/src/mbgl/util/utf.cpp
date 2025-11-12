#include <mbgl/util/logging.hpp>
#include <mbgl/util/utf.hpp>

#include <locale>
#include <codecvt>

namespace mbgl {
namespace util {

std::u16string convertUTF8ToUTF16(const std::string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
        return conv.from_bytes(str);
    } catch (...) {
        Log::Error(Event::General, "Invalid UTF-8 sequence encountered during conversion to UTF-16:");
        for (const auto& c : str) {
            Log::Error(Event::General, std::string(" ") + c);
        }
        return u"";
    }
}

std::string convertUTF16ToUTF8(const std::u16string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
        return conv.to_bytes(str);
    } catch (...) {
        Log::Error(Event::General, "Invalid UTF-16 sequence encountered during conversion to UTF-8:");
        for (const auto& c : str) {
            Log::Error(Event::General, std::string(" ") + static_cast<char>(c));
        }
        return "";
    }
}

} // namespace util
} // namespace mbgl
