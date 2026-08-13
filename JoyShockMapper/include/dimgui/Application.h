#pragma once

#include "imgui.h"
#include "SDL3/SDL.h"
#include <functional>
#include <future>
#include <atomic>
#include <map>
#include <optional>
#include <span>
#include "Mapping.h"
#include "InputSelector.h"

enum class ButtonID;
template<typename T>
class JSMVariable;

class JslWrapper;

void RequestShowMainWindow();

class AppIf
{
public:
	virtual ~AppIf() = default;

	virtual void createChord(ButtonID chord) = 0;
	virtual void editMapping(JSMVariable<Mapping>* variable, string_view name) = 0;
};

class Application : protected AppIf
{
public:
	Application(JslWrapper *jsl);

	~Application() override = default;

	void init();

	void cleanUp();

	void draw(std::span<SDL_Gamepad*> controllers);

protected:
	void createChord(ButtonID chord) override;
	void editMapping(JSMVariable<Mapping>* variable, string_view name) override;


private:
	static void HelpMarker(string_view cmd);
	void updateDeviceProfiles(std::span<SDL_Gamepad*> controllers);
	void saveActiveDeviceProfiles();
	void loadUiSettings();
	void saveUiSettings() const;
	bool setStartWithWindows(bool enabled);

	template<typename T>
	static void drawCombo(SettingID stg, ButtonID chord, ImGuiComboFlags flags = ImGuiComboFlags_NoArrowButton, bool labeled = false);

	struct BindingTab
	{
		BindingTab(string_view name, JslWrapper *jsl, ButtonID chord = ButtonID::NONE);
		bool draw(ImVec2 &renderingAreaPos, ImVec2 &renderingAreaSize, bool setFocus = false);

	private:
		void drawButton(ButtonID btn, ImVec2 size = ImVec2{ 0, 0 });
		void drawLabel(ButtonID btn);
		void drawLabel(SettingID stg);
		void drawLabel(string_view cmd);
		void drawAnyFloat(SettingID stg, bool labeled = false);
		void drawPercentFloat(SettingID stg, bool labeled = false);
		void drawAny2Floats(SettingID stg, bool labeled = false);
		template<typename T>
		T getSettingValue(SettingID setting, JSMVariable<T> **outVariable = nullptr);
		
		ButtonID _chord;
	public:
		static AppIf * _app;

		string _name;
		ButtonID _showPopup = ButtonID::INVALID;
		SettingID _stickConfigPopup = SettingID::INVALID;
		JslWrapper *_jsl;

		bool operator<(const BindingTab& rhs)
		{
			return _chord < rhs._chord;
		}
	};
	enum class MainPage
	{
		Home,
		Mappings,
		Gyro,
		Settings,
	};

	ButtonID _newTab = ButtonID::NONE;
	map<ButtonID, BindingTab> _tabs;
	InputSelector _inputSelector;
	MainPage _mainPage = MainPage::Home;
	ButtonID _selectedButton = ButtonID::S;
	std::string _leftProfileKey;
	std::string _rightProfileKey;
	std::string _fullProfileKey;
	bool _leftJoyConConnected = false;
	bool _rightJoyConConnected = false;
	bool _minimizeToTray = true;
	bool _startWithWindows = false;
	bool _flyMouseEnabled = true;
	ButtonID _flyMouseHoldButton = ButtonID::ZL;
	float _flyMouseSensitivity = 1.f;
	std::string _settingsStatus;
	bool show_demo_window = false;
	bool show_plot_demo_window = false;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	future<bool> threadDone;
	JslWrapper *_jsl;
};
