#include "render/core/color.h"
#include "shell/bar/widgets/custom_button_data.h"

#include <print>

namespace {

  bool expect(bool condition, const char* message) {
    if (!condition) {
      std::println(stderr, "custom_button_data_test: FAIL: {}", message);
    }
    return condition;
  }

} // namespace

int main() {
  bool ok = true;

  {
    const auto parsed =
        custom_button::parseDynamicContent(R"({"text":"42%","icon":"cpu","tooltip":"CPU load","color":"primary"})");
    ok = expect(parsed.has_value(), "full JSON payload should parse") && ok;
    if (parsed.has_value()) {
      ok = expect(parsed->text == "42%", "text field should be extracted") && ok;
      ok = expect(parsed->icon == "cpu", "icon field should be extracted") && ok;
      ok = expect(parsed->tooltip == "CPU load", "tooltip field should be extracted") && ok;
      ok = expect(parsed->color == colorSpecFromRole(ColorRole::Primary), "color role token should resolve to the role")
          && ok;
    }
  }

  {
    const auto parsed = custom_button::parseDynamicContent(R"({"text":"only text"})");
    ok = expect(parsed.has_value(), "partial JSON payload should parse") && ok;
    if (parsed.has_value()) {
      ok = expect(parsed->text == "only text", "text field should be extracted") && ok;
      ok = expect(parsed->icon.empty(), "missing icon field should be empty") && ok;
      ok = expect(parsed->tooltip.empty(), "missing tooltip field should be empty") && ok;
      ok = expect(!parsed->color.has_value(), "missing color field should have no override") && ok;
    }
  }

  {
    const auto parsed = custom_button::parseDynamicContent(R"({"text":5})");
    ok = expect(parsed.has_value(), "JSON object with a wrong-typed field should still parse") && ok;
    if (parsed.has_value()) {
      ok = expect(parsed->text.empty(), "wrong-typed field should be treated as absent") && ok;
    }
  }

  {
    const auto parsed =
        custom_button::parseDynamicContent(R"({"icon":"lock","iconColor":"secondary","textColor":"#ff0000"})");
    ok = expect(parsed.has_value(), "iconColor/textColor JSON payload should parse") && ok;
    if (parsed.has_value()) {
      ok = expect(parsed->icon == "lock", "icon field should be extracted") && ok;
      ok = expect(
               parsed->iconColor == colorSpecFromRole(ColorRole::Secondary),
               "iconColor role token should resolve to the role"
           )
          && ok;
      ok = expect(parsed->textColor.has_value(), "textColor hex should resolve") && ok;
      ok = expect(!parsed->color.has_value(), "color field should have no override when absent") && ok;
    }
  }

  {
    const auto parsed = custom_button::parseDynamicContent(R"({"icon_color":"error","text_color":"primary"})");
    ok = expect(parsed.has_value(), "snake_case icon_color/text_color should parse") && ok;
    if (parsed.has_value()) {
      ok = expect(
               parsed->iconColor == colorSpecFromRole(ColorRole::Error),
               "icon_color role token should resolve to the role"
           )
          && ok;
      ok = expect(
               parsed->textColor == colorSpecFromRole(ColorRole::Primary),
               "text_color role token should resolve to the role"
           )
          && ok;
    }
  }

  ok = expect(!custom_button::parseDynamicContent("not json").has_value(), "non-JSON input should fail to parse") && ok;
  ok = expect(
           !custom_button::parseDynamicContent(R"(["array", "not", "object"])").has_value(),
           "a JSON array should not parse as an object payload"
       )
      && ok;
  ok = expect(!custom_button::parseDynamicContent("").has_value(), "empty input should fail to parse") && ok;

  {
    const auto color = custom_button::parseColorToken("#ff0000");
    ok = expect(color.has_value(), "hex color should parse") && ok;
    if (color.has_value()) {
      Color expected;
      ok = expect(tryParseHexColor("#ff0000", expected), "reference hex parse should succeed") && ok;
      ok = expect(*color == fixedColorSpec(expected), "hex color should resolve to a fixed ColorSpec") && ok;
    }
  }

  ok = expect(custom_button::parseColorToken("on_surface_variant").has_value(), "palette role token should resolve")
      && ok;
  ok = expect(!custom_button::parseColorToken("none").has_value(), "\"none\" should resolve to no override") && ok;
  ok = expect(!custom_button::parseColorToken("not-a-color").has_value(), "unknown token should resolve to no override")
      && ok;

  return ok ? 0 : 1;
}
