#include "AboutComponent.hpp"

Element AboutComponent::Render() {
  auto document = vbox({
    hbox({
      text(L"Website                    ") | bold,
      separator(),
      text(L"https://github.com/jmarrec/EnergyPlus-Cpp-Demo"),
    }),
    separator(),
    hbox({
      text(L"Skipping Lines faster      ") | bold,
      separator(),
      text(L"Use TAB / SHIFT+TAB to skip lines (10 TABs = all)"),
    }),
  });

  if (Focused()) {
    document = document | inverted;
  }

  return document | border;
}
