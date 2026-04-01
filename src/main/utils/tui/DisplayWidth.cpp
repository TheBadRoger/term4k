#include "utils/tui/DisplayWidth.h"

#include <cstdint>

namespace tui {

namespace {

bool decodeNext(const std::string &text, std::size_t &offset, std::uint32_t &outCodepoint) {
    if (offset >= text.size()) return false;

    const unsigned char b0 = static_cast<unsigned char>(text[offset]);
    if ((b0 & 0x80U) == 0U) {
        outCodepoint = b0;
        ++offset;
        return true;
    }

    if ((b0 & 0xE0U) == 0xC0U && offset + 1 < text.size()) {
        const unsigned char b1 = static_cast<unsigned char>(text[offset + 1]);
        if ((b1 & 0xC0U) != 0x80U) return false;
        outCodepoint = ((b0 & 0x1FU) << 6U) | (b1 & 0x3FU);
        offset += 2;
        return true;
    }

    if ((b0 & 0xF0U) == 0xE0U && offset + 2 < text.size()) {
        const unsigned char b1 = static_cast<unsigned char>(text[offset + 1]);
        const unsigned char b2 = static_cast<unsigned char>(text[offset + 2]);
        if ((b1 & 0xC0U) != 0x80U || (b2 & 0xC0U) != 0x80U) return false;
        outCodepoint = ((b0 & 0x0FU) << 12U) | ((b1 & 0x3FU) << 6U) | (b2 & 0x3FU);
        offset += 3;
        return true;
    }

    if ((b0 & 0xF8U) == 0xF0U && offset + 3 < text.size()) {
        const unsigned char b1 = static_cast<unsigned char>(text[offset + 1]);
        const unsigned char b2 = static_cast<unsigned char>(text[offset + 2]);
        const unsigned char b3 = static_cast<unsigned char>(text[offset + 3]);
        if ((b1 & 0xC0U) != 0x80U || (b2 & 0xC0U) != 0x80U || (b3 & 0xC0U) != 0x80U) return false;
        outCodepoint = ((b0 & 0x07U) << 18U) | ((b1 & 0x3FU) << 12U) | ((b2 & 0x3FU) << 6U) | (b3 & 0x3FU);
        offset += 4;
        return true;
    }

    return false;
}

bool isCombining(const std::uint32_t cp) {
    return (cp >= 0x0300U && cp <= 0x036FU) || (cp >= 0x1AB0U && cp <= 0x1AFFU) ||
           (cp >= 0x1DC0U && cp <= 0x1DFFU) || (cp >= 0x20D0U && cp <= 0x20FFU) ||
           (cp >= 0xFE20U && cp <= 0xFE2FU);
}

bool isWide(const std::uint32_t cp) {
    return (cp >= 0x1100U && cp <= 0x115FU) || (cp >= 0x2E80U && cp <= 0xA4CFU) ||
           (cp >= 0xAC00U && cp <= 0xD7A3U) || (cp >= 0xF900U && cp <= 0xFAFFU) ||
           (cp >= 0xFE10U && cp <= 0xFE19U) || (cp >= 0xFE30U && cp <= 0xFE6FU) ||
           (cp >= 0xFF00U && cp <= 0xFF60U) || (cp >= 0xFFE0U && cp <= 0xFFE6U);
}

std::size_t codepointWidth(const std::uint32_t cp) {
    if (cp == 0U) return 0;
    if (cp < 32U || (cp >= 0x7FU && cp < 0xA0U)) return 0;
    if (isCombining(cp)) return 0;
    return isWide(cp) ? 2U : 1U;
}

} // namespace

std::size_t displayWidth(const std::string &text) {
    std::size_t width = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        const std::size_t prev = i;
        std::uint32_t cp = 0;
        if (!decodeNext(text, i, cp)) {
            ++i;
            continue;
        }
        width += codepointWidth(cp);
        if (i <= prev) ++i;
    }
    return width;
}

std::string clipToDisplayWidth(const std::string &text, const std::size_t maxWidth) {
    if (maxWidth == 0) return "";

    std::size_t width = 0;
    std::size_t i = 0;
    std::size_t endByte = 0;

    while (i < text.size()) {
        const std::size_t startByte = i;
        std::uint32_t cp = 0;
        if (!decodeNext(text, i, cp)) {
            ++i;
            continue;
        }

        const std::size_t w = codepointWidth(cp);
        if (width + w > maxWidth) break;
        width += w;
        endByte = i;

        if (i <= startByte) ++i;
    }

    return text.substr(0, endByte);
}

} // namespace tui

