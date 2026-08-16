use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

/// Settings owned by the future Rust/Tauri shell. The real-time mapping engine
/// remains in C++ until its behavior can be replaced feature by feature.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(default, rename_all = "camelCase")]
pub struct UiSettings {
    pub language: Language,
    pub minimize_to_tray: bool,
    pub start_with_windows: bool,
    pub fly_mouse: FlyMouseSettings,
}

impl Default for UiSettings {
    fn default() -> Self {
        Self {
            language: Language::Zh,
            minimize_to_tray: true,
            start_with_windows: false,
            fly_mouse: FlyMouseSettings::default(),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum Language { Zh, En }

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(default, rename_all = "camelCase")]
pub struct FlyMouseSettings {
    pub enabled: bool,
    pub hold_button: ControllerButton,
    /// Maximum native JoyShockMapper sensitivity at fast rotation speeds.
    pub sensitivity: f32,
    pub precision_sensitivity: f32,
    pub response_threshold: f32,
    pub smoothing_threshold: f32,
}

impl Default for FlyMouseSettings {
    fn default() -> Self {
        Self {
            enabled: true,
            hold_button: ControllerButton::Zl,
            sensitivity: 1.0,
            precision_sensitivity: 0.7,
            response_threshold: 75.0,
            smoothing_threshold: 2.0,
        }
    }
}

impl FlyMouseSettings {
    /// The invariant used by both engines: motion output is permitted only
    /// while the configured controller button is physically held.
    pub fn is_active(&self, held_button: Option<ControllerButton>) -> bool {
        self.enabled && held_button == Some(self.hold_button)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "UPPERCASE")]
pub enum ControllerButton {
    Zl,
    Zr,
    L,
    R,
    L3,
    R3,
    Minus,
    Plus,
    Capture,
    Home,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DeviceProfile {
    pub id: String,
    pub mappings: Vec<ButtonMapping>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ButtonMapping {
    pub button: String,
    pub single: String,
    pub double: String,
}

const PROFILE_BUTTONS: &[&str] = &[
    "UP", "DOWN", "LEFT", "RIGHT", "L", "ZL", "MINUS", "E", "S", "N", "W", "R",
    "ZR", "PLUS", "HOME", "LSL", "LSR", "RSL", "RSR", "L3", "R3", "LEAN_LEFT",
    "LEAN_RIGHT", "MIC", "LUP", "LDOWN", "LLEFT", "LRIGHT", "LRING", "RUP", "RDOWN",
    "RLEFT", "RRIGHT", "RRING", "MUP", "MDOWN", "MLEFT", "MRIGHT", "MRING", "TOUCH",
    "ZLF", "CAPTURE", "ZRF", "TUP", "TDOWN", "TLEFT", "TRIGHT", "TRING",
];

impl DeviceProfile {
    pub fn parse(id: impl Into<String>, source: &str) -> Self {
        let mut values: BTreeMap<String, (String, String)> = BTreeMap::new();
        for line in source.lines() {
            let Some((input, command)) = line.split_once('\t') else { continue };
            if command.is_empty() { continue; }
            let (button, is_double) = match input.split_once(',') {
                Some((left, right)) if left == right => (left, true),
                None => (input, false),
                _ => continue,
            };
            if !PROFILE_BUTTONS.contains(&button) { continue; }
            let entry = values.entry(button.to_owned()).or_default();
            if is_double { entry.1 = command.to_owned(); } else { entry.0 = command.to_owned(); }
        }
        Self {
            id: id.into(),
            mappings: PROFILE_BUTTONS.iter().map(|button| {
                let (single, double) = values.remove(*button).unwrap_or_default();
                ButtonMapping { button: (*button).to_owned(), single, double }
            }).collect(),
        }
    }

    pub fn validate(&self) -> Result<(), String> {
        if !is_profile_id(&self.id) { return Err("invalid device profile id".into()); }
        for mapping in &self.mappings {
            if !PROFILE_BUTTONS.contains(&mapping.button.as_str()) {
                return Err(format!("invalid controller button: {}", mapping.button));
            }
            for command in [&mapping.single, &mapping.double] {
                if command.len() > 512 || command.contains(['\r', '\n', '\t']) {
                    return Err(format!("invalid mapping command for {}", mapping.button));
                }
            }
        }
        Ok(())
    }

    pub fn serialize(&self) -> Result<String, String> {
        self.validate()?;
        let mut output = String::from("# JoyShockMapper automatic device profile\n");
        for mapping in &self.mappings {
            if !mapping.single.is_empty() {
                output.push_str(&format!("{}\t{}\n", mapping.button, mapping.single));
            }
            if !mapping.double.is_empty() {
                output.push_str(&format!("{0},{0}\t{1}\n", mapping.button, mapping.double));
            }
        }
        Ok(output)
    }
}

pub fn is_profile_id(id: &str) -> bool {
    let Some(hash) = id.strip_prefix("device_").and_then(|id| id.strip_suffix(".jsmprofile")) else {
        return false;
    };
    hash.len() == 16 && hash.bytes().all(|value| value.is_ascii_hexdigit())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn fly_mouse_requires_the_configured_button_to_be_held() {
        let settings = FlyMouseSettings::default();
        assert!(!settings.is_active(None));
        assert!(!settings.is_active(Some(ControllerButton::Zr)));
        assert!(settings.is_active(Some(ControllerButton::Zl)));
    }

    #[test]
    fn disabled_fly_mouse_never_activates() {
        let settings = FlyMouseSettings {
            enabled: false,
            ..FlyMouseSettings::default()
        };
        assert!(!settings.is_active(Some(ControllerButton::Zl)));
    }

    #[test]
    fn cxx_profile_format_round_trips_single_and_double_press() {
        let source = "# profile\nZL\tRMOUSE\nZL,ZL\tESC\nN\tSPACE\ninvalid\n";
        let profile = DeviceProfile::parse("device_0123456789abcdef.jsmprofile", source);
        assert_eq!(profile.mappings.len(), PROFILE_BUTTONS.len());
        let zl = profile.mappings.iter().find(|mapping| mapping.button == "ZL").unwrap();
        assert_eq!(zl.double, "ESC");
        let serialized = profile.serialize().unwrap();
        assert!(serialized.contains("ZL\tRMOUSE\nZL,ZL\tESC\n"));
        assert!(serialized.contains("N\tSPACE\n"));
    }

    #[test]
    fn profile_rejects_path_traversal_and_multiline_commands() {
        let mut profile = DeviceProfile::parse("../../OnStartup.txt", "ZL\tRMOUSE\n");
        assert!(profile.serialize().is_err());
        profile.id = "device_0123456789abcdef.jsmprofile".into();
        profile.mappings[0].single = "A\nB".into();
        assert!(profile.serialize().is_err());
    }
}
