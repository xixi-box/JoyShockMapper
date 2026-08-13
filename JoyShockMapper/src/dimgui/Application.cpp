#include "dimgui/Application.h"
#include "dimgui/Localization.h"
#include "InputHelpers.h"
#include "JSMVariable.hpp"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "implot.h"
#include "SettingsManager.h"
#include "CmdRegistry.h"
#include "InputHelpers.h"
#include "JslWrapper.h"
#include <regex>
#include <span>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <atomic>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#define ImTextureID SDL_Texture

extern CmdRegistry commandRegistry;
extern vector<JSMButton> mappings;

using namespace magic_enum;
using namespace ImGui;
#define EndMenu() ImGui::EndMenu(); // Resolve symbol conflict with windows

AppIf* Application::BindingTab::_app = nullptr;

namespace
{
std::atomic_bool g_showMainWindowRequested = false;
}

void RequestShowMainWindow()
{
	g_showMainWindowRequested.store(true, std::memory_order_release);
}

namespace
{
const char* SettingDisplayName(SettingID setting)
{
	if (!g_chinese)
		return enum_name(setting).data();
	switch (setting)
	{
	case SettingID::ZL_MODE: return Zh(u8"左扳机模式");
	case SettingID::ZR_MODE: return Zh(u8"右扳机模式");
	case SettingID::LEFT_STICK_MODE: return Zh(u8"左摇杆模式");
	case SettingID::RIGHT_STICK_MODE: return Zh(u8"右摇杆模式");
	case SettingID::MOTION_STICK_MODE: return Zh(u8"体感摇杆模式");
	case SettingID::STICK_SENS: return Zh(u8"摇杆灵敏度");
	case SettingID::FLICK_STICK_OUTPUT: return Zh(u8"甩动摇杆输出");
	case SettingID::MOUSE_RING_RADIUS: return Zh(u8"鼠标环半径");
	case SettingID::GYRO_OUTPUT: return Zh(u8"陀螺仪输出");
	case SettingID::GYRO_SPACE: return Zh(u8"陀螺仪坐标空间");
	case SettingID::MIN_GYRO_SENS: return Zh(u8"最低陀螺仪灵敏度");
	case SettingID::MAX_GYRO_SENS: return Zh(u8"最高陀螺仪灵敏度");
	case SettingID::MIN_GYRO_THRESHOLD: return Zh(u8"最低陀螺仪阈值");
	case SettingID::MAX_GYRO_THRESHOLD: return Zh(u8"最高陀螺仪阈值");
	case SettingID::GYRO_SMOOTH_THRESHOLD: return Zh(u8"陀螺仪平滑阈值");
	case SettingID::GYRO_SMOOTH_TIME: return Zh(u8"陀螺仪平滑时间");
	case SettingID::GYRO_CUTOFF_SPEED: return Zh(u8"陀螺仪截止速度");
	case SettingID::GYRO_CUTOFF_RECOVERY: return Zh(u8"陀螺仪截止恢复");
	case SettingID::MOUSE_X_FROM_GYRO_AXIS: return Zh(u8"鼠标 X 轴来源");
	case SettingID::MOUSE_Y_FROM_GYRO_AXIS: return Zh(u8"鼠标 Y 轴来源");
	case SettingID::JOYCON_GYRO_MASK: return Zh(u8"Joy-Con 陀螺仪范围");
	case SettingID::HOLD_PRESS_TIME: return Zh(u8"长按判定时间");
	case SettingID::TURBO_PERIOD: return Zh(u8"连发间隔");
	case SettingID::DBL_PRESS_WINDOW: return Zh(u8"双击判定窗口");
	case SettingID::SIM_PRESS_WINDOW: return Zh(u8"同时按下判定窗口");
	case SettingID::LEAN_THRESHOLD: return Zh(u8"倾斜阈值");
	case SettingID::LEFT_STICK_DEADZONE_INNER: return Zh(u8"左摇杆内死区");
	case SettingID::LEFT_STICK_DEADZONE_OUTER: return Zh(u8"左摇杆外死区");
	case SettingID::RIGHT_STICK_DEADZONE_INNER: return Zh(u8"右摇杆内死区");
	case SettingID::RIGHT_STICK_DEADZONE_OUTER: return Zh(u8"右摇杆外死区");
	case SettingID::MOTION_DEADZONE_INNER: return Zh(u8"体感摇杆内死区");
	case SettingID::MOTION_DEADZONE_OUTER: return Zh(u8"体感摇杆外死区");
	case SettingID::FLICK_SNAP_MODE: return Zh(u8"甩动吸附模式");
	case SettingID::FLICK_SNAP_STRENGTH: return Zh(u8"甩动吸附强度");
	case SettingID::FLICK_DEADZONE_ANGLE: return Zh(u8"甩动死区角度");
	case SettingID::FLICK_TIME: return Zh(u8"甩动时间");
	case SettingID::FLICK_TIME_EXPONENT: return Zh(u8"甩动时间指数");
	case SettingID::VIRTUAL_STICK_CALIBRATION: return Zh(u8"虚拟摇杆校准");
	case SettingID::STICK_POWER: return Zh(u8"摇杆响应曲线");
	case SettingID::STICK_ACCELERATION_RATE: return Zh(u8"摇杆加速速率");
	case SettingID::STICK_ACCELERATION_CAP: return Zh(u8"摇杆加速上限");
	case SettingID::SCREEN_RESOLUTION_X: return Zh(u8"屏幕水平分辨率");
	case SettingID::SCREEN_RESOLUTION_Y: return Zh(u8"屏幕垂直分辨率");
	case SettingID::SCROLL_SENS: return Zh(u8"滚动灵敏度");
	case SettingID::LEFT_STICK_UNDEADZONE_INNER: return Zh(u8"左摇杆内反死区");
	case SettingID::LEFT_STICK_UNDEADZONE_OUTER: return Zh(u8"左摇杆外反死区");
	case SettingID::RIGHT_STICK_UNDEADZONE_INNER: return Zh(u8"右摇杆内反死区");
	case SettingID::RIGHT_STICK_UNDEADZONE_OUTER: return Zh(u8"右摇杆外反死区");
	case SettingID::LEFT_STICK_UNPOWER: return Zh(u8"左摇杆反响应曲线");
	case SettingID::RIGHT_STICK_UNPOWER: return Zh(u8"右摇杆反响应曲线");
	case SettingID::ANGLE_TO_AXIS_DEADZONE_INNER: return Zh(u8"轴向转换内死区");
	case SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER: return Zh(u8"轴向转换外死区");
	case SettingID::WIND_STICK_RANGE: return Zh(u8"回正摇杆范围");
	case SettingID::WIND_STICK_POWER: return Zh(u8"回正摇杆曲线");
	case SettingID::UNWIND_RATE: return Zh(u8"回正速率");
	default: return enum_name(setting).data();
	}
}

string SettingWidgetLabel(SettingID setting, bool labeled)
{
	stringstream label;
	if (!labeled)
		label << "##" << enum_name(setting);
	else if (g_chinese)
		label << SettingDisplayName(setting) << "###" << enum_name(setting);
	else
		label << enum_name(setting);
	return label.str();
}

const char* ButtonDisplayName(ButtonID button)
{
	if (!g_chinese)
		return enum_name(button).data();

	switch (button)
	{
	case ButtonID::UP: return Zh(u8"方向键 上");
	case ButtonID::DOWN: return Zh(u8"方向键 下");
	case ButtonID::LEFT: return Zh(u8"方向键 左");
	case ButtonID::RIGHT: return Zh(u8"方向键 右");
	case ButtonID::L: return Zh(u8"左肩键 L");
	case ButtonID::ZL: return Zh(u8"左扳机 ZL");
	case ButtonID::R: return Zh(u8"右肩键 R");
	case ButtonID::ZR: return Zh(u8"右扳机 ZR");
	case ButtonID::MINUS: return Zh(u8"菜单键 -");
	case ButtonID::PLUS: return Zh(u8"菜单键 +");
	case ButtonID::HOME: return Zh(u8"主页键");
	case ButtonID::CAPTURE: return Zh(u8"截图 / 触摸板按下");
	case ButtonID::N: return Zh(u8"面键 上（N）");
	case ButtonID::E: return Zh(u8"面键 右（E）");
	case ButtonID::S: return Zh(u8"面键 下（S）");
	case ButtonID::W: return Zh(u8"面键 左（W）");
	case ButtonID::L3: return Zh(u8"左摇杆按下 L3");
	case ButtonID::R3: return Zh(u8"右摇杆按下 R3");
	case ButtonID::LSL: return Zh(u8"左 Joy-Con SL");
	case ButtonID::LSR: return Zh(u8"左 Joy-Con SR");
	case ButtonID::RSL: return Zh(u8"右 Joy-Con SL");
	case ButtonID::RSR: return Zh(u8"右 Joy-Con SR");
	default: return enum_name(button).data();
	}
}

bool ButtonInProfileScope(ButtonID button, int scope)
{
	if (scope == 3)
		return button >= ButtonID::UP && button < ButtonID::SIZE;

	if (scope == 1)
	{
		switch (button)
		{
		case ButtonID::UP: case ButtonID::DOWN: case ButtonID::LEFT: case ButtonID::RIGHT:
		case ButtonID::L: case ButtonID::ZL: case ButtonID::MINUS: case ButtonID::LSL:
		case ButtonID::LSR: case ButtonID::L3: case ButtonID::LUP: case ButtonID::LDOWN:
		case ButtonID::LLEFT: case ButtonID::LRIGHT: case ButtonID::LRING: case ButtonID::ZLF:
			return true;
		default:
			return false;
		}
	}

	if (scope == 2)
	{
		switch (button)
		{
		case ButtonID::N: case ButtonID::E: case ButtonID::S: case ButtonID::W:
		case ButtonID::R: case ButtonID::ZR: case ButtonID::PLUS: case ButtonID::HOME:
		case ButtonID::CAPTURE: case ButtonID::RSL: case ButtonID::RSR: case ButtonID::R3:
		case ButtonID::RUP: case ButtonID::RDOWN: case ButtonID::RLEFT: case ButtonID::RRIGHT:
		case ButtonID::RRING: case ButtonID::ZRF:
			return true;
		default:
			return false;
		}
	}
	return false;
}

string DeviceIdentity(SDL_Gamepad* controller)
{
	SDL_GamepadType type = SDL_GetRealGamepadType(controller);
	if (type == SDL_GAMEPAD_TYPE_UNKNOWN)
		type = SDL_GetGamepadType(controller);
	stringstream identity;
	identity << enum_name(type) << '|'
	         << SDL_GetGamepadVendor(controller) << '|'
	         << SDL_GetGamepadProduct(controller) << '|';
	if (const char* serial = SDL_GetGamepadSerial(controller); serial && *serial)
		identity << serial;
	else if (const char* name = SDL_GetGamepadName(controller); name)
		identity << name;
	return identity.str();
}

uint64_t StableProfileHash(string_view value)
{
	uint64_t hash = 14695981039346656037ull;
	for (unsigned char ch : value)
	{
		hash ^= ch;
		hash *= 1099511628211ull;
	}
	return hash;
}

filesystem::path DeviceProfileDirectory()
{
	const char* localAppData = getenv("LOCALAPPDATA");
	filesystem::path root = localAppData && *localAppData ? filesystem::path(localAppData) : filesystem::temp_directory_path();
	return root / "JoyShockMapper" / "devices";
}

filesystem::path UiSettingsPath()
{
	const char* localAppData = getenv("LOCALAPPDATA");
	filesystem::path root = localAppData && *localAppData ? filesystem::path(localAppData) : filesystem::temp_directory_path();
	return root / "JoyShockMapper" / "ui.settings";
}

filesystem::path DeviceProfilePath(string_view key)
{
	stringstream filename;
	filename << "device_" << hex << setw(16) << setfill('0') << StableProfileHash(key) << ".jsmprofile";
	return DeviceProfileDirectory() / filename.str();
}

void ClearProfileMappings(int scope)
{
	for (int index = enum_integer(ButtonID::UP); index < enum_integer(ButtonID::SIZE); ++index)
	{
		const ButtonID button = enum_value<ButtonID>(index);
		if (!ButtonInProfileScope(button, scope))
			continue;
		auto& mapping = mappings[index];
		mapping.set(Mapping::NO_MAPPING);
		if (auto* doublePress = mapping.atChord(button))
		{
			doublePress->set(Mapping::NO_MAPPING);
			mapping.processChordRemoval(button, doublePress);
		}
	}
}

void LoadDeviceProfile(string_view key, int scope)
{
	const filesystem::path profilePath = DeviceProfilePath(key);
	if (!filesystem::exists(profilePath))
		return;
	ClearProfileMappings(scope);
	ifstream input(profilePath);
	string line;
	while (getline(input, line))
	{
		if (line.empty() || line.front() == '#')
			continue;
		const size_t separator = line.find('\t');
		if (separator == string::npos)
			continue;
		const string inputName = line.substr(0, separator);
		const string command = line.substr(separator + 1);
		const size_t comma = inputName.find(',');
		const string buttonName = inputName.substr(0, comma);
		auto parsedButton = enum_cast<ButtonID>(buttonName);
		if (!parsedButton || !ButtonInProfileScope(*parsedButton, scope) || command.empty())
			continue;
		auto& mapping = mappings[enum_integer(*parsedButton)];
		if (comma == string::npos)
			mapping.set(Mapping(command));
		else if (inputName.substr(comma + 1) == buttonName)
			mapping.createChord(*parsedButton).set(Mapping(command));
	}
}

void SaveDeviceProfile(string_view key, int scope)
{
	if (key.empty() || scope == 0)
		return;
	const filesystem::path directory = DeviceProfileDirectory();
	error_code error;
	filesystem::create_directories(directory, error);
	if (error)
		return;
	const filesystem::path destination = DeviceProfilePath(key);
	filesystem::path temporary = destination;
	temporary += ".tmp";
	ofstream output(temporary, ios::trunc);
	if (!output)
		return;
	output << "# JoyShockMapper automatic device profile\n";
	for (int index = enum_integer(ButtonID::UP); index < enum_integer(ButtonID::SIZE); ++index)
	{
		const ButtonID button = enum_value<ButtonID>(index);
		if (!ButtonInProfileScope(button, scope))
			continue;
		const auto& mapping = mappings[index];
		if (mapping.value().isValid())
			output << enum_name(button) << '\t' << mapping.value().command() << '\n';
		if (const auto* doublePress = mapping.getDblPressMap(); doublePress && doublePress->second.value().isValid())
			output << enum_name(button) << ',' << enum_name(button) << '\t' << doublePress->second.value().command() << '\n';
	}
	output.flush();
	output.close();
	filesystem::remove(destination, error);
	error.clear();
	filesystem::rename(temporary, destination, error);
	if (error)
		filesystem::remove(temporary, error);
}

constexpr ImVec4 SURFACE = { 0.055f, 0.067f, 0.082f, 1.00f };
constexpr ImVec4 PANEL = { 0.082f, 0.098f, 0.118f, 1.00f };
constexpr ImVec4 PANEL_RAISED = { 0.118f, 0.141f, 0.165f, 1.00f };
constexpr ImVec4 BORDER = { 0.180f, 0.220f, 0.255f, 0.90f };
constexpr ImVec4 TEXT = { 0.925f, 0.945f, 0.960f, 1.00f };
constexpr ImVec4 TEXT_MUTED = { 0.520f, 0.590f, 0.640f, 1.00f };
constexpr ImVec4 ACCENT = { 0.080f, 0.690f, 0.850f, 1.00f };
constexpr ImVec4 ACCENT_HOVER = { 0.090f, 0.350f, 0.430f, 1.00f };
constexpr ImVec4 ACCENT_ACTIVE = { 0.070f, 0.520f, 0.650f, 1.00f };
void ApplyControllerTheme()
{
	auto& style = ImGui::GetStyle();
	style.WindowPadding = { 16.f, 16.f };
	style.FramePadding = { 11.f, 7.f };
	style.CellPadding = { 8.f, 7.f };
	style.ItemSpacing = { 10.f, 9.f };
	style.ItemInnerSpacing = { 7.f, 5.f };
	style.ScrollbarSize = 12.f;
	style.WindowRounding = 7.f;
	style.ChildRounding = 7.f;
	style.FrameRounding = 4.f;
	style.PopupRounding = 6.f;
	style.ScrollbarRounding = 12.f;
	style.GrabRounding = 8.f;
	style.TabRounding = 9.f;
	style.WindowBorderSize = 1.f;
	style.ChildBorderSize = 1.f;
	style.PopupBorderSize = 1.f;
	style.FrameBorderSize = 0.f;
	style.TabBorderSize = 0.f;

	auto* colors = style.Colors;
	colors[ImGuiCol_Text] = TEXT;
	colors[ImGuiCol_TextDisabled] = TEXT_MUTED;
	colors[ImGuiCol_WindowBg] = SURFACE;
	colors[ImGuiCol_ChildBg] = PANEL;
	colors[ImGuiCol_PopupBg] = { 0.070f, 0.082f, 0.098f, 0.99f };
	colors[ImGuiCol_Border] = BORDER;
	colors[ImGuiCol_BorderShadow] = { 0.f, 0.f, 0.f, 0.f };
	colors[ImGuiCol_FrameBg] = PANEL_RAISED;
	colors[ImGuiCol_FrameBgHovered] = ACCENT_HOVER;
	colors[ImGuiCol_FrameBgActive] = ACCENT_ACTIVE;
	colors[ImGuiCol_TitleBg] = PANEL_RAISED;
	colors[ImGuiCol_TitleBgActive] = PANEL;
	colors[ImGuiCol_TitleBgCollapsed] = SURFACE;
	colors[ImGuiCol_MenuBarBg] = { 0.040f, 0.050f, 0.062f, 1.00f };
	colors[ImGuiCol_ScrollbarBg] = SURFACE;
	colors[ImGuiCol_ScrollbarGrab] = BORDER;
	colors[ImGuiCol_ScrollbarGrabHovered] = TEXT_MUTED;
	colors[ImGuiCol_ScrollbarGrabActive] = ACCENT_ACTIVE;
	colors[ImGuiCol_CheckMark] = ACCENT;
	colors[ImGuiCol_SliderGrab] = ACCENT;
	colors[ImGuiCol_SliderGrabActive] = { 0.000f, 0.380f, 0.860f, 1.00f };
	colors[ImGuiCol_Button] = PANEL_RAISED;
	colors[ImGuiCol_ButtonHovered] = ACCENT_HOVER;
	colors[ImGuiCol_ButtonActive] = ACCENT_ACTIVE;
	colors[ImGuiCol_Header] = { 0.070f, 0.250f, 0.310f, 1.00f };
	colors[ImGuiCol_HeaderHovered] = ACCENT_HOVER;
	colors[ImGuiCol_HeaderActive] = ACCENT_ACTIVE;
	colors[ImGuiCol_Separator] = BORDER;
	colors[ImGuiCol_SeparatorHovered] = ACCENT_HOVER;
	colors[ImGuiCol_SeparatorActive] = ACCENT;
	colors[ImGuiCol_ResizeGrip] = { 0.000f, 0.478f, 1.000f, 0.18f };
	colors[ImGuiCol_ResizeGripHovered] = { 0.000f, 0.478f, 1.000f, 0.55f };
	colors[ImGuiCol_ResizeGripActive] = ACCENT;
	colors[ImGuiCol_Tab] = PANEL;
	colors[ImGuiCol_TabHovered] = ACCENT_HOVER;
	colors[ImGuiCol_TabSelected] = PANEL;
	colors[ImGuiCol_TabSelectedOverline] = ACCENT;
	colors[ImGuiCol_TabDimmed] = SURFACE;
	colors[ImGuiCol_TabDimmedSelected] = PANEL_RAISED;
	colors[ImGuiCol_DockingPreview] = { 0.000f, 0.478f, 1.000f, 0.35f };
	colors[ImGuiCol_DockingEmptyBg] = SURFACE;
	colors[ImGuiCol_TableHeaderBg] = PANEL_RAISED;
	colors[ImGuiCol_TableBorderStrong] = BORDER;
	colors[ImGuiCol_TableBorderLight] = { 0.792f, 0.804f, 0.831f, 0.45f };
	colors[ImGuiCol_NavHighlight] = ACCENT;
}

void DrawStatusBadge(const char* label, bool active)
{
	ImGui::PushStyleColor(ImGuiCol_Button, active ? ImVec4{ 0.050f, 0.300f, 0.235f, 1.f } : PANEL_RAISED);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? ACCENT_HOVER : PANEL_RAISED);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, active ? ACCENT_HOVER : PANEL_RAISED);
	ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4{ 0.180f, 0.900f, 0.650f, 1.f } : TEXT_MUTED);
	ImGui::SmallButton(label);
	ImGui::PopStyleColor(4);
}

