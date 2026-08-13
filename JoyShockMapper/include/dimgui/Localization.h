#pragma once

#include <string>
#include <string_view>

inline bool g_chinese = true;

inline const char* Zh(const char8_t* text)
{
	return reinterpret_cast<const char*>(text);
}

inline const char* Tr(const char* english, const char* chinese)
{
	return g_chinese ? chinese : english;
}

inline void ReplaceUiText(std::string& text, std::string_view from, std::string_view to)
{
	for (size_t pos = 0; (pos = text.find(from, pos)) != std::string::npos; pos += to.size())
		text.replace(pos, from.size(), to);
}

inline std::string LocalizeMappingDescription(std::string_view description)
{
	std::string text(description);
	if (!g_chinese)
		return text;

	ReplaceUiText(text, "no input", Zh(u8"无映射"));
	ReplaceUiText(text, " on Start Press", Zh(u8"（按下时）"));
	ReplaceUiText(text, " on ReleasePress", Zh(u8"（松开时）"));
	ReplaceUiText(text, " on TurboPress", Zh(u8"（连发）"));
	ReplaceUiText(text, " on TapPress", Zh(u8"（轻触时）"));
	ReplaceUiText(text, " on HoldPress", Zh(u8"（长按时）"));
	ReplaceUiText(text, "Toggle ", Zh(u8"切换 "));
	ReplaceUiText(text, "Instant ", Zh(u8"立即 "));
	ReplaceUiText(text, "Release ", Zh(u8"松开 "));
	ReplaceUiText(text, " and ", Zh(u8" 和 "));
	return text;
}
