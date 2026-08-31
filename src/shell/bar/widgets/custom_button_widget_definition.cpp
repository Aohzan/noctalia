#include "shell/bar/widgets/custom_button_widget_definition.h"

#include "shell/bar/widgets/glyph_button_definition.h"

const noctalia::bar::WidgetDefinition<CustomButtonWidget::Options>& customButtonWidgetDefinition() {
  using noctalia::bar::field;
  using Options = CustomButtonWidget::Options;

  static const noctalia::bar::WidgetDefinition<Options> definition{
      .type = "custom_button",
      .fields = noctalia::bar::glyphButtonFields<Options>(
          field<&Options::label>({
              .key = "label",
          }),
          field<&Options::tooltip>({
              .key = "tooltip",
          }),
          field<&Options::dataCommand>({
              .key = "data_command",
          }),
          field<&Options::dataCommandInterval>({
              .key = "data_command_interval",
              .minValue = 0.25,
              .maxValue = 3600.0,
              .step = 0.25,
          })
      ),
      .glyph = [](const Options& options) { return options.glyph; },
  };
  return definition;
}