void DrawLanguageSwitch()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.f, 0.f });
	const bool englishSelected = !g_chinese;
	if (englishSelected)
		ImGui::PushStyleColor(ImGuiCol_Button, ACCENT_ACTIVE);
	if (ImGui::SmallButton("EN##language"))
		g_chinese = false;
	if (englishSelected)
		ImGui::PopStyleColor();
	ImGui::SameLine();
	const bool chineseSelected = g_chinese;
	if (chineseSelected)
		ImGui::PushStyleColor(ImGuiCol_Button, ACCENT_ACTIVE);
	if (ImGui::SmallButton(Zh(u8"中文##language")))
		g_chinese = true;
	if (chineseSelected)
		ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
}

ImVec2 operator+(ImVec2 lhs, ImVec2 rhs)
{
	return ImVec2{ lhs.x + rhs.x, lhs.y + rhs.y };
}

ButtonID operator+(ButtonID lhs, int rhs)
{
	auto newEnum = enum_cast<ButtonID>(enum_integer(lhs) + rhs);
	return newEnum ? *newEnum : ButtonID::INVALID;
}

Application::Application(JslWrapper* jsl)
  : _jsl(jsl)
{
	BindingTab::_app = this;
	_tabs.try_emplace(ButtonID::NONE, "Base Layer", jsl);
}

void Application::HelpMarker(string_view cmd)
{
	SameLine();
	TextDisabled("(?)");
	if (IsItemHovered())
	{
		BeginTooltip();
		PushTextWrapPos(GetFontSize() * 35.0f);
		TextUnformatted(commandRegistry.getHelp(cmd).data());
		PopTextWrapPos();
		EndTooltip();
	}
}

template<typename T>
void Application::drawCombo(SettingID stg, ButtonID chord, ImGuiComboFlags flags, bool label)
{
	JSMVariable<T>* variable = nullptr;
	T value;
	auto setting = SettingsManager::get<T>(stg);
	if (setting)
	{
		variable = setting->atChord(chord);
		value = variable ? variable->value() : setting->value();
	}
	else
	{
		variable = SettingsManager::getV<T>(stg);
		value = variable->value();
	}

	const string name = SettingWidgetLabel(stg, label);
	stringstream selectedEnum;
	if constexpr (is_same_v<T, ControllerScheme>)
	{
		if (g_chinese && value == ControllerScheme::NONE)
			selectedEnum << Zh(u8"无");
		else if (g_chinese && value == ControllerScheme::DS4)
			selectedEnum << "PlayStation 4";
		else
			selectedEnum << value;
	}
	else
		variable ? selectedEnum << value : selectedEnum << '[' << value << ']';
	if (BeginCombo(name.c_str(), selectedEnum.str().c_str(), flags))
	{
		for (auto [enumVal, enumStr] : enum_entries<T>())
		{
			if (enumVal == T::INVALID)
				continue;

			bool disabled = false;
			if constexpr (is_same_v<T, TriggerMode>)
			{
				if (enumVal == TriggerMode::X_LT || enumVal == TriggerMode::X_RT)
				{
					auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER)->value();
					disabled = virtual_controller == ControllerScheme::NONE;
					if (virtual_controller == ControllerScheme::DS4)
					{
						enumStr = enumVal == TriggerMode::X_LT ? "PS_L2" : "PS_R2";
					}
				}
			}
			if constexpr (is_same_v<T, StickMode>)
			{
				auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER)->value();
				if (enumVal >= StickMode::LEFT_STICK && enumVal <= StickMode::RIGHT_STEER_X)
				{
					disabled = virtual_controller == ControllerScheme::NONE;
				}
				if (setting && setting->_id != SettingID::MOTION_STICK_MODE &&
				  (enumVal == StickMode::LEFT_STEER_X || enumVal == StickMode::RIGHT_STEER_X))
				{
					// steer stick mode is only valid for the motion stick, don't add to other combo boxes
					continue;
				}
				if (enumVal == StickMode::INNER_RING || enumVal == StickMode::OUTER_RING)
				{
					// Don't add legacy commands to UI. Use setting "XXXX_RING_MODE = [INNER|OUTER]" instead
					continue;
				}
			}
			if constexpr (is_same_v<T, GyroOutput>)
			{
				auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER)->value();
				if (enumVal == GyroOutput::PS_MOTION)
				{
					disabled = virtual_controller == ControllerScheme::DS4;
				}
			}
			if (disabled)
				BeginDisabled();
			const char* choiceLabel = enumStr.data();
			if constexpr (is_same_v<T, ControllerScheme>)
			{
				if (g_chinese && enumVal == ControllerScheme::NONE)
					choiceLabel = Zh(u8"无");
				else if (g_chinese && enumVal == ControllerScheme::DS4)
					choiceLabel = "PlayStation 4";
			}
			bool isSelected = enumVal == value;
			if (Selectable(choiceLabel, isSelected))
			{
				if (!variable)
					variable = &setting->createChord(chord);

				variable->set(enumVal);
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (isSelected)
				SetItemDefaultFocus();
			if (disabled)
				EndDisabled();
		}
		EndCombo();
	}
}

void Application::BindingTab::drawButton(ButtonID btn, ImVec2 size)
{
	std::stringstream labelID;
	auto mapping = mappings[enum_integer(btn)].atChord(_chord);
	string desc;
	if (mapping)
		desc = LocalizeMappingDescription(mapping->value().description());
	else
	{
		stringstream ss;
		ss << '[' << LocalizeMappingDescription(mappings[enum_integer(btn)].value().description()) << ']';
		desc = ss.str();
	}

	for (auto pos = desc.find("and"); pos != string::npos; pos = desc.find("and", pos))
	{
		desc.insert(pos, "\n");
		pos += 2;
	}
	labelID << desc;                    // Button label to display
	labelID << "###" << enum_name(btn); // Button ID for ImGui
	if (Button(labelID.str().data(), size))
	{
		_showPopup = btn;
	}

	string label = mapping ? string(mapping->label()) : Tr("Set a chorded button", Zh(u8"设置组合键按键"));
	if (IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayNormal))
	{
		if (!label.empty())
		{
			SetTooltip(label.data());
		}
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
		{
			_app->createChord(btn);
		}
	}
	if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
	{
		if (MenuItem(Tr("Set", Zh(u8"设置")), Tr("Left Click", Zh(u8"左键"))))
		{
			_showPopup = btn;
		}
		if (MenuItem(Tr("Clear", Zh(u8"清除")), ""))
		{
			if (mapping)
			{
				mapping->set(Mapping::NO_MAPPING);
				mappings[enum_integer(btn)].processChordRemoval(_chord, mapping);
				mappings[enum_integer(btn)].updateLabel("");
			}
		}
		if (MenuItem(Tr("Set Double Press", Zh(u8"设置双击")), "", false, false))
		{
			auto simMap = mappings[enum_integer(btn)].atSimPress(btn);
			// TODO: set simMap to some value using the input selector and create a display for it somewhere in the UI
		}
		if (MenuItem(Tr("Chord this button", Zh(u8"为此按键创建组合层")), Tr("Middle Click", Zh(u8"中键"))))
		{
			_app->createChord(btn);
		}
		if (BeginMenu(Tr("Simultaneous Press with", Zh(u8"与其他按键同时按下"))))
		{
			for (auto pair = enum_entries<ButtonID>().begin(); pair->first < ButtonID::SIZE; ++pair)
			{

				if (pair->first != btn)
				{
					if (MenuItem(pair->second.data(), nullptr, false, false))
					{
						auto simMap = mappings[enum_integer(btn)].atSimPress(pair->first);
						// TODO: set simMap to some value using the input selector and create a display for it somewhere in the UI
					}
				}
			}
			EndMenu();
		}
		drawAnyFloat(SettingID::HOLD_PRESS_TIME, true);
		drawAnyFloat(SettingID::TURBO_PERIOD, true);
		drawAnyFloat(SettingID::DBL_PRESS_WINDOW, true);
		drawAnyFloat(SettingID::SIM_PRESS_WINDOW, true);
		if (btn == ButtonID::LEAN_LEFT || btn == ButtonID::LEAN_RIGHT)
		{
			drawAnyFloat(SettingID::LEAN_THRESHOLD, true);
		}
		EndPopup();
	}
}

void Application::init()
{
	loadUiSettings();
	HideConsole();
	// Setup window
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY /*| SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_TRANSPARENT*/);
	window = SDL_CreateWindow("JoyShockMapper", 1440, 900, window_flags);

	// Setup SDL_Renderer instance
	renderer = SDL_CreateRenderer(window, nullptr); // -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (!renderer || !window)
		exit(0);
	SDL_SetWindowMinimumSize(window, 1024, 680);
	// SDL_RendererInfo info;
	// SDL_GetRendererInfo(renderer, &info);
	// SDL_Log("Current SDL_Renderer: %s", info.name);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = GetIO();
	io.IniFilename = nullptr;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	StyleColorsLight();
	ApplyControllerTheme();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	#ifdef _WIN32
	io.FontDefault = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 17.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
	#else
	io.FontDefault = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", 17.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
	#endif
	if (!io.FontDefault)
		io.FontDefault = io.Fonts->AddFontDefault();
	// io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	// ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	// IM_ASSERT(font != NULL);

	GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Add window transparency
	//MakeWindowTransparent(window, RGB(Uint8(clear_color.x * 255), Uint8(clear_color.y * 255), Uint8(clear_color.z * 255)), Uint8(clear_color.w * 255));

	// int (SDLCALL * SDL_EventFilter) (void *userdata, SDL_Event * event);
	SDL_SetEventFilter([](void* userdata, SDL_Event* evt) -> bool
	  { return evt->type >= SDL_EVENT_JOYSTICK_AXIS_MOTION && evt->type <= SDL_EVENT_GAMEPAD_SENSOR_UPDATE ? false : true; },
	  nullptr);
}

