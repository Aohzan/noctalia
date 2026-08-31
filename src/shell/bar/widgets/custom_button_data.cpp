#include "shell/bar/widgets/custom_button_data.h"

#include "render/core/color.h"

#include <nlohmann/json.hpp>

namespace custom_button {

  namespace {

    std::string jsonStringField(const nlohmann::json& payload, const std::string& key) {
      const auto it = payload.find(key);
      if (it == payload.end() || !it->is_string()) {
        return {};
      }
      return it->get<std::string>();
    }

  } // namespace

  std::optional<ColorSpec> parseColorToken(std::string_view token) {
    if (const auto role = colorRoleFromToken(token); role.has_value()) {
      return colorSpecFromRole(*role);
    }
    Color fixed;
    if (tryParseHexColor(token, fixed)) {
      return fixedColorSpec(fixed);
    }
    return std::nullopt;
  }

  std::optional<DynamicContent> parseDynamicContent(std::string_view output) {
    const nlohmann::json payload = nlohmann::json::parse(output, nullptr, false);
    if (payload.is_discarded() || !payload.is_object()) {
      return std::nullopt;
    }

    DynamicContent content;
    content.text = jsonStringField(payload, "text");
    content.icon = jsonStringField(payload, "icon");
    content.tooltip = jsonStringField(payload, "tooltip");
    content.color = parseColorToken(jsonStringField(payload, "color"));
    content.iconColor = parseColorToken(jsonStringField(payload, "icon_color"));
    if (!content.iconColor.has_value()) {
      content.iconColor = parseColorToken(jsonStringField(payload, "iconColor"));
    }
    content.textColor = parseColorToken(jsonStringField(payload, "text_color"));
    if (!content.textColor.has_value()) {
      content.textColor = parseColorToken(jsonStringField(payload, "textColor"));
    }
    return content;
  }

} // namespace custom_button
