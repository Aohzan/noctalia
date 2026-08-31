#pragma once

#include "ui/palette.h"

#include <optional>
#include <string>
#include <string_view>

namespace custom_button {

  struct DynamicContent {
    std::string text;
    std::string icon;
    std::string tooltip;
    std::optional<ColorSpec> color;
    std::optional<ColorSpec> iconColor;
    std::optional<ColorSpec> textColor;
  };

  // Parses `output` as a JSON object with optional string fields "text", "icon",
  // "tooltip", "color", "icon_color", "iconColor", "text_color", "textColor".
  // "color" overrides both icon and text color; "icon_color"/"iconColor" and
  // "text_color"/"textColor" override only the respective element. Returns
  // nullopt when `output` is not a JSON object; a present-but-wrong-typed field
  // is treated as absent rather than failing the whole parse.
  [[nodiscard]] std::optional<DynamicContent> parseDynamicContent(std::string_view output);

  // Resolves a palette role token (see kColorRoleTokens in ui/palette.h) or a
  // "#rrggbb"/"#rrggbbaa" hex string. Returns nullopt for anything else,
  // including "none" — callers treat that the same as "no override".
  [[nodiscard]] std::optional<ColorSpec> parseColorToken(std::string_view token);

} // namespace custom_button
