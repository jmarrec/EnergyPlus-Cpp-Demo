#include "AboutComponent.hpp"

Element AboutComponent::Render() {
  auto document = vbox({
    hbox({
      text(L"Website            ") | bold,
      separator(),
      text(L"https://github.com/jmarrec/EnergyPlus-Cpp-Demo"),
    }),
    separator(),
    hbox({
      text(L"Build date         ") | bold,
      separator(),
      text(L"Unspecified"),
    }),
  });

  if (Focused()) {
    document = document | inverted;
  }

  return document | border;
}