void Application::cleanUp()
{
	// Cleanup
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImPlot::DestroyContext();
	DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void Application::loadUiSettings()
{
	ifstream settings(UiSettingsPath());
	string line;
	while (getline(settings, line))
	{
		if (line == "minimize_to_tray=0") _minimizeToTray = false;
		if (line == "minimize_to_tray=1") _minimizeToTray = true;
		if (line == "start_with_windows=0") _startWithWindows = false;
		if (line == "start_with_windows=1") _startWithWindows = true;
		if (line == "fly_mouse_enabled=0") _flyMouseEnabled = false;
		if (line == "fly_mouse_enabled=1") _flyMouseEnabled = true;
		if (line.starts_with("fly_mouse_hold_button="))
		{
			const auto value = enum_cast<ButtonID>(line.substr(line.find('=') + 1));
			if (value && *value >= ButtonID::UP && *value < ButtonID::SIZE)
				_flyMouseHoldButton = *value;
		}
		if (line.starts_with("fly_mouse_sensitivity="))
		{
			try { _flyMouseSensitivity = std::clamp(stof(line.substr(line.find('=') + 1)), 0.1f, 10.f); }
			catch (...) {}
		}
	}
	if (auto gyro = SettingsManager::getV<GyroSettings>(SettingID::GYRO_ON))
	{
		auto value = gyro->value();
		value.always_off = true;
		value.ignore_mode = GyroIgnoreMode::BUTTON;
		value.button = _flyMouseHoldButton;
		gyro->set(value);
	}
	if (auto minSensitivity = SettingsManager::getV<FloatXY>(SettingID::MIN_GYRO_SENS))
		minSensitivity->set({ _flyMouseEnabled ? _flyMouseSensitivity : 0.f, _flyMouseEnabled ? _flyMouseSensitivity : 0.f });
	if (auto maxSensitivity = SettingsManager::getV<FloatXY>(SettingID::MAX_GYRO_SENS))
		maxSensitivity->set({ _flyMouseEnabled ? _flyMouseSensitivity : 0.f, _flyMouseEnabled ? _flyMouseSensitivity : 0.f });
	if (_startWithWindows && !setStartWithWindows(true))
		_settingsStatus = Tr("Could not refresh the Windows startup entry.", Zh(u8"无法更新 Windows 开机启动项。"));
}

void Application::saveUiSettings() const
{
	error_code error;
	filesystem::create_directories(UiSettingsPath().parent_path(), error);
	ofstream settings(UiSettingsPath(), ios::trunc);
	if (settings)
	{
		settings << "minimize_to_tray=" << (_minimizeToTray ? 1 : 0) << '\n';
		settings << "start_with_windows=" << (_startWithWindows ? 1 : 0) << '\n';
		settings << "fly_mouse_enabled=" << (_flyMouseEnabled ? 1 : 0) << '\n';
		settings << "fly_mouse_hold_button=" << enum_name(_flyMouseHoldButton) << '\n';
		settings << "fly_mouse_sensitivity=" << _flyMouseSensitivity << '\n';
	}
}

bool Application::setStartWithWindows(bool enabled)
{
#ifdef _WIN32
	HKEY key = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
		return false;

	const wchar_t* valueName = L"JoyShockMapper";
	LONG result = ERROR_SUCCESS;
	if (enabled)
	{
		wchar_t executable[MAX_PATH]{};
		if (GetModuleFileNameW(nullptr, executable, MAX_PATH) == 0)
		{
			RegCloseKey(key);
			return false;
		}
		wstring command = L"\"" + wstring(executable) + L"\"";
		result = RegSetValueExW(key, valueName, 0, REG_SZ,
		  reinterpret_cast<const BYTE*>(command.c_str()), static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
	}
	else
	{
		result = RegDeleteValueW(key, valueName);
		if (result == ERROR_FILE_NOT_FOUND)
			result = ERROR_SUCCESS;
	}
	RegCloseKey(key);
	return result == ERROR_SUCCESS;
#else
	return !enabled;
#endif
}

void Application::draw(std::span<SDL_Gamepad*> controllers)
{
	SDL_Gamepad* controller = controllers.empty() ? nullptr : controllers.front();
	updateDeviceProfiles(controllers);
	if (g_showMainWindowRequested.exchange(false, std::memory_order_acq_rel))
	{
		SDL_ShowWindow(window);
		SDL_RestoreWindow(window);
		SDL_RaiseWindow(window);
	}
	if (_minimizeToTray && (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0)
		SDL_HideWindow(window);
	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	SDL_Event event;
	bool done = false;
	while (!done && SDL_PollEvent(&event) != 0)
	{
		ImGui_ImplSDL3_ProcessEvent(&event);
		if (event.type == SDL_EVENT_QUIT)
			done = true;
		if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
			done = true;
		if (_minimizeToTray && event.type == SDL_EVENT_WINDOW_MINIMIZED && event.window.windowID == SDL_GetWindowID(window))
			SDL_HideWindow(window);
	}

	if (done)
		WriteToConsole("QUIT");

	// Start the Dear ImGui frame
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	NewFrame();

	DockSpaceOverViewport(ImGui::GetWindowDockID(), GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	// 1. Show the big demo window (Most of the sample code is in ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ShowDemoWindow(&show_demo_window);

	if (show_plot_demo_window)
		ImPlot::ShowDemoWindow(&show_plot_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	// static float f = 0.0f;
	using namespace ImGui;
	static bool openUsingTheGui = false;
	if (false && BeginMainMenuBar())
	{
		if (BeginMenu(Tr("File", Zh(u8"文件"))))
		{
			if (MenuItem(Tr("New", Zh(u8"新建")), "CTRL+N", nullptr, false))
			{
			}
			if (MenuItem(Tr("Open", Zh(u8"打开")), "CTRL+O", nullptr, false))
			{
			}
			if (MenuItem(Tr("Save", Zh(u8"保存")), "CTRL+S", nullptr, false))
			{
			}
			if (MenuItem(Tr("Save As...", Zh(u8"另存为…")), "SHIFT+CTRL+S", nullptr, false))
			{
			}
			Separator();
			if (MenuItem(Tr("On Startup", Zh(u8"启动配置"))))
			{
				WriteToConsole("OnStartup.txt");
			}
			if (MenuItem(Tr("On Reset", Zh(u8"重置配置"))))
			{
				WriteToConsole("OnReset.txt");
			}
			if (BeginMenu(Tr("Templates", Zh(u8"配置模板"))))
			{
				string gyroConfigsFolder{ GYRO_CONFIGS_FOLDER() };
				for (auto file : ListDirectory(gyroConfigsFolder.c_str()))
				{
					string fullPathName = ".\\GyroConfigs\\" + file;
					auto noext = file.substr(0, file.find_last_of('.'));
					if (MenuItem(noext.c_str()))
					{
						WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
						SettingsManager::getV<Switch>(SettingID::AUTOLOAD)->set(Switch::OFF);
					}
				}
				EndMenu();
			}
			if (BeginMenu(Tr("AutoLoad", Zh(u8"自动加载配置"))))
			{
				string autoloadFolder{ AUTOLOAD_FOLDER() };
				for (auto file : ListDirectory(autoloadFolder.c_str()))
				{
					string fullPathName = ".\\AutoLoad\\" + file;
					auto noext = file.substr(0, file.find_last_of('.'));
					if (MenuItem(noext.c_str()))
					{
						WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
						SettingsManager::getV<Switch>(SettingID::AUTOLOAD)->set(Switch::OFF);
					}
				}
				EndMenu();
			}
			Separator();
			if (MenuItem(Tr("Quit", Zh(u8"退出"))))
			{
				WriteToConsole("QUIT");
			}
			EndMenu();
		}
		if (BeginMenu(Tr("Commands", Zh(u8"控制"))))
		{
			if (MenuItem(Tr("Reconnect Controllers", Zh(u8"重新连接控制器"))))
			{
				WriteToConsole("RECONNECT_CONTROLLERS");
			}
			HelpMarker("RECONNECT_CONTROLLERS");
			
			if (MenuItem(Tr("Reset Mappings", Zh(u8"重置映射"))))
			{
				WriteToConsole("RESET_MAPPINGS");
			}
			HelpMarker("RESET_MAPPINGS");

			Separator();

			static float duration = 3.f;
			SliderFloat(Tr("Calibration duration", Zh(u8"校准时长")), &duration, 0.5f, 5.0f);
			if (MenuItem(Tr("Calibrate All Controllers", Zh(u8"校准全部控制器"))))
			{
				auto t = std::thread([]()
				  {
						WriteToConsole("RESTART_GYRO_CALIBRATION");
						Sleep(int32_t(duration * 1000.f)); // ms
						WriteToConsole("FINISH_GYRO_CALIBRATION"); });
				t.detach();
			}
			HelpMarker("RESTART_GYRO_CALIBRATION");

			auto autocalibrateSetting = SettingsManager::getV<Switch>(SettingID::AUTO_CALIBRATE_GYRO);
			bool value = autocalibrateSetting->value() == Switch::ON;
			if (Checkbox(Tr("Automatic gyro calibration###AUTO_CALIBRATE_GYRO", Zh(u8"自动校准陀螺仪###AUTO_CALIBRATE_GYRO")), &value))
			{
				autocalibrateSetting->set(value ? Switch::ON : Switch::OFF);
			}
			HelpMarker("AUTO_CALIBRATE_GYRO");

			if (MenuItem(Tr("Calculate Real World Calibration", Zh(u8"计算实际灵敏度校准"))))
			{
				WriteToConsole("CALCULATE_REAL_WORLD_CALIBRATION");
			}
			HelpMarker("CALCULATE_REAL_WORLD_CALIBRATION");

			if (MenuItem(Tr("Set Motion Stick Center", Zh(u8"设置体感摇杆中心"))))
			{
				WriteToConsole("SET_MOTION_STICK_NEUTRAL");
			}
			HelpMarker("SET_MOTION_STICK_NEUTRAL");

			if (MenuItem(Tr("Calibrate Adaptive Triggers", Zh(u8"校准自适应扳机"))))
			{
				WriteToConsole("CALIBRATE_TRIGGERS");
				ShowConsole();
			}
			HelpMarker("CALIBRATE_TRIGGERS");

			Separator();

			static bool whitelistAdd = false;
			if (Checkbox(Tr("Add to whitelister application", Zh(u8"加入白名单程序")), &whitelistAdd))
			{
				if (whitelistAdd)
					WriteToConsole("WHITELIST_ADD");
				else
					WriteToConsole("WHITELIST_REMOVE");
			}
			HelpMarker("WHITELIST_ADD");

			if (MenuItem(Tr("Show whitelister", Zh(u8"显示白名单工具"))))
			{
				WriteToConsole("WHITELIST_SHOW");
			}
			HelpMarker("WHITELIST_SHOW");
			EndMenu();
		}
		if (BeginMenu(Tr("System", Zh(u8"系统"))))
		{
			// TODO: HIDE_MINIMIZED
			auto tickTime = SettingsManager::getV<float>(SettingID::TICK_TIME);
			float tt = tickTime->value();
			InputFloat(Tr("Tick interval###TICK_TIME", Zh(u8"刷新间隔###TICK_TIME")), &tt, 0.f, 0.f, "%.3f");
			if (IsItemDeactivatedAfterEdit())
			{
				tickTime->set(tt);
			}
			HelpMarker(enum_name(SettingID::TICK_TIME).data());

			string dir = SettingsManager::getV<PathString>(SettingID::JSM_DIRECTORY)->value();
			dir.resize(256, '\0');
			InputText(Tr("Configuration directory###JSM_DIRECTORY", Zh(u8"配置目录###JSM_DIRECTORY")), dir.data(), dir.size(), ImGuiInputTextFlags_EnterReturnsTrue);
			if (IsItemDeactivatedAfterEdit())
			{
				dir.resize(strlen(dir.c_str()));
				SettingsManager::getV<PathString>(SettingID::JSM_DIRECTORY)->set(dir);
			}
			HelpMarker("JSM_DIRECTORY");

			drawCombo<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER, ButtonID::NONE, 0, "VIRTUAL_CONTROLLER");
			HelpMarker("VIRTUAL_CONTROLLER");

			auto rumbleEnable = SettingsManager::getV<Switch>(SettingID::RUMBLE);
			bool value = rumbleEnable->value() == Switch::ON;
			if (Checkbox(Tr("Rumble###RUMBLE", Zh(u8"震动###RUMBLE")), &value))
			{
				rumbleEnable->set(value ? Switch::ON : Switch::OFF);
			}
			HelpMarker("RUMBLE");

			auto adaptiveTriggers = SettingsManager::get<Switch>(SettingID::ADAPTIVE_TRIGGER);
			value = adaptiveTriggers->value() == Switch::ON;
			if (Checkbox(Tr("Adaptive triggers###ADAPTIVE_TRIGGER", Zh(u8"自适应扳机###ADAPTIVE_TRIGGER")), &value))
			{
				adaptiveTriggers->set(value ? Switch::ON : Switch::OFF);
			}
			HelpMarker("ADAPTIVE_TRIGGER");

			auto autoload = SettingsManager::getV<Switch>(SettingID::AUTOLOAD);
			bool al = autoload->value() == Switch::ON;
			if (Checkbox(Tr("Automatic profile loading###AUTOLOAD", Zh(u8"自动加载配置###AUTOLOAD")), &al))
			{
				autoload->set(al ? Switch::ON : Switch::OFF);
			}
			HelpMarker(enum_name(SettingID::AUTOLOAD).data());

			float rwc = *SettingsManager::get<float>(SettingID::REAL_WORLD_CALIBRATION);
			InputFloat(Tr("Real-world calibration###REAL_WORLD_CALIBRATION", Zh(u8"实际灵敏度校准###REAL_WORLD_CALIBRATION")), &rwc, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_None);
			if (IsItemDeactivatedAfterEdit())
			{
				SettingsManager::get<float>(SettingID::REAL_WORLD_CALIBRATION)->set(rwc);
			}
			HelpMarker(enum_name(SettingID::REAL_WORLD_CALIBRATION).data());

			float igs = *SettingsManager::get<float>(SettingID::IN_GAME_SENS);
			InputFloat(Tr("In-game sensitivity###IN_GAME_SENS", Zh(u8"游戏内灵敏度###IN_GAME_SENS")), &igs, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_None);
			if (IsItemDeactivatedAfterEdit())
			{
				SettingsManager::get<float>(SettingID::IN_GAME_SENS)->set(igs);
			}
			HelpMarker(enum_name(SettingID::IN_GAME_SENS).data());

			EndMenu();
		}
		if (BeginMenu(Tr("Debug", Zh(u8"调试"))))
		{
			Checkbox(Tr("Show ImGui demo", Zh(u8"显示 ImGui 演示")), &show_demo_window);
			Checkbox(Tr("Show ImPlot demo", Zh(u8"显示 ImPlot 演示")), &show_plot_demo_window);
			MenuItem(Tr("Record a bug", Zh(u8"记录问题")), nullptr, false, false);
			EndMenu();
		}
		if (BeginMenu(Tr("Help", Zh(u8"帮助"))))
		{
			if (MenuItem(Tr("Using the GUI", Zh(u8"界面使用说明"))))
			{
				openUsingTheGui = true;
			}
			MenuItem(Tr("Read Me", Zh(u8"自述文档")), nullptr, false, false);
			MenuItem(Tr("Check For Updates", Zh(u8"检查更新")), nullptr, false, false);
			MenuItem(Tr("About", Zh(u8"关于")), nullptr, false, false);
			EndMenu();
		}
		EndMainMenuBar();
	}

	if (openUsingTheGui)
	{
		OpenPopup("UsingTheGUI");
		openUsingTheGui = false;
	}
	if (BeginPopup("UsingTheGUI", ImGuiWindowFlags_Modal))
	{
		TextColored(ACCENT, "%s", Tr("Using the interface", Zh(u8"界面使用说明")));
		Separator();
		BulletText("%s", Tr("Left click to change a mapping or setting.", Zh(u8"左键单击可修改映射或设置。")));
		BulletText("%s", Tr("Right click to see related options.", Zh(u8"右键单击可查看相关选项。")));
		BulletText("%s", Tr("Middle click to open a chord layer.", Zh(u8"中键单击可打开组合键层。")));
		if (Button(Tr("Done", Zh(u8"完成"))))
		{
			CloseCurrentPopup();
		}
		EndPopup();
	}

	ImVec2 renderingAreaPos{};
	ImVec2 renderingAreaSize{};
	const ImGuiViewport* viewport = GetMainViewport();
	SetNextWindowPos(viewport->WorkPos);
	SetNextWindowSize(viewport->WorkSize);
	Begin("MainWindow", nullptr,
	  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar |
	  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

	BeginChild("ControlDeckHeader", { 0.f, 82.f }, ImGuiChildFlags_Borders);
	if (BeginTable("ControlDeckHeaderLayout", 3, ImGuiTableFlags_SizingStretchProp))
	{
		TableSetupColumn("Identity", ImGuiTableColumnFlags_WidthStretch, 1.f);
		TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 440.f);
		TableSetupColumn("Language", ImGuiTableColumnFlags_WidthFixed, 98.f);
		TableNextRow();
		TableNextColumn();
		TextColored(ACCENT, "JoyShockMapper");
		SameLine();
		TextDisabled("/ %s", Tr("Controller Studio", Zh(u8"控制器工作台")));
		TextColored(TEXT_MUTED, "%s", Tr("Tune mappings visually. The original command engine stays in control.", Zh(u8"直观调整映射，底层仍由原生命令引擎驱动。")));
		TableNextColumn();
		SetCursorPosY(GetCursorPosY() + 9.f);
		DrawStatusBadge(controller ? Tr("CONTROLLER ONLINE", Zh(u8"控制器已连接")) : Tr("NO CONTROLLER", Zh(u8"未连接控制器")), controller != nullptr);
		SameLine();
		auto autoload = SettingsManager::getV<Switch>(SettingID::AUTOLOAD);
		const bool autoloadEnabled = autoload && autoload->value() == Switch::ON;
		DrawStatusBadge(autoloadEnabled ? Tr("AUTOLOAD ON", Zh(u8"自动加载：开")) : Tr("AUTOLOAD OFF", Zh(u8"自动加载：关")), autoloadEnabled);
		SameLine();
		auto scheme = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER);
		stringstream schemeLabel;
		schemeLabel << Tr("OUTPUT ", Zh(u8"输出 "));
		if (scheme && scheme->value() != ControllerScheme::NONE)
			schemeLabel << enum_name(scheme->value());
		else
			schemeLabel << Tr("NONE", Zh(u8"无"));
		DrawStatusBadge(schemeLabel.str().c_str(), scheme && scheme->value() != ControllerScheme::NONE);
		TableNextColumn();
		SetCursorPosY(GetCursorPosY() + 9.f);
		DrawLanguageSwitch();
		EndTable();
	}
	EndChild();
	Dummy({ 0.f, 4.f });

	constexpr float navigationWidth = 214.f;
	BeginChild("SimpleNavigation", { navigationWidth, 0.f }, ImGuiChildFlags_Borders);
	TextColored(TEXT_MUTED, "%s", Tr("WORKSPACE", Zh(u8"工作区")));
	Dummy({ 0.f, 4.f });
	auto navigationButton = [this](MainPage page, const char* label)
	{
		const bool selected = _mainPage == page;
		if (selected)
		{
			PushStyleColor(ImGuiCol_Button, ACCENT_HOVER);
			PushStyleColor(ImGuiCol_Text, ACCENT);
		}
		if (Button(label, { -FLT_MIN, 42.f }))
			_mainPage = page;
		if (selected)
			PopStyleColor(2);
	};
	navigationButton(MainPage::Home, Tr("Home", Zh(u8"首页")));
	navigationButton(MainPage::Mappings, Tr("Button mappings", Zh(u8"按键映射")));
	navigationButton(MainPage::Gyro, Tr("Gyroscope", Zh(u8"陀螺仪")));
	navigationButton(MainPage::Settings, Tr("Settings", Zh(u8"设置")));

	SetCursorPosY(GetWindowHeight() - 72.f);
	TextDisabled("JoyShockMapper");
	TextDisabled("%s", Tr("Native engine active", Zh(u8"原生引擎运行中")));
	EndChild();
	SameLine();

	BeginChild("SimpleContent", { 0.f, 0.f }, ImGuiChildFlags_Borders);
	if (_mainPage == MainPage::Home)
	{
		TextColored(TEXT, "%s", controller ? Tr("Controller connected", Zh(u8"手柄已连接")) : Tr("Connect a controller", Zh(u8"请连接手柄")));
		const char* controllerName = controller ? SDL_GetGamepadName(controller) : nullptr;
		TextColored(TEXT_MUTED, "%s", controllerName ? controllerName : Tr("USB and Bluetooth controllers are detected automatically.", Zh(u8"支持自动识别 USB 和蓝牙手柄。")));
		if (controller)
		{
			int batteryPercent = -1;
			SDL_GetGamepadPowerInfo(controller, &batteryPercent);
			if (batteryPercent >= 0)
			{
				SameLine();
				TextColored(ACCENT, Tr("Battery %d%%", Zh(u8"电量 %d%%")), batteryPercent);
			}
		}
		SameLine(GetContentRegionAvail().x - 170.f);
		if (Button(Tr("Refresh devices", Zh(u8"刷新设备")), { 160.f, 0.f }))
			WriteToConsole("RECONNECT_CONTROLLERS");
		Dummy({ 0.f, 6.f });

		const float detailHeight = 154.f;
		const float canvasHeight = std::max(360.f, GetContentRegionAvail().y - detailHeight - 10.f);
		BeginChild("ControllerOverview", { 0.f, canvasHeight }, ImGuiChildFlags_Borders);
		const ImVec2 origin = GetWindowPos();
		const ImVec2 size = GetWindowSize();
		const ImVec2 center{ origin.x + size.x * 0.5f, origin.y + size.y * 0.51f };
		auto* drawList = GetWindowDrawList();
		const ImU32 controlColor = GetColorU32(ImVec4{ 0.075f, 0.090f, 0.105f, 1.f });
		const float joyConHeight = std::min(310.f, size.y * 0.70f);
		const float joyConWidth = joyConHeight * 0.34f;
		const float gap = joyConWidth * 0.48f;
		const ImVec2 leftMin{ center.x - gap * 0.5f - joyConWidth, center.y - joyConHeight * 0.5f };
		const ImVec2 leftMax{ center.x - gap * 0.5f, center.y + joyConHeight * 0.5f };
		const ImVec2 rightMin{ center.x + gap * 0.5f, center.y - joyConHeight * 0.5f };
		const ImVec2 rightMax{ center.x + gap * 0.5f + joyConWidth, center.y + joyConHeight * 0.5f };
		const ImU32 leftBlue = GetColorU32(ImVec4{ 0.00f, 0.72f, 0.90f, 1.f });
		const ImU32 rightRed = GetColorU32(ImVec4{ 1.00f, 0.24f, 0.22f, 1.f });
		const ImU32 casingEdge = GetColorU32(ImVec4{ 0.82f, 0.90f, 0.94f, 0.42f });
		const ImU32 casingShadow = GetColorU32(ImVec4{ 0.01f, 0.015f, 0.02f, 0.55f });
		const ImU32 buttonEdge = GetColorU32(ImVec4{ 0.48f, 0.52f, 0.55f, 0.72f });

		auto drawJoyConBody = [&](ImVec2 minimum, ImVec2 maximum, ImU32 color, bool left)
		{
			const float outerRadius = joyConWidth * 0.47f;
			drawList->AddRectFilled(minimum + ImVec2{ 5.f, 8.f }, maximum + ImVec2{ 8.f, 11.f }, casingShadow, outerRadius);
			drawList->AddRectFilled(minimum, maximum, color, outerRadius);
			const ImVec2 innerMin = left ? ImVec2{ maximum.x - 12.f, minimum.y } : ImVec2{ minimum.x, minimum.y };
			const ImVec2 innerMax = left ? ImVec2{ maximum.x, maximum.y } : ImVec2{ minimum.x + 12.f, maximum.y };
			drawList->AddRectFilled(innerMin, innerMax, color);
			drawList->AddRect(minimum, maximum, casingEdge, outerRadius, 0, 1.5f);

			const float railX = left ? maximum.x - 6.f : minimum.x + 6.f;
			drawList->AddRectFilled({ railX - 4.f, minimum.y + 15.f }, { railX + 4.f, maximum.y - 15.f }, GetColorU32(ImVec4{ 0.035f, 0.040f, 0.045f, 0.95f }), 2.f);
			drawList->AddLine({ railX + (left ? -2.f : 2.f), minimum.y + 28.f }, { railX + (left ? -2.f : 2.f), maximum.y - 28.f }, buttonEdge, 1.f);
			drawList->AddCircleFilled({ railX, minimum.y + 33.f }, 2.4f, buttonEdge, 12);
			drawList->AddCircleFilled({ railX, maximum.y - 33.f }, 2.4f, buttonEdge, 12);

			const float shoulderInset = joyConWidth * 0.15f;
			ImVec2 shoulderMin{ minimum.x + shoulderInset, minimum.y - 7.f };
			ImVec2 shoulderMax{ maximum.x - shoulderInset, minimum.y + 16.f };
			drawList->AddRectFilled(shoulderMin, shoulderMax, controlColor, 7.f);
			drawList->AddRect(shoulderMin, shoulderMax, buttonEdge, 7.f, 0, 1.2f);
			drawList->AddLine({ minimum.x + 13.f, minimum.y + 16.f }, { maximum.x - 13.f, minimum.y + 16.f }, casingEdge, 1.f);
		};
		drawJoyConBody(leftMin, leftMax, leftBlue, true);
		drawJoyConBody(rightMin, rightMax, rightRed, false);

		const ImVec2 leftStick{ (leftMin.x + leftMax.x) * 0.5f - 4.f, leftMin.y + joyConHeight * 0.27f };
		const ImVec2 rightStick{ (rightMin.x + rightMax.x) * 0.5f + 4.f, rightMin.y + joyConHeight * 0.68f };
		for (const ImVec2 stick : { leftStick, rightStick })
		{
			drawList->AddCircleFilled(stick + ImVec2{ 2.f, 3.f }, joyConWidth * 0.215f, casingShadow, 36);
			drawList->AddCircleFilled(stick, joyConWidth * 0.205f, GetColorU32(ImVec4{ 0.035f, 0.040f, 0.045f, 1.f }), 36);
			drawList->AddCircle(stick, joyConWidth * 0.155f, buttonEdge, 32, 2.f);
			drawList->AddCircle(stick, joyConWidth * 0.118f, GetColorU32(ImVec4{ 0.12f, 0.13f, 0.14f, 1.f }), 32, 3.f);
		}

		const ImVec2 dpad{ (leftMin.x + leftMax.x) * 0.5f - 4.f, leftMin.y + joyConHeight * 0.62f };
		const ImVec2 face{ (rightMin.x + rightMax.x) * 0.5f + 4.f, rightMin.y + joyConHeight * 0.31f };
		const float buttonOffset = joyConWidth * 0.19f;
		const std::array buttonOffsets = { ImVec2{ 0,-buttonOffset }, ImVec2{ buttonOffset,0 }, ImVec2{ 0,buttonOffset }, ImVec2{ -buttonOffset,0 } };
		for (const ImVec2 offset : buttonOffsets)
		{
			for (const ImVec2 base : { dpad, face })
			{
				drawList->AddCircleFilled(base + offset + ImVec2{ 1.2f, 1.8f }, joyConWidth * 0.102f, casingShadow, 24);
				drawList->AddCircleFilled(base + offset, joyConWidth * 0.095f, controlColor, 24);
				drawList->AddCircle(base + offset, joyConWidth * 0.095f, buttonEdge, 24, 1.f);
			}
		}
		const char* dpadLabels[] = { "▲", "▶", "▼", "◀" };
		const char* faceLabels[] = { "X", "A", "B", "Y" };
		for (size_t i = 0; i < buttonOffsets.size(); ++i)
		{
			const ImVec2 dpadSize = CalcTextSize(dpadLabels[i]);
			const ImVec2 faceSize = CalcTextSize(faceLabels[i]);
			const ImVec2 dpadText{ dpad.x + buttonOffsets[i].x - dpadSize.x * 0.5f, dpad.y + buttonOffsets[i].y - dpadSize.y * 0.5f };
			const ImVec2 faceText{ face.x + buttonOffsets[i].x - faceSize.x * 0.5f, face.y + buttonOffsets[i].y - faceSize.y * 0.5f };
			drawList->AddText(dpadText, GetColorU32(ImVec4{ 0.73f, 0.76f, 0.78f, 1.f }), dpadLabels[i]);
			drawList->AddText(faceText, GetColorU32(ImVec4{ 0.73f, 0.76f, 0.78f, 1.f }), faceLabels[i]);
		}
		drawList->AddRectFilled({ leftMax.x - joyConWidth * 0.62f, leftMin.y + 21.f }, { leftMax.x - joyConWidth * 0.32f, leftMin.y + 27.f }, controlColor, 2.f);
		drawList->AddRectFilled({ rightMin.x + joyConWidth * 0.32f, rightMin.y + 18.f }, { rightMin.x + joyConWidth * 0.62f, rightMin.y + 24.f }, controlColor, 2.f);
		drawList->AddRectFilled({ rightMin.x + joyConWidth * 0.44f, rightMin.y + 9.f }, { rightMin.x + joyConWidth * 0.50f, rightMin.y + 33.f }, controlColor, 2.f);
		const ImVec2 capture{ leftMin.x + joyConWidth * 0.50f, leftMax.y - joyConHeight * 0.12f };
		drawList->AddRectFilled({ capture.x - 7.f, capture.y - 7.f }, { capture.x + 7.f, capture.y + 7.f }, controlColor, 2.f);
		drawList->AddCircleFilled(capture, 3.5f, buttonEdge, 16);
		const ImVec2 home{ rightMin.x + joyConWidth * 0.50f, rightMax.y - joyConHeight * 0.12f };
		drawList->AddCircleFilled(home, 10.f, controlColor, 24);
		drawList->AddCircle(home, 7.f, buttonEdge, 20, 1.5f);
		drawList->AddRectFilled({ home.x - 3.5f, home.y - 1.f }, { home.x + 3.5f, home.y + 4.f }, buttonEdge, 1.f);

		if (!_leftJoyConConnected)
		{
			drawList->AddRectFilled(leftMin, leftMax, GetColorU32(ImVec4{ 0.18f, 0.19f, 0.20f, 0.78f }), joyConWidth * 0.48f);
			drawList->AddText({ leftMin.x + 19.f, center.y - 8.f }, GetColorU32(TEXT_MUTED), Tr("OFFLINE", Zh(u8"未连接")));
		}
		if (!_rightJoyConConnected)
		{
			drawList->AddRectFilled(rightMin, rightMax, GetColorU32(ImVec4{ 0.18f, 0.19f, 0.20f, 0.78f }), joyConWidth * 0.48f);
			drawList->AddText({ rightMin.x + 19.f, center.y - 8.f }, GetColorU32(TEXT_MUTED), Tr("OFFLINE", Zh(u8"未连接")));
		}

		struct Callout { ButtonID button; ImVec2 anchor; bool left; float y; };
		const std::array callouts = {
			Callout{ ButtonID::ZL, { leftMin.x + 24.f, leftMin.y + 8.f }, true, 52.f },
			Callout{ ButtonID::L, { leftMax.x - 28.f, leftMin.y + 8.f }, true, 106.f },
			Callout{ ButtonID::UP, { dpad.x, dpad.y - 28.f }, true, 172.f },
			Callout{ ButtonID::L3, leftStick, true, 238.f },
			Callout{ ButtonID::ZR, { rightMax.x - 24.f, rightMin.y + 8.f }, false, 52.f },
			Callout{ ButtonID::R, { rightMin.x + 28.f, rightMin.y + 8.f }, false, 106.f },
			Callout{ ButtonID::N, { face.x, face.y - 28.f }, false, 172.f },
			Callout{ ButtonID::R3, rightStick, false, 238.f },
		};
		const float labelWidth = std::min(235.f, size.x * 0.25f);
		for (const auto& item : callouts)
		{
			const auto& mapping = mappings[enum_integer(item.button)];
			string action = LocalizeMappingDescription(mapping.value().description());
			if (action.empty() || action == "no input")
				action = Tr("Not mapped", Zh(u8"未映射"));
			if (action.size() > 27)
				action = action.substr(0, 25) + "...";
			const float x = item.left ? 18.f : size.x - labelWidth - 18.f;
			const ImVec2 labelStart{ origin.x + x, origin.y + item.y };
			const ImVec2 lineEnd{ item.left ? labelStart.x + labelWidth : labelStart.x, labelStart.y + 19.f };
			drawList->AddLine(item.anchor, lineEnd, GetColorU32(item.button == _selectedButton ? ACCENT : BORDER), item.button == _selectedButton ? 2.5f : 1.5f);
			SetCursorScreenPos(labelStart);
			stringstream label;
			label << enum_name(item.button) << "  →  " << action << "###HomeCallout" << enum_name(item.button);
			if (Button(label.str().c_str(), { labelWidth, 38.f }))
				_selectedButton = item.button;
		}
		SetCursorScreenPos({ origin.x + size.x * 0.5f - 105.f, origin.y + size.y - 46.f });
		TextColored(controller ? ACCENT : TEXT_MUTED, "%s", controller ? Tr("DEVICE PROFILE RESTORED", Zh(u8"已恢复设备映射")) : Tr("WAITING FOR JOY-CON", Zh(u8"等待 Joy-Con 连接")));
		EndChild();

		Dummy({ 0.f, 6.f });
		BeginChild("SelectedMapping", { 0.f, detailHeight }, ImGuiChildFlags_Borders);
		TextColored(ACCENT, "%s  ·  %s", ButtonDisplayName(_selectedButton), Tr("Mapping details", Zh(u8"映射详情")));
		const auto& selectedMapping = mappings[enum_integer(_selectedButton)];
		const string singleDescription = LocalizeMappingDescription(selectedMapping.value().description());
		const auto* doubleMapping = selectedMapping.getDblPressMap();
		const string doubleDescription = doubleMapping ? LocalizeMappingDescription(doubleMapping->second.value().description()) : string();
		if (BeginTable("SelectedMappingActions", 2, ImGuiTableFlags_SizingStretchSame))
		{
			TableNextRow();
			TableNextColumn();
			TextDisabled("%s", Tr("SINGLE PRESS", Zh(u8"单击")));
			Text("%s", singleDescription.empty() ? Tr("Not mapped", Zh(u8"未映射")) : singleDescription.c_str());
			if (Button(Tr("Edit single press###HomeSingle", Zh(u8"编辑单击映射###HomeSingle")), { -FLT_MIN, 34.f }))
				editMapping(&mappings[enum_integer(_selectedButton)], enum_name(_selectedButton));
			TableNextColumn();
			TextDisabled("%s", Tr("DOUBLE PRESS", Zh(u8"双击")));
			Text("%s", doubleDescription.empty() ? Tr("Not mapped", Zh(u8"未映射")) : doubleDescription.c_str());
			if (Button(Tr("Edit double press###HomeDouble", Zh(u8"编辑双击映射###HomeDouble")), { -FLT_MIN, 34.f }))
			{
				const ButtonID button = _selectedButton;
				auto* variable = &mappings[enum_integer(button)].createChord(button);
				stringstream name;
				name << button << ',' << button;
				_inputSelector.show(variable, name.str(), [this, button, variable]()
				{
					mappings[enum_integer(button)].processChordRemoval(button, variable);
					saveActiveDeviceProfiles();
				});
			}
			EndTable();
		}
		EndChild();
	}
	else if (_mainPage == MainPage::Mappings)
	{
		TextColored(TEXT, "%s", Tr("Button mappings", Zh(u8"按键映射")));
		TextColored(TEXT_MUTED, "%s", Tr("Click Edit to assign keyboard, mouse, virtual controller or JSM commands.", Zh(u8"点击“编辑”即可映射键盘、鼠标、虚拟手柄或 JSM 命令。")));
		Dummy({ 0.f, 10.f });

		static constexpr std::array commonButtons = {
			ButtonID::UP, ButtonID::DOWN, ButtonID::LEFT, ButtonID::RIGHT,
			ButtonID::N, ButtonID::E, ButtonID::S, ButtonID::W,
			ButtonID::L, ButtonID::ZL, ButtonID::R, ButtonID::ZR,
			ButtonID::L3, ButtonID::R3, ButtonID::MINUS, ButtonID::PLUS,
			ButtonID::HOME, ButtonID::CAPTURE,
		};
		static constexpr std::array extraButtons = {
			ButtonID::LSL, ButtonID::LSR, ButtonID::RSL, ButtonID::RSR,
			ButtonID::LEAN_LEFT, ButtonID::LEAN_RIGHT, ButtonID::MIC, ButtonID::TOUCH,
			ButtonID::LUP, ButtonID::LDOWN, ButtonID::LLEFT, ButtonID::LRIGHT, ButtonID::LRING,
			ButtonID::RUP, ButtonID::RDOWN, ButtonID::RLEFT, ButtonID::RRIGHT, ButtonID::RRING,
			ButtonID::MUP, ButtonID::MDOWN, ButtonID::MLEFT, ButtonID::MRIGHT, ButtonID::MRING,
			ButtonID::ZLF, ButtonID::ZRF, ButtonID::TUP, ButtonID::TDOWN, ButtonID::TLEFT, ButtonID::TRIGHT, ButtonID::TRING,
		};

		auto drawMappingRows = [this](auto const& buttons)
		{
			for (ButtonID button : buttons)
		{
				TableNextRow();
				TableSetColumnIndex(0);
				TextUnformatted(ButtonDisplayName(button));
				TableSetColumnIndex(1);
				const auto& mapping = mappings[enum_integer(button)];
				const string description = LocalizeMappingDescription(mapping.value().description());
				TextColored(description.empty() ? TEXT_MUTED : TEXT, "%s", description.empty() ? Tr("Not mapped", Zh(u8"未映射")) : description.c_str());
				TableSetColumnIndex(2);
				const auto* doubleMapping = mapping.getDblPressMap();
				const string doubleDescription = doubleMapping ? LocalizeMappingDescription(doubleMapping->second.value().description()) : string();
				TextColored(doubleDescription.empty() ? TEXT_MUTED : TEXT, "%s", doubleDescription.empty() ? Tr("Not mapped", Zh(u8"未映射")) : doubleDescription.c_str());
				TableSetColumnIndex(3);
				stringstream ss;
				ss << Tr("Single", Zh(u8"单击")) << "###SimpleMap" << enum_name(button);
				if (Button(ss.str().c_str(), { 64.f, 0.f }))
					editMapping(&mappings[enum_integer(button)], enum_name(button));
				SameLine();
				stringstream doubleLabel;
				doubleLabel << Tr("Double", Zh(u8"双击")) << "###DoubleMap" << enum_name(button);
				if (Button(doubleLabel.str().c_str(), { 64.f, 0.f }))
				{
					auto* variable = &mappings[enum_integer(button)].createChord(button);
					stringstream name;
					name << button << ',' << button;
					_inputSelector.show(variable, name.str(), [this, button, variable]()
					{
						mappings[enum_integer(button)].processChordRemoval(button, variable);
						saveActiveDeviceProfiles();
					});
				}
			}
		};

		if (BeginTable("SimpleMappings", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
			{ 0.f, GetContentRegionAvail().y - 44.f }))
		{
			TableSetupColumn(Tr("Controller button", Zh(u8"手柄按键")), ImGuiTableColumnFlags_WidthStretch, 0.24f);
			TableSetupColumn(Tr("Single press", Zh(u8"单击映射")), ImGuiTableColumnFlags_WidthStretch, 0.32f);
			TableSetupColumn(Tr("Double press", Zh(u8"双击映射")), ImGuiTableColumnFlags_WidthStretch, 0.26f);
			TableSetupColumn(Tr("Edit", Zh(u8"编辑")), ImGuiTableColumnFlags_WidthFixed, 150.f);
			TableHeadersRow();
			drawMappingRows(commonButtons);
			TableNextRow();
			TableSetColumnIndex(0);
			TextColored(TEXT_MUTED, "%s", Tr("Less common buttons", Zh(u8"其他按键")));
			TableSetColumnIndex(1);
			TextColored(TEXT_MUTED, "%s", Tr("Joy-Con rails, lean, microphone and touch", Zh(u8"Joy-Con 侧键、倾斜、麦克风与触摸")));
			TableSetColumnIndex(2);
			TextColored(TEXT_MUTED, "%s", Tr("Optional second action", Zh(u8"可选的第二动作")));
			TableSetColumnIndex(3);
			static bool showExtraButtons = false;
			if (Button(showExtraButtons ? Tr("Hide", Zh(u8"收起")) : Tr("Show", Zh(u8"展开")), { -FLT_MIN, 0.f }))
				showExtraButtons = !showExtraButtons;
			if (showExtraButtons)
				drawMappingRows(extraButtons);
			EndTable();
		}
	}
	else if (_mainPage == MainPage::Gyro)
	{
		TextColored(TEXT, "%s", Tr("Gyroscope", Zh(u8"陀螺仪")));
		TextColored(TEXT_MUTED, "%s", Tr("Only the useful basics, explained plainly.", Zh(u8"只保留容易理解、日常会用到的基础调节。")));
		Dummy({ 0.f, 12.f });
		if (BeginTable("SimpleGyroCards", 2, ImGuiTableFlags_SizingStretchSame))
		{
			TableNextRow();
			TableNextColumn();
			BeginChild("GyroAimCard", { 0.f, 368.f }, ImGuiChildFlags_Borders);
			TextColored(ACCENT, "%s", Tr("AIMING", Zh(u8"体感瞄准")));
			auto minSensitivity = SettingsManager::getV<FloatXY>(SettingID::MIN_GYRO_SENS);
			auto maxSensitivity = SettingsManager::getV<FloatXY>(SettingID::MAX_GYRO_SENS);
			float sensitivity = maxSensitivity ? maxSensitivity->value().x() : 0.f;
			bool gyroEnabled = _flyMouseEnabled;
			if (Checkbox(Tr("Enable motion aiming###SimpleGyroEnable", Zh(u8"启用体感瞄准###SimpleGyroEnable")), &gyroEnabled))
			{
				_flyMouseEnabled = gyroEnabled;
				sensitivity = gyroEnabled ? _flyMouseSensitivity : 0.f;
				if (minSensitivity) minSensitivity->set({ sensitivity, sensitivity });
				if (maxSensitivity) maxSensitivity->set({ sensitivity, sensitivity });
				saveUiSettings();
			}
			TextColored(TEXT_MUTED, "%s", Tr("The cursor moves only while the activation button is held.", Zh(u8"只有按住启用键时，转动手柄才会移动鼠标。")));
			Dummy({ 0.f, 10.f });
			BeginDisabled(!gyroEnabled);
			static constexpr std::array flyMouseButtons = {
				ButtonID::ZL, ButtonID::ZR, ButtonID::L, ButtonID::R,
				ButtonID::L3, ButtonID::R3, ButtonID::MINUS, ButtonID::PLUS,
				ButtonID::CAPTURE, ButtonID::HOME,
			};
			if (BeginCombo(Tr("Hold to activate###FlyMouseHold", Zh(u8"按住启用键###FlyMouseHold")), ButtonDisplayName(_flyMouseHoldButton)))
			{
				for (ButtonID option : flyMouseButtons)
				{
					if (Selectable(ButtonDisplayName(option), _flyMouseHoldButton == option))
					{
						_flyMouseHoldButton = option;
						if (auto gyro = SettingsManager::getV<GyroSettings>(SettingID::GYRO_ON))
						{
							auto value = gyro->value();
							value.always_off = true;
							value.ignore_mode = GyroIgnoreMode::BUTTON;
							value.button = option;
							gyro->set(value);
						}
						saveUiSettings();
					}
				}
				EndCombo();
			}
			TextColored(TEXT_MUTED, Tr("Current: hold %s", Zh(u8"当前：按住 %s")), ButtonDisplayName(_flyMouseHoldButton));
			Dummy({ 0.f, 8.f });
			SetNextItemWidth(-FLT_MIN);
			if (SliderFloat(Tr("Sensitivity###SimpleGyroSensitivity", Zh(u8"灵敏度###SimpleGyroSensitivity")), &sensitivity, 0.1f, 10.f, "%.1f"))
			{
				_flyMouseSensitivity = sensitivity;
				if (minSensitivity) minSensitivity->set({ sensitivity, sensitivity });
				if (maxSensitivity) maxSensitivity->set({ sensitivity, sensitivity });
				saveUiSettings();
			}
			TextColored(TEXT_MUTED, "%s", Tr("Lower is steadier; higher turns faster.", Zh(u8"数值越低越稳，越高转向越快。")));
			Dummy({ 0.f, 8.f });
			auto gyroOutput = SettingsManager::getV<GyroOutput>(SettingID::GYRO_OUTPUT);
			if (gyroOutput)
			{
				GyroOutput outputValue = gyroOutput->value();
				const char* outputLabel = outputValue == GyroOutput::MOUSE ? Tr("Mouse (recommended)", Zh(u8"鼠标（推荐）")) :
				  outputValue == GyroOutput::LEFT_STICK ? Tr("Left stick", Zh(u8"左摇杆")) :
				  outputValue == GyroOutput::RIGHT_STICK ? Tr("Right stick", Zh(u8"右摇杆")) : "PlayStation Motion";
				if (BeginCombo(Tr("Output###SimpleGyroOutput", Zh(u8"输出方式###SimpleGyroOutput")), outputLabel))
				{
					for (GyroOutput option : { GyroOutput::MOUSE, GyroOutput::LEFT_STICK, GyroOutput::RIGHT_STICK, GyroOutput::PS_MOTION })
					{
						const char* optionLabel = option == GyroOutput::MOUSE ? Tr("Mouse (recommended)", Zh(u8"鼠标（推荐）")) :
						  option == GyroOutput::LEFT_STICK ? Tr("Left stick", Zh(u8"左摇杆")) :
						  option == GyroOutput::RIGHT_STICK ? Tr("Right stick", Zh(u8"右摇杆")) : "PlayStation Motion";
						if (Selectable(optionLabel, outputValue == option))
							gyroOutput->set(option);
					}
					EndCombo();
				}
			}
			EndDisabled();
			EndChild();

			TableNextColumn();
			BeginChild("GyroCalibrationCard", { 0.f, 368.f }, ImGuiChildFlags_Borders);
		TextColored(ACCENT, "%s", Tr("CALIBRATION", Zh(u8"校准")));
		auto autoCalibration = SettingsManager::getV<Switch>(SettingID::AUTO_CALIBRATE_GYRO);
		bool autoCalibrationEnabled = autoCalibration && autoCalibration->value() == Switch::ON;
		if (Checkbox(Tr("Calibrate automatically###SimpleAutoGyro", Zh(u8"自动校准陀螺仪###SimpleAutoGyro")), &autoCalibrationEnabled) && autoCalibration)
			autoCalibration->set(autoCalibrationEnabled ? Switch::ON : Switch::OFF);
		TextColored(TEXT_MUTED, "%s", Tr("Place the controller on a stable surface during manual calibration.", Zh(u8"手动校准时，请将控制器平放在稳定表面。")));
		static float simpleCalibrationDuration = 3.f;
		SetNextItemWidth(220.f);
		SliderFloat(Tr("Duration###SimpleCalibrationDuration", Zh(u8"校准时长###SimpleCalibrationDuration")), &simpleCalibrationDuration, 0.5f, 5.f, "%.1f s");
		if (Button(Tr("Calibrate now", Zh(u8"立即校准")), { 220.f, 38.f }))
		{
			const int32_t durationMs = static_cast<int32_t>(simpleCalibrationDuration * 1000.f);
			std::thread([durationMs]()
			{
				WriteToConsole("RESTART_GYRO_CALIBRATION");
				Sleep(durationMs);
				WriteToConsole("FINISH_GYRO_CALIBRATION");
			}).detach();
		}
		Dummy({ 0.f, 8.f });
		TextColored(TEXT_MUTED, "%s", Tr("Calibration removes slow cursor drift while the controller is still.", Zh(u8"校准可以消除手柄静止时的光标缓慢漂移。")));
		EndChild();
			EndTable();
		}
	}
	else if (_mainPage == MainPage::Settings)
	{
		TextColored(TEXT, "%s", Tr("Settings", Zh(u8"设置")));
		TextColored(TEXT_MUTED, "%s", Tr("Control how JoyShockMapper behaves in Windows.", Zh(u8"设置 JoyShockMapper 在 Windows 中的运行方式。")));
		Dummy({ 0.f, 12.f });

		BeginChild("BackgroundSettings", { 0.f, 238.f }, ImGuiChildFlags_Borders);
		TextColored(ACCENT, "%s", Tr("BACKGROUND OPERATION", Zh(u8"后台运行")));
		Dummy({ 0.f, 8.f });
		bool minimizeToTray = _minimizeToTray;
		if (Checkbox(Tr("Minimize to system tray###MinimizeToTray", Zh(u8"最小化到系统托盘###MinimizeToTray")), &minimizeToTray))
		{
			_minimizeToTray = minimizeToTray;
			saveUiSettings();
			_settingsStatus = Tr("Tray preference saved.", Zh(u8"托盘设置已保存。"));
		}
		TextColored(TEXT_MUTED, "%s", Tr("When minimized, the window is hidden while mappings and gyro keep running. Click the tray icon to reopen it.", Zh(u8"最小化后窗口会隐藏，但映射和陀螺仪仍在后台运行；单击托盘图标即可重新打开。")));
		Dummy({ 0.f, 14.f });

		bool startWithWindows = _startWithWindows;
		if (Checkbox(Tr("Start with Windows###StartWithWindows", Zh(u8"开机自动启动###StartWithWindows")), &startWithWindows))
		{
			if (setStartWithWindows(startWithWindows))
			{
				_startWithWindows = startWithWindows;
				saveUiSettings();
				_settingsStatus = startWithWindows ? Tr("Windows startup enabled.", Zh(u8"已开启开机自动启动。")) : Tr("Windows startup disabled.", Zh(u8"已关闭开机自动启动。"));
			}
			else
			{
				_settingsStatus = Tr("Could not update the Windows startup entry.", Zh(u8"无法修改 Windows 开机启动项。"));
			}
		}
		TextColored(TEXT_MUTED, "%s", Tr("Uses the current executable path and does not require administrator access.", Zh(u8"使用当前程序路径，不需要管理员权限；移动 EXE 后再次打开软件即可更新路径。")));
		if (!_settingsStatus.empty())
		{
			Dummy({ 0.f, 12.f });
			TextColored(ACCENT, "%s", _settingsStatus.c_str());
		}
		EndChild();
	}
	else
	{
		TextColored(TEXT, "%s", Tr("Advanced controller editor", Zh(u8"高级控制器编辑器")));
		TextColored(TEXT_MUTED, "%s", Tr("The original full JSM interface is preserved here.", Zh(u8"原有的完整 JSM 设置界面保留在这里。")));
		Dummy({ 0.f, 6.f });
		if (BeginTabBar("BindingsTab"))
		{
			for (auto tabIt = _tabs.begin(); tabIt != _tabs.end();)
			{
				const bool close = !tabIt->second.draw(renderingAreaPos, renderingAreaSize);
				if (close && tabIt->first != ButtonID::NONE)
					tabIt = _tabs.erase(tabIt);
				else
					++tabIt;
			}
			if (_newTab != ButtonID::NONE && !_tabs.contains(_newTab))
			{
				stringstream ss;
				ss << "Chorded " << _newTab;
				auto [tab, inserted] = _tabs.try_emplace(_newTab, ss.str(), _jsl, _newTab);
				if (inserted)
					tab->second.draw(renderingAreaPos, renderingAreaSize, true);
			}
			_newTab = ButtonID::NONE;

			for (auto& mapping : mappings)
			{
				for (auto chords = mapping.getChords(); *chords; ++*chords)
				{
					const auto chord = **chords;
					if (!_tabs.contains(chord))
					{
						stringstream ss;
						ss << "Chorded " << chord;
						auto [tab, inserted] = _tabs.try_emplace(chord, ss.str(), _jsl, chord);
						if (inserted)
							tab->second.draw(renderingAreaPos, renderingAreaSize);
					}
				}
			}
			for (auto& [_, settingBase] : SettingsManager::getSettings())
			{
				for (auto chords = settingBase->getChords(); *chords; ++*chords)
				{
					const auto chord = **chords;
					if (!_tabs.contains(chord))
					{
						stringstream ss;
						ss << "Chorded " << chord;
						auto [tab, inserted] = _tabs.try_emplace(chord, ss.str(), _jsl, chord);
						if (inserted)
							tab->second.draw(renderingAreaPos, renderingAreaSize);
					}
				}
			}
			EndTabBar();
		}
	}
	_inputSelector.draw();
	EndChild();
	End();       // MainWindow

	// Rendering
	SDL_SetRenderDrawColor(renderer, 241, 242, 245, 255);
	SDL_RenderClear(renderer);
	Render();
	ImGui_ImplSDLRenderer3_RenderDrawData(GetDrawData(), renderer);
	SDL_RenderPresent(renderer);
}

void Application::createChord(ButtonID chord)
{
	_newTab = chord;
}

void Application::updateDeviceProfiles(std::span<SDL_Gamepad*> controllers)
{
	string leftKey;
	string rightKey;
	string fullKey;
	_leftJoyConConnected = false;
	_rightJoyConConnected = false;

	for (SDL_Gamepad* gamepad : controllers)
	{
		SDL_GamepadType type = SDL_GetRealGamepadType(gamepad);
		if (type == SDL_GAMEPAD_TYPE_UNKNOWN)
			type = SDL_GetGamepadType(gamepad);
		if (type == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT)
		{
			_leftJoyConConnected = true;
			leftKey = DeviceIdentity(gamepad);
		}
		else if (type == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT)
		{
			_rightJoyConConnected = true;
			rightKey = DeviceIdentity(gamepad);
		}
		else
		{
			_leftJoyConConnected = true;
			_rightJoyConConnected = true;
			if (fullKey.empty())
				fullKey = DeviceIdentity(gamepad);
		}
	}

	if (!fullKey.empty())
	{
		_leftProfileKey.clear();
		_rightProfileKey.clear();
		const string profileKey = "full|" + fullKey;
		if (profileKey != _fullProfileKey)
		{
			_fullProfileKey = profileKey;
			LoadDeviceProfile(_fullProfileKey, 3);
		}
	}
	else
	{
		_fullProfileKey.clear();
		if (!leftKey.empty())
		{
			const string profileKey = "left|" + leftKey;
			if (profileKey != _leftProfileKey)
			{
				_leftProfileKey = profileKey;
				LoadDeviceProfile(_leftProfileKey, 1);
			}
		}
		if (!rightKey.empty())
		{
			const string profileKey = "right|" + rightKey;
			if (profileKey != _rightProfileKey)
			{
				_rightProfileKey = profileKey;
				LoadDeviceProfile(_rightProfileKey, 2);
			}
		}
	}
}

void Application::saveActiveDeviceProfiles()
{
	if (!_fullProfileKey.empty())
		SaveDeviceProfile(_fullProfileKey, 3);
	else
	{
		SaveDeviceProfile(_leftProfileKey, 1);
		SaveDeviceProfile(_rightProfileKey, 2);
	}
}

void Application::editMapping(JSMVariable<Mapping>* variable, string_view name)
{
	_inputSelector.show(variable, name, [this]() { saveActiveDeviceProfiles(); });
}

Application::BindingTab::BindingTab(string_view name, JslWrapper* jsl, ButtonID chord)
  : _name(name)
  , _chord(chord)
  , _jsl(jsl)
{
}

void Application::BindingTab::drawLabel(ButtonID btn)
{
	AlignTextToFramePadding();
	Text(enum_name(btn).data());
	if (IsItemHovered(ImGuiHoveredFlags_DelayNormal))
	{
		SetTooltip(commandRegistry.getHelp(enum_name(btn)).data());
	}
};
void Application::BindingTab::drawLabel(SettingID stg)
{
	AlignTextToFramePadding();
	TextUnformatted(SettingDisplayName(stg));
	if (IsItemHovered(ImGuiHoveredFlags_DelayNormal))
	{
		if (g_chinese)
			SetTooltip("%s\n\n%s", enum_name(stg).data(), commandRegistry.getHelp(enum_name(stg)).data());
		else
			SetTooltip(commandRegistry.getHelp(enum_name(stg)).data());
	}
};
void Application::BindingTab::drawLabel(string_view cmd)
{
	AlignTextToFramePadding();
	Text(cmd.data());
	if (IsItemHovered(ImGuiHoveredFlags_DelayNormal))
	{
		SetTooltip(commandRegistry.getHelp(cmd).data());
	}
}

void Application::BindingTab::drawAnyFloat(SettingID stg, bool labeled)
{
	JSMVariable<float>* variable = nullptr;
	float value;
	auto setting = SettingsManager::get<float>(stg);
	if (setting)
	{
		variable = setting->atChord(_chord);
		value = variable ? variable->value() : setting->value();
	}
	else
	{
		variable = SettingsManager::getV<float>(stg);
		value = variable->value();
	}

	const string label = SettingWidgetLabel(stg, labeled);
	InputFloat(label.c_str(), &value, 0.f, 0.f, variable ? "%.3f" : "[%.3f]", ImGuiInputTextFlags_None);
	if (IsItemDeactivatedAfterEdit())
	{
		if (!variable)
			variable = &setting->createChord(_chord);

		variable->set(value);
	}
	if (labeled && IsItemHovered(ImGuiHoveredFlags_DelayNormal))
	{
		SetTooltip(commandRegistry.getHelp(enum_name(stg)).data());
	}
}

void Application::BindingTab::drawPercentFloat(SettingID stg, bool labeled)
{
	JSMVariable<float>* variable = nullptr;
	float value;
	auto setting = SettingsManager::get<float>(stg);
	if (setting)
	{
		variable = setting->atChord(_chord);
		value = variable ? variable->value() : setting->value();
	}
	else
	{
		variable = SettingsManager::getV<float>(stg);
		value = variable->value();
	}

	const string label = SettingWidgetLabel(stg, labeled);
	SliderFloat(label.c_str(), &value, 0.f, 1.f, variable ? "%.2f" : "[%.2f]", ImGuiInputTextFlags_None);
	if (IsItemDeactivatedAfterEdit())
	{
		if (!variable)
			variable = &setting->createChord(_chord);

		variable->set(value);
	}
}

template<typename T>
T Application::BindingTab::getSettingValue(SettingID stg, JSMVariable<T>** outVariable)
{
	T value;
	JSMVariable<T> *variable = nullptr;
	auto setting = SettingsManager::get<T>(stg);
	if (setting)
	{
		variable = setting->atChord(_chord);
		value = variable ? variable->value() : setting->value();
	}
	else
	{
		variable = SettingsManager::getV<T>(stg);
		value = variable->value();
	}
	if (outVariable)
	{
		*outVariable = variable;
	}
	return value;
}

void Application::BindingTab::drawAny2Floats(SettingID stg, bool labeled)
{

	JSMVariable<FloatXY>* variable = nullptr;
	FloatXY value;
	auto setting = SettingsManager::get<FloatXY>(stg);
	if (setting)
	{
		variable = setting->atChord(_chord);
		value = variable ? variable->value() : setting->value();
	}
	else
	{
		variable = SettingsManager::getV<FloatXY>(stg);
		value = variable->value();
	}

	const string label = SettingWidgetLabel(stg, labeled);
	InputFloat2(label.c_str(), &value.first, variable ? "%.0f" : "[%.0f]", ImGuiInputTextFlags_None);
	if (IsItemDeactivatedAfterEdit())
	{
		if (!variable)
			variable = &setting->createChord(_chord);

		variable->set(value);
	}
}

bool Application::BindingTab::draw(ImVec2& renderingAreaPos, ImVec2& renderingAreaSize, bool setFocus)
{
	static constexpr float barSize = 75.f;
	ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
	if (ButtonID::NONE == _chord)
		flags |= ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;
	if (setFocus)
		flags |= ImGuiTabItemFlags_SetSelected;
	bool open = true;
	stringstream tabLabel;
	if (_chord == ButtonID::NONE)
		tabLabel << Tr("Base Layer", Zh(u8"基础层"));
	else
		tabLabel << Tr("Chorded ", Zh(u8"组合层 ")) << _chord;
	tabLabel << "###" << _name;
	if (ImGui::BeginTabItem(tabLabel.str().c_str(), &open, flags))
	{
		// SliderFloat("barwidth", &barSize, 20.f, 500.f);
		auto mainWindowSize = ImGui::GetContentRegionAvail();
		// static int sizingPolicy = 0;
		// int sizingPolicies[] = { ImGuiTableFlags_SizingFixedFit, ImGuiTableFlags_SizingFixedSame, ImGuiTableFlags_SizingStretchProp, ImGuiTableFlags_SizingStretchSame };
		// Combo("Sizing policy", &sizingPolicy, "FixedFit\0FixedSame\0StretchProp\0StretchSame");

		// Left
		BeginChild("Left Bindings", { mainWindowSize.x * 1.f / 5.f, mainWindowSize.y - barSize }, true, ImGuiChildFlags_None);
		if (BeginTable("LeftTable", 2, ImGuiTableFlags_SizingStretchSame))
		{
			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Top buttons", Zh(u8"顶部按键")));
			TableNextColumn();

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::ZLF);
			TableNextColumn();
			bool disabled = getSettingValue<TriggerMode>(SettingID::ZL_MODE) == TriggerMode::NO_FULL;
			if (disabled)
				BeginDisabled();
			drawButton(ButtonID::ZLF);
			if (disabled)
				EndDisabled();

			TableNextRow();
			TableNextColumn();
			drawLabel(SettingID::ZL_MODE);
			TableNextColumn();
			drawCombo<TriggerMode>(SettingID::ZL_MODE, _chord);
			if (IsItemClicked(ImGuiMouseButton_Right))
			{
				_stickConfigPopup = SettingID::ZL_MODE;
			}

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::ZL);
			TableNextColumn();
			drawButton(ButtonID::ZL);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::L);
			TableNextColumn();
			drawButton(ButtonID::L);

			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Face buttons", Zh(u8"正面按键")));

			TableNextRow();
			TableNextColumn();
			drawLabel("-");
			TableNextColumn();
			drawButton(ButtonID::MINUS);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::UP);
			TableNextColumn();
			drawButton(ButtonID::UP);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::LEFT);
			TableNextColumn();
			drawButton(ButtonID::LEFT);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::RIGHT);
			TableNextColumn();
			drawButton(ButtonID::RIGHT);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::DOWN);
			TableNextColumn();
			drawButton(ButtonID::DOWN);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::CAPTURE);
			disabled = getSettingValue<TriggerMode>(SettingID::TOUCHPAD_DUAL_STAGE_MODE) == TriggerMode::NO_FULL;
			if (disabled)
				BeginDisabled();
			TableNextColumn();
			drawButton(ButtonID::CAPTURE);
			if (disabled)
				EndDisabled();

			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Back buttons", Zh(u8"背部按键")));

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::LSL);
			TableNextColumn();
			drawButton(ButtonID::LSL);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::LSR);
			TableNextColumn();
			drawButton(ButtonID::LSR);

			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Left stick", Zh(u8"左摇杆")));

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::L3);
			TableNextColumn();
			drawButton(ButtonID::L3);

			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::LRING);
			SameLine();
			drawCombo<RingMode>(SettingID::LEFT_RING_MODE, _chord, ImGuiComboFlags_NoPreview);
			if (IsItemHovered())
				SetTooltip(commandRegistry.getHelp(enum_name(SettingID::LEFT_RING_MODE)).data());
			TableNextColumn();
			drawButton(ButtonID::LRING);

			TableNextRow();
			TableNextColumn();
			drawLabel(SettingID::LEFT_STICK_MODE);
			TableNextColumn();
			drawCombo<StickMode>(SettingID::LEFT_STICK_MODE, _chord);
			if (IsItemClicked(ImGuiMouseButton_Right))
			{
				_stickConfigPopup = SettingID::RIGHT_STICK_MODE;
			}
			// SameLine();
			// if (Button("..."))
			//	_stickConfigPopup = SettingID::LEFT_STICK_MODE;
			// if (IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			//	SetTooltip("More left stick settings");

			StickMode leftStickMode = getSettingValue<StickMode>(SettingID::LEFT_STICK_MODE);
			if (leftStickMode == StickMode::NO_MOUSE || leftStickMode == StickMode::OUTER_RING || leftStickMode == StickMode::INNER_RING)
			{
				TableNextRow();
				TableNextColumn();
				drawLabel(ButtonID::LUP);
				TableNextColumn();
				drawButton(ButtonID::LUP);

				TableNextRow();
				TableNextColumn();
				drawLabel(ButtonID::LLEFT);
				TableNextColumn();
				drawButton(ButtonID::LLEFT);

				TableNextRow();
				TableNextColumn();
				drawLabel(ButtonID::LRIGHT);
				TableNextColumn();
				drawButton(ButtonID::LRIGHT);

				TableNextRow();
				TableNextColumn();
				drawLabel(ButtonID::LDOWN);
				TableNextColumn();
				drawButton(ButtonID::LDOWN);
			}
			else if (leftStickMode == StickMode::AIM)
			{
				TableNextRow();
				TableNextColumn();
				drawLabel(SettingID::STICK_SENS);
				TableNextColumn();
				drawAny2Floats(SettingID::STICK_SENS);
			}
			else if (leftStickMode == StickMode::FLICK || leftStickMode == StickMode::FLICK_ONLY || leftStickMode == StickMode::ROTATE_ONLY)
			{
				TableNextRow();
				TableNextColumn();
				drawLabel(SettingID::FLICK_STICK_OUTPUT);
				TableNextColumn();
				drawCombo<GyroOutput>(SettingID::FLICK_STICK_OUTPUT, _chord);
			}
			else if (leftStickMode == StickMode::MOUSE_AREA || leftStickMode == StickMode::MOUSE_RING)
			{
				TableNextRow();
				TableNextColumn();
				drawLabel(SettingID::MOUSE_RING_RADIUS);
				TableNextColumn();
				drawAnyFloat(SettingID::MOUSE_RING_RADIUS);
			}
			else if (leftStickMode == StickMode::SCROLL_WHEEL)
			{
				TableNextRow();
				TableNextColumn();
				drawLabel(ButtonID::LLEFT);
				TableNextColumn();
				drawButton(ButtonID::LLEFT);

				TableNextRow();
				TableNextColumn();
				drawLabel(ButtonID::LRIGHT);
				TableNextColumn();
				drawButton(ButtonID::LRIGHT);
			}
			EndTable();
		}
		EndChild();

		ImGui::SameLine();

		// Right
		BeginGroup();
		BeginChild("Top buttons", { mainWindowSize.x * 3.f / 5.f, barSize }, true);
		if (BeginTable("TopTable", 6, ImGuiTableFlags_SizingStretchSame))
		{
			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::TOUCH);
			SameLine();
			drawCombo<TriggerMode>(SettingID::TOUCHPAD_DUAL_STAGE_MODE, _chord, ImGuiComboFlags_NoPreview);
			if (IsItemHovered())
				SetTooltip(commandRegistry.getHelp(enum_name(SettingID::TOUCHPAD_DUAL_STAGE_MODE)).data());
			TableSetColumnIndex(4);
			drawLabel(ButtonID::MIC);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::TOUCH);
			TableSetColumnIndex(4);
			drawButton(ButtonID::MIC);

			EndTable();
		}
		EndChild();

		renderingAreaSize = { mainWindowSize.x * 3.f / 5.f, GetContentRegionAvail().y - barSize };
		BeginChild("Rendering window", renderingAreaSize, true);
		renderingAreaPos = ImGui::GetWindowPos();
		EndChild(); // Top buttons
		EndGroup();

		SameLine();
		BeginChild("Right Bindings", { 0.f, mainWindowSize.y - barSize }, true);
		if (BeginTable("RightTable", 2, ImGuiTableFlags_SizingStretchSame))
		{
			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Top buttons", Zh(u8"顶部按键")));

			TableNextRow();
			TableNextColumn();
			bool disabled = getSettingValue<TriggerMode>(SettingID::ZL_MODE) == TriggerMode::NO_FULL;
			if (disabled)
				BeginDisabled();
			drawButton(ButtonID::ZRF);
			if (disabled)
				EndDisabled();
			TableNextColumn();
			drawLabel(ButtonID::ZRF);

			TableNextRow();
			TableNextColumn();
			drawCombo<TriggerMode>(SettingID::ZR_MODE, _chord);
			if (IsItemClicked(ImGuiMouseButton_Right))
			{
				_stickConfigPopup = SettingID::ZL_MODE;
			}
			TableNextColumn();
			drawLabel(SettingID::ZR_MODE);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::ZR);
			TableNextColumn();
			drawLabel(ButtonID::ZR);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::R);
			TableNextColumn();
			drawLabel(ButtonID::R);

			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Face buttons", Zh(u8"正面按键")));

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::PLUS);
			TableNextColumn();
			drawLabel("+");

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::N);
			TableNextColumn();
			drawLabel(ButtonID::N);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::E);
			TableNextColumn();
			drawLabel(ButtonID::E);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::W);
			TableNextColumn();
			drawLabel(ButtonID::W);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::S);
			TableNextColumn();
			drawLabel(ButtonID::S);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::HOME);
			TableNextColumn();
			drawLabel(ButtonID::HOME);

			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Back buttons", Zh(u8"背部按键")));

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::RSR);
			TableNextColumn();
			drawLabel(ButtonID::RSR);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::RSL);
			TableNextColumn();
			drawLabel(ButtonID::RSL);

			TableNextRow();
			TableNextColumn();
			drawLabel(Tr("Right stick", Zh(u8"右摇杆")));

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::R3);
			TableNextColumn();
			drawLabel(ButtonID::R3);

			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::RRING);
			TableNextColumn();
			drawLabel(ButtonID::RRING);
			SameLine();
			drawCombo<RingMode>(SettingID::RIGHT_RING_MODE, _chord, ImGuiComboFlags_NoPreview);
			if (IsItemHovered())
				SetTooltip(commandRegistry.getHelp(enum_name(SettingID::RIGHT_RING_MODE)).data());

			TableNextRow();
			TableNextColumn();
			drawCombo<StickMode>(SettingID::RIGHT_STICK_MODE, _chord);
			if (IsItemClicked(ImGuiMouseButton_Right))
			{
				_stickConfigPopup = SettingID::RIGHT_STICK_MODE;
			}
			// SameLine();
			// if (Button("..."))
			//	_stickConfigPopup = SettingID::RIGHT_STICK_MODE;
			// if (IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			//	SetTooltip("More right stick settings");
			TableNextColumn();
			drawLabel(SettingID::RIGHT_STICK_MODE);

			StickMode rightStickMode = getSettingValue<StickMode>(SettingID::RIGHT_STICK_MODE);
			if (rightStickMode == StickMode::NO_MOUSE || rightStickMode == StickMode::OUTER_RING || rightStickMode == StickMode::INNER_RING)
			{
				TableNextRow();
				TableNextColumn();
				drawButton(ButtonID::RUP);
				TableNextColumn();
				drawLabel(ButtonID::RUP);

				TableNextRow();
				TableNextColumn();
				drawButton(ButtonID::RLEFT);
				TableNextColumn();
				drawLabel(ButtonID::RLEFT);

				TableNextRow();
				TableNextColumn();
				drawButton(ButtonID::RRIGHT);
				TableNextColumn();
				drawLabel(ButtonID::RRIGHT);

				TableNextRow();
				TableNextColumn();
				drawButton(ButtonID::RDOWN);
				TableNextColumn();
				drawLabel(ButtonID::RDOWN);
			}
			else if (rightStickMode == StickMode::AIM)
			{
				TableNextRow();
				TableNextColumn();
				drawAny2Floats(SettingID::STICK_SENS);
				TableNextColumn();
				drawLabel(SettingID::STICK_SENS);
			}
			else if (rightStickMode == StickMode::FLICK || rightStickMode == StickMode::FLICK_ONLY || rightStickMode == StickMode::ROTATE_ONLY)
			{
				TableNextRow();
				TableNextColumn();
				drawCombo<GyroOutput>(SettingID::FLICK_STICK_OUTPUT, _chord);
				TableNextColumn();
				drawLabel(SettingID::FLICK_STICK_OUTPUT);
			}
			else if (rightStickMode == StickMode::MOUSE_AREA || rightStickMode == StickMode::MOUSE_RING)
			{
				TableNextRow();
				TableNextColumn();
				drawAnyFloat(SettingID::MOUSE_RING_RADIUS);
				TableNextColumn();
				drawLabel(SettingID::MOUSE_RING_RADIUS);
			}
			else if (rightStickMode == StickMode::SCROLL_WHEEL)
			{
				TableNextRow();
				TableNextColumn();
				drawButton(ButtonID::RLEFT);
				TableNextColumn();
				drawLabel(ButtonID::RLEFT);

				TableNextRow();
				TableNextColumn();
				drawButton(ButtonID::RRIGHT);
				TableNextColumn();
				drawLabel(ButtonID::RRIGHT);
			}
			EndTable();
		}
		EndChild();

		BeginChild("Bottom Bindings", { 0.f, barSize }, true);
		if (BeginTable("BottomTable", 11, ImGuiTableFlags_SizingStretchSame))
		{
			// TODO: GYRO_OUTPUT, AUTO_CALIBRATE_GYRO
			TableNextRow();
			TableNextColumn();
			drawButton(ButtonID::LEAN_LEFT);
			TableNextColumn();
			drawButton(ButtonID::LEAN_RIGHT);
			TableNextColumn();
			drawCombo<GyroOutput>(SettingID::GYRO_OUTPUT, _chord);
			if (IsItemClicked(ImGuiMouseButton_Right))
			{
				OpenPopup("GyroSensContext");
			}
			TableNextColumn();
			drawCombo<GyroSpace>(SettingID::GYRO_SPACE, _chord);
			TableNextColumn();
			JSMVariable<GyroSettings>* gyroSettingsVar = nullptr;
			auto gyroSettingsVal = getSettingValue<GyroSettings>(SettingID::GYRO_ON, &gyroSettingsVar);
			stringstream ss;
			ss << gyroSettingsVal;
			if (BeginCombo("##GyroButton", ss.str().c_str(), ImGuiComboFlags_NoArrowButton))
			{
				bool isSelected = gyroSettingsVal.ignore_mode == GyroIgnoreMode::LEFT_STICK;
				if (Selectable(enum_name(GyroIgnoreMode::LEFT_STICK).data(), isSelected))
				{
					gyroSettingsVal.ignore_mode = GyroIgnoreMode::LEFT_STICK;
					gyroSettingsVal.button = ButtonID::NONE;
					gyroSettingsVar->set(gyroSettingsVal);
				}
				if (isSelected)
					SetItemDefaultFocus();
				isSelected = gyroSettingsVal.ignore_mode == GyroIgnoreMode::RIGHT_STICK;
				if (Selectable(enum_name(GyroIgnoreMode::RIGHT_STICK).data(), isSelected))
				{
					gyroSettingsVal.ignore_mode = GyroIgnoreMode::RIGHT_STICK;
					gyroSettingsVal.button = ButtonID::NONE;
					gyroSettingsVar->set(gyroSettingsVal);
				}
				if (isSelected)
					SetItemDefaultFocus();
				for (ButtonID id = ButtonID::NONE; id < ButtonID::SIZE; id = id + 1)
				{
					isSelected = gyroSettingsVal.button == id && gyroSettingsVal.ignore_mode == GyroIgnoreMode::BUTTON;
					if (Selectable(enum_name(id).data(), isSelected))
					{
						gyroSettingsVal.ignore_mode = GyroIgnoreMode::BUTTON;
						gyroSettingsVal.button = id;
						gyroSettingsVar->set(gyroSettingsVal);
					}
					if (isSelected)
						SetItemDefaultFocus();
				}
				EndCombo();
			}
			TableNextColumn();
			drawButton(ButtonID::MRING);
			TableNextColumn();
			drawCombo<StickMode>(SettingID::MOTION_STICK_MODE, _chord);
			if (IsItemClicked(ImGuiMouseButton_Right))
			{
				_stickConfigPopup = SettingID::MOTION_STICK_MODE;
			}

			StickMode motionStickMode = getSettingValue<StickMode>(SettingID::MOTION_STICK_MODE);
			if (motionStickMode == StickMode::NO_MOUSE || motionStickMode == StickMode::OUTER_RING || motionStickMode == StickMode::INNER_RING)
			{
				TableNextColumn();
				drawButton(ButtonID::MUP);

				TableNextColumn();
				drawButton(ButtonID::MLEFT);

				TableNextColumn();
				drawButton(ButtonID::MRIGHT);

				TableNextColumn();
				drawButton(ButtonID::MDOWN);
			}
			else if (motionStickMode == StickMode::AIM)
			{
				TableNextColumn();
				drawAny2Floats(SettingID::STICK_SENS);
			}
			else if (motionStickMode == StickMode::FLICK || motionStickMode == StickMode::FLICK_ONLY || motionStickMode == StickMode::ROTATE_ONLY)
			{
				TableNextColumn();
				drawCombo<GyroOutput>(SettingID::FLICK_STICK_OUTPUT, _chord);
			}
			else if (motionStickMode == StickMode::MOUSE_AREA || motionStickMode == StickMode::MOUSE_RING)
			{
				TableNextColumn();
				drawAnyFloat(SettingID::MOUSE_RING_RADIUS);
			}
			else if (motionStickMode == StickMode::SCROLL_WHEEL)
			{
				TableNextColumn();
				drawButton(ButtonID::MLEFT);

				TableNextColumn();
				drawButton(ButtonID::MRIGHT);
			}

			// TODO: LEAN_THRESHOLD in context menu
			// TODO: TRACKBALL_DECAY TRIGGER_SKIP_DELAY
			TableNextRow();
			TableNextColumn();
			drawLabel(ButtonID::LEAN_LEFT);
			TableNextColumn();
			drawLabel(ButtonID::LEAN_RIGHT);
			TableNextColumn();
			drawLabel(SettingID::GYRO_OUTPUT);
			TableNextColumn();
			drawLabel(SettingID::GYRO_SPACE);
			TableNextColumn();
			AlignTextToFramePadding();
			Text("GYRO_");
			if (IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			{
				SetTooltip(commandRegistry.getHelp(gyroSettingsVal.always_off ? "GYRO_ON" : "GYRO_OFF").data());
			}
			SameLine();
			Switch enableButton = *enum_cast<Switch>(gyroSettingsVal.always_off);
			if (BeginCombo("##GyroMode", enum_name(enableButton).data(), ImGuiComboFlags_NoArrowButton))
			{
				if (Selectable(Tr("ON###GYRO_ON", Zh(u8"开启###GYRO_ON")), enableButton == Switch::ON))
				{
					gyroSettingsVal.always_off = true;
					gyroSettingsVar->set(gyroSettingsVal);
				}
				if (Selectable(Tr("OFF###GYRO_OFF", Zh(u8"关闭###GYRO_OFF")), enableButton == Switch::OFF))
				{
					gyroSettingsVal.always_off = false;
					gyroSettingsVar->set(gyroSettingsVal);
				}
				EndCombo();
			}
			TableNextColumn();
			drawLabel(ButtonID::MRING);
			SameLine();
			drawCombo<RingMode>(SettingID::MOTION_RING_MODE, _chord, ImGuiComboFlags_NoPreview);
			TableNextColumn();
			drawLabel(SettingID::MOTION_STICK_MODE);

			if (motionStickMode == StickMode::NO_MOUSE || motionStickMode == StickMode::OUTER_RING || motionStickMode == StickMode::INNER_RING)
			{
				TableNextColumn();
				drawLabel(ButtonID::MUP);

				TableNextColumn();
				drawLabel(ButtonID::MLEFT);

				TableNextColumn();
				drawLabel(ButtonID::MRIGHT);

				TableNextColumn();
				drawLabel(ButtonID::MDOWN);
			}
			else if (motionStickMode == StickMode::AIM)
			{
				TableNextColumn();
				drawLabel(SettingID::STICK_SENS);
			}
			else if (motionStickMode == StickMode::FLICK || motionStickMode == StickMode::FLICK_ONLY || motionStickMode == StickMode::ROTATE_ONLY)
			{
				TableNextColumn();
				drawLabel(SettingID::FLICK_STICK_OUTPUT);
			}
			else if (motionStickMode == StickMode::MOUSE_AREA || motionStickMode == StickMode::MOUSE_RING)
			{
				TableNextColumn();
				drawLabel(SettingID::MOUSE_RING_RADIUS);
			}
			else if (motionStickMode == StickMode::SCROLL_WHEEL)
			{
				TableNextColumn();
				drawLabel(ButtonID::MLEFT);

				TableNextColumn();
				drawLabel(ButtonID::MRIGHT);
			}

			if (BeginPopup("GyroSensContext"))
			{
				drawAny2Floats(SettingID::MIN_GYRO_SENS, true);
				drawAny2Floats(SettingID::MAX_GYRO_SENS, true);
				drawAnyFloat(SettingID::MIN_GYRO_THRESHOLD, true);
				drawAnyFloat(SettingID::MAX_GYRO_THRESHOLD, true);
				drawAnyFloat(SettingID::GYRO_SMOOTH_THRESHOLD, true);
				drawAnyFloat(SettingID::GYRO_SMOOTH_TIME, true);
				drawAnyFloat(SettingID::GYRO_CUTOFF_SPEED, true);
				drawAnyFloat(SettingID::GYRO_CUTOFF_RECOVERY, true);
				drawCombo<GyroAxisMask>(SettingID::MOUSE_X_FROM_GYRO_AXIS, _chord, ImGuiComboFlags_NoArrowButton, true);
				drawCombo<GyroAxisMask>(SettingID::MOUSE_Y_FROM_GYRO_AXIS, _chord, ImGuiComboFlags_NoArrowButton, true);
				drawCombo<JoyconMask>(SettingID::JOYCON_GYRO_MASK, _chord, ImGuiComboFlags_NoArrowButton, true);
				EndPopup();
			}

			EndTable();
		}
		EndChild();

		if (_showPopup != ButtonID::INVALID)
		{
			stringstream ss;
			JSMVariable<Mapping>* variable = nullptr;
			if (_chord != ButtonID::NONE)
			{
				ss << _chord << ',';
				variable = &mappings[int(_showPopup)].createChord(_chord);
			}
			else
			{
				variable = &mappings[int(_showPopup)];
			}
			ss << _showPopup;
			_app->editMapping(variable, ss.str());
			_showPopup = ButtonID::INVALID;
		}

		if (ImGui::BeginPopup("StickConfig"))
		{
			// TODO: LEFT_STICK_VIRTUAL_SCALE, RIGHT_STICK_VIRTUAL_SCALE
			if (_stickConfigPopup == SettingID::RIGHT_STICK_MODE)
			{
				drawLabel(SettingID::RIGHT_STICK_DEADZONE_INNER);
				SameLine();
				drawPercentFloat(SettingID::RIGHT_STICK_DEADZONE_INNER);

				drawLabel(SettingID::RIGHT_STICK_DEADZONE_OUTER);
				SameLine();
				drawPercentFloat(SettingID::RIGHT_STICK_DEADZONE_OUTER);
			}
			else if (_stickConfigPopup == SettingID::LEFT_STICK_MODE)
			{
				drawLabel(SettingID::LEFT_STICK_DEADZONE_INNER);
				SameLine();
				drawPercentFloat(SettingID::LEFT_STICK_DEADZONE_INNER);

				drawLabel(SettingID::LEFT_STICK_DEADZONE_OUTER);
				SameLine();
				drawPercentFloat(SettingID::LEFT_STICK_DEADZONE_OUTER);
			}
			else if (_stickConfigPopup == SettingID::MOTION_STICK_MODE)
			{
				drawLabel(SettingID::MOTION_DEADZONE_INNER);
				SameLine();
				drawAnyFloat(SettingID::MOTION_DEADZONE_INNER);

				drawLabel(SettingID::MOTION_DEADZONE_OUTER);
				SameLine();
				drawAnyFloat(SettingID::MOTION_DEADZONE_OUTER);
			}
			else if (_stickConfigPopup == SettingID::ZL_MODE)
			{
				bool hairTrigger = SettingsManager::get<float>(SettingID::TRIGGER_THRESHOLD)->value() == -1.f;
				if (Checkbox(Tr("Hair Trigger", Zh(u8"快速扳机")), &hairTrigger))
				{
					if (hairTrigger)
						SettingsManager::get<float>(SettingID::TRIGGER_THRESHOLD)->set(-1.f);
					else
						SettingsManager::get<float>(SettingID::TRIGGER_THRESHOLD)->reset();
				}
				if (!hairTrigger)
					drawPercentFloat(SettingID::TRIGGER_THRESHOLD, TRUE);
			}

			auto stickMode = getSettingValue<StickMode>(_stickConfigPopup);
			if (stickMode == StickMode::FLICK || stickMode == StickMode::FLICK_ONLY || stickMode == StickMode::ROTATE_ONLY)
			{
				drawLabel(SettingID::FLICK_SNAP_MODE);
				SameLine();
				drawCombo<FlickSnapMode>(SettingID::FLICK_SNAP_MODE, _chord);

				drawLabel(SettingID::FLICK_SNAP_STRENGTH);
				SameLine();
				drawPercentFloat(SettingID::FLICK_SNAP_STRENGTH);

				drawLabel(SettingID::FLICK_DEADZONE_ANGLE);
				SameLine();
				drawAnyFloat(SettingID::FLICK_DEADZONE_ANGLE);

				auto& fsOut = *SettingsManager::get<GyroOutput>(SettingID::FLICK_STICK_OUTPUT);
				if (fsOut == GyroOutput::MOUSE)
				{
					drawLabel(SettingID::FLICK_TIME);
					SameLine();
					drawAnyFloat(SettingID::FLICK_TIME);

					drawLabel(SettingID::FLICK_TIME_EXPONENT);
					SameLine();
					drawAnyFloat(SettingID::FLICK_TIME_EXPONENT);
				}
				else // LEFT_STICK, RIGHT_STICK, PS_MOTION
				{
					drawLabel(SettingID::VIRTUAL_STICK_CALIBRATION);
					SameLine();
					drawAnyFloat(SettingID::VIRTUAL_STICK_CALIBRATION);
				}
			}
			else if (stickMode == StickMode::AIM)
			{
				drawLabel(SettingID::STICK_POWER);
				SameLine();
				drawAnyFloat(SettingID::STICK_POWER);

				drawLabel(SettingID::STICK_ACCELERATION_RATE);
				SameLine();
				drawAnyFloat(SettingID::STICK_ACCELERATION_RATE);

				drawLabel(SettingID::STICK_ACCELERATION_CAP);
				SameLine();
				drawAnyFloat(SettingID::STICK_ACCELERATION_CAP);
			}

			else if (stickMode == StickMode::MOUSE_RING)
			{
				drawLabel(SettingID::SCREEN_RESOLUTION_X);
				SameLine();
				drawAnyFloat(SettingID::SCREEN_RESOLUTION_X);

				drawLabel(SettingID::SCREEN_RESOLUTION_Y);
				SameLine();
				drawAnyFloat(SettingID::SCREEN_RESOLUTION_Y);
			}
			else if (stickMode == StickMode::SCROLL_WHEEL)
			{
				drawLabel(SettingID::SCROLL_SENS);
				SameLine();
				drawAny2Floats(SettingID::SCROLL_SENS);
			}
			else if (stickMode == StickMode::LEFT_STICK)
			{
				drawLabel(SettingID::LEFT_STICK_UNDEADZONE_INNER);
				SameLine();
				drawAnyFloat(SettingID::LEFT_STICK_UNDEADZONE_INNER);

				drawLabel(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
				SameLine();
				drawAnyFloat(SettingID::LEFT_STICK_UNDEADZONE_OUTER);

				drawLabel(SettingID::VIRTUAL_STICK_CALIBRATION);
				SameLine();
				drawAnyFloat(SettingID::VIRTUAL_STICK_CALIBRATION);
			}
			else if (stickMode == StickMode::RIGHT_STICK)
			{
				drawLabel(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
				SameLine();
				drawAnyFloat(SettingID::RIGHT_STICK_UNDEADZONE_INNER);

				drawLabel(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
				SameLine();
				drawAnyFloat(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);

				drawLabel(SettingID::VIRTUAL_STICK_CALIBRATION);
				SameLine();
				drawAnyFloat(SettingID::VIRTUAL_STICK_CALIBRATION);
			}
			else if (stickMode >= StickMode::LEFT_ANGLE_TO_X && stickMode <= StickMode::RIGHT_ANGLE_TO_Y)
			{
				drawLabel(SettingID::ANGLE_TO_AXIS_DEADZONE_INNER);
				SameLine();
				drawAnyFloat(SettingID::ANGLE_TO_AXIS_DEADZONE_INNER);

				drawLabel(SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER);
				SameLine();
				drawAnyFloat(SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER);

				if (stickMode == StickMode::LEFT_ANGLE_TO_X || stickMode == StickMode::LEFT_ANGLE_TO_Y) // isLeft
				{
					drawLabel(SettingID::LEFT_STICK_UNDEADZONE_INNER);
					SameLine();
					drawAnyFloat(SettingID::LEFT_STICK_UNDEADZONE_INNER);

					drawLabel(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
					SameLine();
					drawAnyFloat(SettingID::LEFT_STICK_UNDEADZONE_OUTER);

					drawLabel(SettingID::LEFT_STICK_UNPOWER);
					SameLine();
					drawAnyFloat(SettingID::LEFT_STICK_UNPOWER);
				}
				else // isRight!
				{
					drawLabel(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
					SameLine();
					drawAnyFloat(SettingID::RIGHT_STICK_UNDEADZONE_INNER);

					drawLabel(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
					SameLine();
					drawAnyFloat(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);

					drawLabel(SettingID::RIGHT_STICK_UNPOWER);
					SameLine();
					drawAnyFloat(SettingID::RIGHT_STICK_UNPOWER);
				}
			}
			else if (stickMode == StickMode::LEFT_WIND_X || stickMode == StickMode::RIGHT_WIND_X)
			{
				drawLabel(SettingID::WIND_STICK_RANGE);
				SameLine();
				drawAnyFloat(SettingID::WIND_STICK_RANGE);

				drawLabel(SettingID::WIND_STICK_POWER);
				SameLine();
				drawAnyFloat(SettingID::WIND_STICK_POWER);

				drawLabel(SettingID::UNWIND_RATE);
				SameLine();
				drawAnyFloat(SettingID::UNWIND_RATE);

				if (stickMode == StickMode::LEFT_WIND_X) // isLeft
				{
					drawLabel(SettingID::LEFT_STICK_UNDEADZONE_INNER);
					SameLine();
					drawAnyFloat(SettingID::LEFT_STICK_UNDEADZONE_INNER);

					drawLabel(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
					SameLine();
					drawAnyFloat(SettingID::LEFT_STICK_UNDEADZONE_OUTER);

					drawLabel(SettingID::LEFT_STICK_UNPOWER);
					SameLine();
					drawAnyFloat(SettingID::LEFT_STICK_UNPOWER);
				}
				else // isRight!
				{
					drawLabel(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
					SameLine();
					drawAnyFloat(SettingID::RIGHT_STICK_UNDEADZONE_INNER);

					drawLabel(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
					SameLine();
					drawAnyFloat(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);

					drawLabel(SettingID::RIGHT_STICK_UNPOWER);
					SameLine();
					drawAnyFloat(SettingID::RIGHT_STICK_UNPOWER);
				}
			}
			EndPopup();
		}

		if (!IsPopupOpen("StickConfig") && _stickConfigPopup != SettingID::INVALID)
		{
			static bool openOrClear = true;
			if (openOrClear)
				OpenPopup("StickConfig");
			else
				_stickConfigPopup = SettingID::INVALID;
			openOrClear = !openOrClear;
		}
		ImGui::EndTabItem();
	}

	return open;
}
