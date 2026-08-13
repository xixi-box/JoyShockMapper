use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Binding {
    pub single: String,
    pub double: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct TriggeredAction {
    pub button: String,
    pub kind: PressKind,
    pub command: String,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum PressKind { Single, Double }

#[derive(Debug, Clone, PartialEq, Eq)]
struct PendingPress { button: String, deadline_ms: u64 }

/// Deterministic button timing core. It has no SDL, keyboard or Tauri
/// dependency, so its behavior can be compared with the C++ engine first.
pub struct InputEngine {
    double_press_window_ms: u64,
    bindings: HashMap<String, Binding>,
    held: HashSet<String>,
    pending: HashMap<String, PendingPress>,
}

impl InputEngine {
    pub fn new(double_press_window_ms: u64) -> Self {
        Self {
            double_press_window_ms,
            bindings: HashMap::new(), held: HashSet::new(), pending: HashMap::new(),
        }
    }

    pub fn bind(&mut self, button: impl Into<String>, binding: Binding) {
        self.bindings.insert(button.into(), binding);
    }

    pub fn press(&mut self, button: &str, now_ms: u64) -> Vec<TriggeredAction> {
        if !self.held.insert(button.to_owned()) { return Vec::new(); }
        let Some(binding) = self.bindings.get(button) else { return Vec::new() };
        if binding.double.is_empty() {
            return action(button, PressKind::Single, &binding.single);
        }
        if self.pending.remove(button).is_some() {
            return action(button, PressKind::Double, &binding.double);
        }
        self.pending.insert(button.to_owned(), PendingPress {
            button: button.to_owned(), deadline_ms: now_ms.saturating_add(self.double_press_window_ms),
        });
        Vec::new()
    }

    pub fn release(&mut self, button: &str) { self.held.remove(button); }

    pub fn tick(&mut self, now_ms: u64) -> Vec<TriggeredAction> {
        let due = self.pending.values()
            .filter(|pending| now_ms >= pending.deadline_ms)
            .map(|pending| pending.button.clone()).collect::<Vec<_>>();
        due.into_iter().flat_map(|button| {
            self.pending.remove(&button);
            self.bindings.get(&button)
                .map(|binding| action(&button, PressKind::Single, &binding.single))
                .unwrap_or_default()
        }).collect()
    }
}

fn action(button: &str, kind: PressKind, command: &str) -> Vec<TriggeredAction> {
    if command.is_empty() { Vec::new() } else { vec![TriggeredAction {
        button: button.to_owned(), kind, command: command.to_owned(),
    }] }
}

/// Splits a simple simultaneous keyboard output such as `LCTRL+LSHIFT+A`.
/// Full JSM command parsing remains in C++ until its grammar is migrated.
pub fn simple_key_chord(command: &str) -> Option<Vec<&str>> {
    let keys = command.split('+').map(str::trim).collect::<Vec<_>>();
    (!keys.is_empty() && keys.iter().all(|key| !key.is_empty() && key.bytes().all(|c| c.is_ascii_alphanumeric() || c == b'_')))
        .then_some(keys)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn engine() -> InputEngine {
        let mut engine = InputEngine::new(200);
        engine.bind("ZL", Binding { single: "RMOUSE".into(), double: "ESC".into() });
        engine
    }

    #[test]
    fn single_press_waits_until_double_window_expires() {
        let mut engine = engine();
        assert!(engine.press("ZL", 0).is_empty());
        engine.release("ZL");
        assert!(engine.tick(199).is_empty());
        let actions = engine.tick(200);
        assert_eq!(actions[0].kind, PressKind::Single);
        assert_eq!(actions[0].command, "RMOUSE");
    }

    #[test]
    fn second_press_replaces_pending_single_with_double() {
        let mut engine = engine();
        engine.press("ZL", 0); engine.release("ZL");
        let actions = engine.press("ZL", 120);
        assert_eq!(actions[0].kind, PressKind::Double);
        assert_eq!(actions[0].command, "ESC");
        assert!(engine.tick(300).is_empty());
    }

    #[test]
    fn held_button_does_not_repeat() {
        let mut engine = engine();
        engine.press("ZL", 0);
        assert!(engine.press("ZL", 50).is_empty());
    }

    #[test]
    fn parses_simple_simultaneous_key_output() {
        assert_eq!(simple_key_chord("LCTRL+LSHIFT+A"), Some(vec!["LCTRL", "LSHIFT", "A"]));
        assert_eq!(simple_key_chord("A++B"), None);
    }
}
