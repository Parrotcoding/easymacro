#pragma once

#include <string>
#include <vector>
#include <winrt/Windows.Data.Json.h>

enum class ActionKind {
  LeftClick,
  RightClick,
  OtherClick,
  Wait
};

struct MacroAction {
  std::string id;
  double delay = 0.0;
  double x = 0.0;
  double y = 0.0;
  ActionKind kind = ActionKind::Wait;
};

std::string GenerateGuidString();
std::string KindToString(ActionKind kind);
ActionKind KindFromString(const std::string& value);
std::wstring KindLabel(ActionKind kind);
std::wstring LocationLabel(const MacroAction& action);
std::wstring FormatDelay(double delaySeconds);

std::vector<MacroAction> ParseActionsFromJson(const winrt::Windows::Data::Json::JsonArray& array);
winrt::Windows::Data::Json::JsonArray SerializeActionsToJson(const std::vector<MacroAction>& actions);
