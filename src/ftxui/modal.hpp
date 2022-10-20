// FTXUI 3.0.0 does not include Modal yet, it's only on master, so bring it here

#ifndef FTXUI_MODAL_HPP
#define FTXUI_MODAL_HPP

#include <ftxui/component/component.hpp>  // For Component, ComponentDecorator

namespace ftxui {
Component Modal(Component main, Component modal, const bool* show_modal);
ComponentDecorator Modal(Component modal, const bool* show_modal);
}  // namespace ftxui

#endif  // FTXUI_MODAL_HPP
