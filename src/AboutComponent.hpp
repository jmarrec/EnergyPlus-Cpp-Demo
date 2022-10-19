#ifndef ABOUT_COMPONENT_HPP
#define ABOUT_COMPONENT_HPP

#include <ftxui/component/component.hpp>

using namespace ftxui;

class AboutComponent : public ComponentBase
{
 public:
  AboutComponent() = default;
  Element Render() override;
};

#endif  // * end of include guard: UI_INFO_COMPONENT_HPP */