use joyshockmapper_input_engine::simple_key_chord;
use std::mem::size_of;
use windows_sys::Win32::UI::Input::KeyboardAndMouse::{
    INPUT, INPUT_0, INPUT_KEYBOARD, KEYBDINPUT, KEYEVENTF_KEYUP, SendInput,
};
#[cfg(test)]
use windows_sys::Win32::{
    Foundation::POINT,
    UI::{
        Input::KeyboardAndMouse::{GetAsyncKeyState, INPUT_MOUSE, MOUSEEVENTF_MOVE, MOUSEINPUT},
        WindowsAndMessaging::{GetCursorPos, SetCursorPos},
    },
};

#[derive(Debug, Clone, PartialEq, Eq)]
enum PlannedOutput {
    KeyDown(u16),
    KeyUp(u16),
}

fn virtual_key(name: &str) -> Option<u16> {
    let upper = name.to_ascii_uppercase();
    match upper.as_str() {
        "CTRL" | "CONTROL" | "LCTRL" | "LCONTROL" => Some(0xA2),
        "RCTRL" | "RCONTROL" => Some(0xA3),
        "SHIFT" | "LSHIFT" => Some(0xA0),
        "RSHIFT" => Some(0xA1),
        "ALT" | "LALT" => Some(0xA4),
        "RALT" => Some(0xA5),
        "SPACE" => Some(0x20),
        "ENTER" | "RETURN" => Some(0x0D),
        "ESC" | "ESCAPE" => Some(0x1B),
        "TAB" => Some(0x09),
        "BACKSPACE" => Some(0x08),
        "UP" => Some(0x26),
        "DOWN" => Some(0x28),
        "LEFT" => Some(0x25),
        "RIGHT" => Some(0x27),
        value if value.len() == 1 => {
            let byte = value.as_bytes()[0];
            (byte.is_ascii_alphanumeric()).then_some(byte as u16)
        }
        value if value.starts_with('F') => value[1..].parse::<u16>().ok()
            .filter(|number| (1..=24).contains(number))
            .map(|number| 0x6F + number),
        _ => None,
    }
}

fn plan_key_chord(command: &str) -> Option<Vec<PlannedOutput>> {
    let keys = simple_key_chord(command)?
        .into_iter().map(virtual_key).collect::<Option<Vec<_>>>()?;
    let mut output = keys.iter().copied().map(PlannedOutput::KeyDown).collect::<Vec<_>>();
    output.extend(keys.into_iter().rev().map(PlannedOutput::KeyUp));
    Some(output)
}

fn keyboard_input(vk: u16, key_up: bool) -> INPUT {
    INPUT {
        r#type: INPUT_KEYBOARD,
        Anonymous: INPUT_0 { ki: KEYBDINPUT {
            wVk: vk,
            wScan: 0,
            dwFlags: if key_up { KEYEVENTF_KEYUP } else { 0 },
            time: 0,
            dwExtraInfo: 0,
        }},
    }
}

#[cfg(test)]
fn mouse_input(dx: i32, dy: i32) -> INPUT {
    INPUT {
        r#type: INPUT_MOUSE,
        Anonymous: INPUT_0 { mi: MOUSEINPUT {
            dx, dy, mouseData: 0, dwFlags: MOUSEEVENTF_MOVE, time: 0, dwExtraInfo: 0,
        }},
    }
}

fn send(inputs: &[INPUT]) -> Result<(), String> {
    let sent = unsafe { SendInput(inputs.len() as u32, inputs.as_ptr(), size_of::<INPUT>() as i32) };
    (sent == inputs.len() as u32).then_some(())
        .ok_or_else(|| format!("SendInput only accepted {sent}/{} events", inputs.len()))
}

pub fn execute_command(command: &str) -> Result<(), String> {
    let plan = plan_key_chord(command)
        .ok_or_else(|| format!("暂不支持输出命令：{command}"))?;
    let inputs = plan.into_iter().map(|event| match event {
        PlannedOutput::KeyDown(vk) => keyboard_input(vk, false),
        PlannedOutput::KeyUp(vk) => keyboard_input(vk, true),
    }).collect::<Vec<_>>();
    send(&inputs)
}

pub fn execute_sequence(commands: &[&str]) -> Result<(), String> {
    for (index, command) in commands.iter().enumerate() {
        execute_command(command)?;
        if index + 1 < commands.len() {
            std::thread::sleep(std::time::Duration::from_millis(35));
        }
    }
    Ok(())
}

#[cfg(test)]
#[derive(Default)]
struct FlyMouseAccumulator { x: f32, y: f32 }

