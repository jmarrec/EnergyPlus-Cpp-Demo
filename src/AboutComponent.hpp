#ifndef ABOUT_COMPONENT_HPP
#define ABOUT_COMPONENT_HPP

#include <ftxui/component/component_base.hpp>  // for ComponentBase
#include <ftxui/dom/elements.hpp>              // for Element

using namespace ftxui;

class AboutComponent : public ComponentBase
{
 public:
  AboutComponent() = default;
  Element Render() override;
  virtual bool Focusable() const override {
    return true;
  };
};

#endif  // * end of include guard: UI_INFO_COMPONENT_HPP */
