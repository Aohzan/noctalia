#include "shell/bar/widgets/custom_button_widget.h"

#include "core/deferred_call.h"
#include "core/log.h"
#include "core/process/process.h"
#include "render/core/renderer.h"
#include "render/scene/input_area.h"
#include "shell/bar/widgets/custom_button_data.h"
#include "ui/builders.h"
#include "ui/palette.h"
#include "ui/style.h"
#include "util/string_utils.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>

namespace {
  constexpr Logger kLog("custom-button");
  // Bounds a single data_command run independent of its polling interval.
  constexpr std::chrono::milliseconds kDataCommandTimeout{8000};
  constexpr std::size_t kDataCommandMaxOutputBytes = 16 * 1024;
} // namespace

CustomButtonWidget::CustomButtonWidget(Options options)
    : m_glyphName(StringUtils::trim(options.glyph)), m_labelText(StringUtils::trim(options.label)),
      m_tooltip(StringUtils::trim(options.tooltip)),
      m_customImage(widget_custom_image::fromConfig(options.customImage, options.customImageColorize)),
      m_dataCommand(StringUtils::trim(options.dataCommand)),
      m_dataCommandIntervalSeconds(std::max(0.25, options.dataCommandInterval)) {}

CustomButtonWidget::~CustomButtonWidget() { *m_alive = false; }

void CustomButtonWidget::create() {
  auto area = ui::inputArea({});

  if (!m_tooltip.empty()) {
    area->setTooltip(m_tooltip);
  }

  if (m_customImage.enabled()) {
    area->addChild(ui::image({.out = &m_image, .fit = ImageFit::Contain}));
  } else {
    area->addChild(
        ui::glyph({
            .out = &m_glyph,
            .glyph = m_glyphName,
            .glyphSize = Style::baseGlyphSize * m_contentScale,
            .color = widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)),
            .visible = !m_glyphName.empty(),
        })
    );
  }

  area->addChild(
      ui::label({
          .out = &m_label,
          .text = m_labelText,
          .fontSize = Style::fontSizeBody * fontScale(),
          .fontWeight = labelFontWeight(),
          .fontFamily = labelFontFamily(),
          .color = widgetForegroundOr(colorSpecFromRole(ColorRole::OnSurface)),
          .maxLines = 1,
          .visible = !m_labelText.empty(),
      })
  );

  m_area = area.get();
  setRoot(std::move(area));

  if (!m_dataCommand.empty()) {
    runDataCommand();
    m_dataCommandTimer.startRepeating(
        std::chrono::milliseconds(static_cast<std::int64_t>(m_dataCommandIntervalSeconds * 1000.0)),
        [this] { runDataCommand(); }
    );
  }
}

void CustomButtonWidget::doUpdate(Renderer&) {
  if (m_area == nullptr) {
    return;
  }
  if (m_label != nullptr) {
    m_label->setText(effectiveText());
    m_label->setColor(effectiveForeground());
  }
  if (m_glyph != nullptr) {
    m_glyph->setGlyph(effectiveGlyph());
    m_glyph->setColor(effectiveIconColor());
  }
  m_area->setTooltip(std::string(effectiveTooltip()));
}

std::string_view CustomButtonWidget::effectiveText() const noexcept {
  return m_dynamicText.empty() ? std::string_view(m_labelText) : std::string_view(m_dynamicText);
}

std::string_view CustomButtonWidget::effectiveGlyph() const noexcept {
  return m_dynamicIcon.empty() ? std::string_view(m_glyphName) : std::string_view(m_dynamicIcon);
}

std::string_view CustomButtonWidget::effectiveTooltip() const noexcept {
  return m_dynamicTooltip.empty() ? std::string_view(m_tooltip) : std::string_view(m_dynamicTooltip);
}

ColorSpec CustomButtonWidget::effectiveForeground() const noexcept {
  return m_dynamicTextColor.value_or(
      m_dynamicColor.value_or(widgetForegroundOr(colorSpecFromRole(ColorRole::OnSurface)))
  );
}

ColorSpec CustomButtonWidget::effectiveIconColor() const noexcept {
  return m_dynamicIconColor.value_or(
      m_dynamicColor.value_or(widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)))
  );
}