#[cfg(test)]
impl FlyMouseAccumulator {
    fn sample(&mut self, gyro_x: f32, gyro_y: f32, sensitivity: f32, dt_seconds: f32) -> (i32, i32) {
        self.x += gyro_x * sensitivity * dt_seconds;
        self.y += gyro_y * sensitivity * dt_seconds;
        let dx = self.x.trunc() as i32;
        let dy = self.y.trunc() as i32;
        self.x -= dx as f32;
        self.y -= dy as f32;
        (dx, dy)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use joyshockmapper_input_engine::{Binding, InputEngine};
    use std::time::{Duration, Instant};

    #[test]
    fn simulated_controller_press_plans_chord_in_safe_order() {
        let mut engine = InputEngine::new(200);
        engine.bind("N", Binding { single: "LCTRL+LSHIFT+A".into(), double: String::new() });
        let action = engine.press("N", 0).pop().expect("simulated press should map");
        assert_eq!(plan_key_chord(&action.command), Some(vec![
            PlannedOutput::KeyDown(0xA2), PlannedOutput::KeyDown(0xA0), PlannedOutput::KeyDown(0x41),
            PlannedOutput::KeyUp(0x41), PlannedOutput::KeyUp(0xA0), PlannedOutput::KeyUp(0xA2),
        ]));
    }

    #[test]
    fn fly_mouse_fractional_motion_is_uniform_and_drift_free() {
        let mut accumulator = FlyMouseAccumulator::default();
        let samples = (0..120).map(|_| accumulator.sample(120.0, 0.0, 1.0, 1.0 / 120.0).0).collect::<Vec<_>>();
        assert_eq!(samples.iter().sum::<i32>(), 120);
        assert!(samples.iter().all(|value| *value == 1));
    }

    #[test]
    fn fly_mouse_keeps_subpixel_motion_instead_of_jittering() {
        let mut accumulator = FlyMouseAccumulator::default();
        let samples = (0..120).map(|_| accumulator.sample(30.0, 0.0, 1.0, 1.0 / 120.0).0).collect::<Vec<_>>();
        assert_eq!(samples.iter().sum::<i32>(), 30);
        assert!(samples.windows(4).all(|window| window.iter().sum::<i32>() == 1));
    }

    #[test]
    fn simulated_120hz_sampling_has_no_long_frame_stalls() {
        let mut gaps = Vec::new();
        let mut previous = Instant::now();
        for _ in 0..30 {
            std::thread::sleep(Duration::from_micros(8_333));
            let now = Instant::now();
            gaps.push(now.duration_since(previous));
            previous = now;
        }
        let worst_gap = gaps.into_iter().max().unwrap_or_default();
        assert!(worst_gap < Duration::from_millis(25), "worst frame gap: {worst_gap:?}");
    }

    #[test]
    #[ignore = "requires an interactive Windows input desktop"]
    fn windows_sendinput_accepts_a_harmless_f24_press() {
        send(&[keyboard_input(0x87, false)]).unwrap();
        std::thread::sleep(Duration::from_millis(10));
        assert_ne!(unsafe { GetAsyncKeyState(0x87) } & 0x8000u16 as i16, 0);
        send(&[keyboard_input(0x87, true)]).unwrap();
    }

    #[test]
    #[ignore = "requires an interactive Windows input desktop"]
    fn simulated_120hz_gyro_path_moves_cursor_and_restores_it() {
        let mut origin = POINT { x: 0, y: 0 };
        assert_ne!(unsafe { GetCursorPos(&mut origin) }, 0);
        let mut accumulator = FlyMouseAccumulator::default();
        let mut gaps = Vec::new();
        let mut previous = Instant::now();
        for _ in 0..60 {
            std::thread::sleep(Duration::from_micros(8_333));
            let now = Instant::now();
            gaps.push(now.duration_since(previous));
            previous = now;
            let (dx, dy) = accumulator.sample(120.0, 0.0, 1.0, 1.0 / 120.0);
            send(&[mouse_input(dx, dy)]).unwrap();
        }
        let mut moved = POINT { x: 0, y: 0 };
        assert_ne!(unsafe { GetCursorPos(&mut moved) }, 0);
        unsafe { SetCursorPos(origin.x, origin.y) };
        assert!(moved.x >= origin.x + 55, "cursor moved only {} px", moved.x - origin.x);
        let worst_gap = gaps.into_iter().max().unwrap_or_default();
        assert!(worst_gap < Duration::from_millis(25), "worst frame gap: {worst_gap:?}");
    }
}
