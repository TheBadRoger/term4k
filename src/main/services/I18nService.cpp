#include "I18nService.h"

#include "config/AppDirs.h"
#include "utils/ErrorNotifier.h"
#include "utils/JsonUtils.h"

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// -- Singleton ---------------------------------------------------------------

I18nService &I18nService::instance() {
    static I18nService inst;
    return inst;
}

// -- I18n::load --------------------------------------------------------------

bool I18nService::load(const std::string &filePath) {
    JsonUtils json;
    if (!JsonUtils::loadFlatObjectFromFile(filePath, json)){
        ErrorNotifier::notify("I18n::load", "failed to load i18n file: " + filePath);
        return false;
    }

    // Extract locale id from filename (strip path + extension),
    // e.g. "config/i18n/zh_CN.json" -> "zh_CN".
    auto lastSlash = std::string::npos;
    {
        auto ps = filePath.rfind('/');
        auto bs = filePath.rfind('\\');
        if (ps != std::string::npos && bs != std::string::npos) lastSlash = std::max(ps, bs);
        else if (ps != std::string::npos) lastSlash = ps;
        else if (bs != std::string::npos) lastSlash = bs;
    }
    const std::string basename =
        (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
    const auto dot = basename.rfind('.');
    // If no extension is found, keep full basename as locale fallback.
    currentLocale = (dot != std::string::npos) ? basename.substr(0, dot) : basename;

    translations.clear();
    translations.insert(json.values().begin(), json.values().end());
    return true;
}

bool I18nService::ensureLocaleLoaded(const std::string &preferredLocale) {
    const std::string locale = preferredLocale.empty() ? "en_US" : preferredLocale;
    if (!translations.empty() && currentLocale == locale) return true;

    AppDirs::init();

    std::vector<std::string> candidates;
    candidates.push_back("src/resources/i18n/" + locale + ".json");
    if (locale != "en_US") candidates.push_back("src/resources/i18n/en_US.json");

    const std::string productionDir = AppDirs::localeDir();
    candidates.push_back(productionDir + locale + ".json");
    if (locale != "en_US") candidates.push_back(productionDir + "en_US.json");

    for (const std::string &path : candidates) {
        std::error_code ec;
        if (!fs::exists(path, ec) || fs::is_directory(path, ec)) continue;
        if (load(path)) return true;
    }

    return false;
}

// -- I18n::get / operator() -------------------------------------------------

std::string I18nService::get(const std::string &key) const {
    auto it = translations.find(key);
    return (it != translations.end()) ? it->second : key;
}

std::string I18nService::operator()(const std::string &key) const {
    return get(key);
}

const std::string &I18nService::locale() const {
    return currentLocale;
}