void CustomButtonWidget::runDataCommand() {
  if (m_dataCommand.empty() || m_dataCommandRunning) {
    return;
  }
  m_dataCommandRunning = true;

  process::RunOptions options;
  options.timeout = kDataCommandTimeout;
  options.maxOutputBytes = kDataCommandMaxOutputBytes;

  process::RunCallbacks callbacks;
  const std::weak_ptr<bool> aliveWeak = m_alive;
  callbacks.onExit = [this, aliveWeak](process::RunResult result) {
    DeferredCall::callLater([this, aliveWeak, result = std::move(result)]() mutable {
      if (aliveWeak.expired()) {
        return;
      }
      handleDataCommandResult(std::move(result));
    });
  };

  if (!process::runAsync(m_dataCommand, std::move(callbacks), options)) {
    m_dataCommandRunning = false;
  }
}

void CustomButtonWidget::handleDataCommandResult(process::RunResult result) {
  m_dataCommandRunning = false;

  const std::string trimmed = StringUtils::trim(result.out);
  if (trimmed.empty()) {
    return;
  }

  const auto parsed = custom_button::parseDynamicContent(trimmed);
  if (!parsed.has_value()) {
    kLog.warn("widget.{}: data_command output was not a JSON object", configName());
    return;
  }

  m_dynamicText = parsed->text;
  m_dynamicIcon = parsed->icon;
  m_dynamicTooltip = parsed->tooltip;
  m_dynamicColor = parsed->color;
  m_dynamicIconColor = parsed->iconColor;
  m_dynamicTextColor = parsed->textColor;
  requestUpdate();
}

void CustomButtonWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
  if (m_area == nullptr || m_label == nullptr) {
    return;
  }

  const bool isVertical = containerHeight > containerWidth;
  const bool showImage = m_image != nullptr;
  const bool showGlyph = !showImage && m_glyph != nullptr && !effectiveGlyph().empty();
  const bool showIcon = showImage || showGlyph;
  const bool showLabel = !effectiveText().empty();
  const float spacing = (showIcon && showLabel) ? Style::spaceXs * m_contentScale : 0.0F;

  if (m_glyph != nullptr) {
    m_glyph->setVisible(showGlyph);
  }
  m_label->setVisible(showLabel);

  if (showImage) {
    widget_custom_image::sync(*m_image, renderer, m_customImage, m_contentScale, effectiveIconColor());
  } else if (showGlyph) {
    m_glyph->setGlyphSize(Style::baseGlyphSize * m_contentScale);
    m_glyph->setColor(effectiveIconColor());
    m_glyph->measure(renderer);
  }

  if (showLabel) {
    m_label->setFontSize((isVertical ? Style::fontSizeCaption : Style::fontSizeBody) * fontScale());
    m_label->setFontWeight(labelFontWeight());
    m_label->setTextAlign(isVertical ? TextAlign::Center : TextAlign::Start);
    m_label->setMaxWidth(isVertical ? containerWidth : 0.0F);
    m_label->setColor(effectiveForeground());
    m_label->measure(renderer);
  }

  if (isVertical) {
    float width = 0.0F;
    float height = 0.0F;
    if (showImage) {
      width = std::max(width, m_image->width());
      height += m_image->height();
    } else if (showGlyph) {
      width = std::max(width, m_glyph->width());
      height += m_glyph->height();
    }
    if (showLabel) {
      if (height > 0.0F) {
        height += spacing;
      }
      width = std::max(width, m_label->width());
      height += m_label->height();
    }

    float y = 0.0F;
    if (showImage) {
      m_image->setPosition(std::round((width - m_image->width()) * 0.5F), y);
      y += m_image->height() + spacing;
    } else if (showGlyph) {
      m_glyph->setPosition(std::round((width - m_glyph->width()) * 0.5F), y);
      y += m_glyph->height() + spacing;
    }
    if (showLabel) {
      m_label->setPosition(std::round((width - m_label->width()) * 0.5F), y);
    }
    m_area->setSize(width, height);
    return;
  }

  float width = 0.0F;
  float height = 0.0F;
  if (showImage) {
    width += m_image->width();
    height = std::max(height, m_image->height());
  } else if (showGlyph) {
    width += m_glyph->width();
    height = std::max(height, m_glyph->height());
  }
  if (showLabel) {
    if (width > 0.0F) {
      width += spacing;
    }
    width += m_label->width();
    height = std::max(height, m_label->height());
  }

  float x = 0.0F;
  if (showImage) {
    m_image->setPosition(x, std::round((height - m_image->height()) * 0.5F));
    x += m_image->width() + spacing;
  } else if (showGlyph) {
    m_glyph->setPosition(x, std::round((height - m_glyph->height()) * 0.5F));
    x += m_glyph->width() + spacing;
  }
  if (showLabel) {
    m_label->setPosition(x, std::round((height - m_label->height()) * 0.5F));
  }
  m_area->setSize(width, height);
}
