#include "pch.h"
#include "MacroAction.h"

#include <objbase.h>
#include <iomanip>
#include <sstream>

using winrt::Windows::Data::Json::JsonArray;
using winrt::Windows::Data::Json::JsonObject;

std::string GenerateGuidString() {
  GUID guid{};
  if (CoCreateGuid(&guid) != S_OK) {
    return "00000000-0000-0000-0000-000000000000";
  }
  wchar_t buffer[64]{};
  StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer)));
  std::wstring value(buffer);
  if (value.size() > 2 && value.front() == L'{' && value.back() == L'}') {
    value = value.substr(1, value.size() - 2);
  }
  return winrt::to_string(value);
}

std::string KindToString(ActionKind kind) {
  switch (kind) {
    case ActionKind::LeftClick:
      return "leftClick";
    case ActionKind::RightClick:
      return "rightClick";
    case ActionKind::OtherClick:
      return "otherClick";
    case ActionKind::Wait:
      return "wait";
  }
  return "wait";
}

ActionKind KindFromString(const std::string& value) {
  if (value == "leftClick") {
    return ActionKind::LeftClick;
  }
  if (value == "rightClick") {
    return ActionKind::RightClick;
  }
  if (value == "otherClick") {
    return ActionKind::OtherClick;
  }
  return ActionKind::Wait;
}

std::wstring KindLabel(ActionKind kind) {
  switch (kind) {
    case ActionKind::LeftClick:
      return L"Left";
    case ActionKind::RightClick:
      return L"Right";
    case ActionKind::OtherClick:
      return L"Other";
    case ActionKind::Wait:
      return L"Wait";
  }
  return L"Wait";
}

std::wstring LocationLabel(const MacroAction& action) {
  if (action.kind == ActionKind::Wait) {
    return L"-";
  }
  std::wstringstream stream;
  stream << L"x: " << static_cast<int>(action.x) << L"  y: " << static_cast<int>(action.y);
  return stream.str();
}

std::wstring FormatDelay(double delaySeconds) {
  std::wstringstream stream;
  stream << std::fixed << std::setprecision(2) << delaySeconds << L"s";
  return stream.str();
}

std::vector<MacroAction> ParseActionsFromJson(const JsonArray& array) {
  std::vector<MacroAction> actions;
  actions.reserve(array.Size());
  for (uint32_t i = 0; i < array.Size(); ++i) {
    auto item = array.GetObjectAt(i);
    MacroAction action{};
    auto id = winrt::to_string(item.GetNamedString(L"id", L""));
    action.id = id.empty() ? GenerateGuidString() : id;
    action.delay = item.GetNamedNumber(L"delay", 0.0);
    action.x = item.GetNamedNumber(L"x", 0.0);
    action.y = item.GetNamedNumber(L"y", 0.0);
    auto kindValue = winrt::to_string(item.GetNamedString(L"kind", L"wait"));
    action.kind = KindFromString(kindValue);
    actions.push_back(action);
  }
  return actions;
}

JsonArray SerializeActionsToJson(const std::vector<MacroAction>& actions) {
  JsonArray array;
  for (const auto& action : actions) {
    JsonObject obj;
    obj.SetNamedValue(L"id", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(action.id)));
    obj.SetNamedValue(L"delay", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(action.delay));
    obj.SetNamedValue(L"x", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(action.x));
    obj.SetNamedValue(L"y", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(action.y));
    obj.SetNamedValue(L"kind", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(KindToString(action.kind))));
    array.Append(obj);
  }
  return array;
}
