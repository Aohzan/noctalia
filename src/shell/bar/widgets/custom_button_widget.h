#pragma once

#include "core/timer_manager.h"
#include "shell/bar/widget.h"
#include "shell/bar/widget_custom_image.h"
#include "ui/palette.h"

#include <memory>
#include <optional>
#include <string>

class Glyph;
class Image;
class InputArea;
class Label;
namespace process {
  struct RunResult;
}

class CustomButtonWidget : public Widget {
public:
  struct Options {
    std::string glyph = "heart";
    std::string customImage;
    bool customImageColorize = false;
    std::string label;
    std::string tooltip;
    std::string dataCommand;
    double dataCommandInterval = 3.0;
  };

  explicit CustomButtonWidget(Options options);
  ~CustomButtonWidget() override;

  void create() override;

private:
  void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
  void doUpdate(Renderer& renderer) override;

  void runDataCommand();
  void handleDataCommandResult(process::RunResult result);

  // Dynamic-value-or-static-config accessors used by create()/doLayout()/doUpdate().
  [[nodiscard]] std::string_view effectiveText() const noexcept;
  [[nodiscard]] std::string_view effectiveGlyph() const noexcept;
  [[nodiscard]] std::string_view effectiveTooltip() const noexcept;
  [[nodiscard]] ColorSpec effectiveForeground() const noexcept;
  [[nodiscard]] ColorSpec effectiveIconColor() const noexcept;

  std::string m_glyphName;
  std::string m_labelText;
  std::string m_tooltip;
  WidgetCustomImage m_customImage;
  InputArea* m_area = nullptr;
  Glyph* m_glyph = nullptr;
  Image* m_image = nullptr;
  Label* m_label = nullptr;

  std::string m_dataCommand;
  double m_dataCommandIntervalSeconds = 3.0;
  Timer m_dataCommandTimer;
  bool m_dataCommandRunning = false;
  std::string m_dynamicText;
  std::string m_dynamicIcon;
  std::string m_dynamicTooltip;
  std::optional<ColorSpec> m_dynamicColor;
  std::optional<ColorSpec> m_dynamicIconColor;
  std::optional<ColorSpec> m_dynamicTextColor;
  // Guards DeferredCall-queued command results against a widget destroyed in the meantime.
  std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};
