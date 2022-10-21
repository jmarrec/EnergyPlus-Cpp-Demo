#include "AboutComponent.hpp"

#include <string>

Element AboutComponent::Render() {
  auto document = vbox({
    hbox({
      text("Website                    ") | bold,
      separator(),
      text("https://github.com/jmarrec/EnergyPlus-Cpp-Demo"),
    }),
    separator(),
    hbox({
      text("Skipping Lines faster      ") | bold,
      separator(),
      vbox({
        text("Use TAB / SHIFT+TAB to skip lines (10 TABs = all)"),
        text("PageUp/PageDown (or Fn+Up/Down on laptops) switches pages"),
        text("Home/End key can go to the start/end of the log"),
        text("The Wheel of your mouse is also usable"),
      }),
    }),
  });

  if (Focused()) {
    document = document | inverted;
  }

  return document | border;
}
