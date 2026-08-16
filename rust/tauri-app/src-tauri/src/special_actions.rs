use std::{
    ffi::CStr,
    path::Path,
    process::Command,
    sync::{OnceLock, mpsc::{self, Sender}},
    time::Duration,
};

use crate::output;
use windows_sys::Win32::{
    Foundation::{HWND, LPARAM},
    UI::{
        Shell::ShellExecuteW,
        WindowsAndMessaging::{EnumWindows, GetForegroundWindow, GetWindowThreadProcessId, IsWindowVisible, SW_RESTORE, SW_SHOWNORMAL, SetForegroundWindow, ShowWindow},
    },
};

static ACTIONS: OnceLock<Sender<String>> = OnceLock::new();

unsafe extern "C" {
    fn jsm_core_set_action_callback(callback: unsafe extern "C" fn(*const std::ffi::c_char));
}

unsafe extern "C" fn receive_action(value: *const std::ffi::c_char) {
    if value.is_null() { return; }
    let action = unsafe { CStr::from_ptr(value) }.to_string_lossy().into_owned();
    log_action(&format!("received action: {action}"));
    if let Some(sender) = ACTIONS.get() {
        let _ = sender.send(action);
    } else {
        log_action("ACTIONS not initialized");
    }
}

pub fn start() {
    let (sender, receiver) = mpsc::channel();
    if ACTIONS.set(sender).is_err() {
        log_action("special_actions::start already initialized");
        return;
    }
    unsafe { jsm_core_set_action_callback(receive_action) };
    log_action("special_actions::start OK, callback registered");
    std::thread::spawn(move || {
        while let Ok(action) = receiver.recv() {
            if let Err(error) = execute(&action) {
                log_action(&format!("execute failed: {error}"));
            }
        }
    });
}

/// Invoked once at startup to verify the screenshot launcher itself works,
/// independent of the C++ action-callback path.
pub fn self_test() {
    std::thread::spawn(|| {
        std::thread::sleep(std::time::Duration::from_millis(2500));
        if let Err(error) = screenshot() {
            log_action(&format!("screenshot self-test failed: {error}"));
        } else {
            log_action("screenshot self-test OK");
        }
    });
}

pub fn launch_focus_command(path: &str, shortcut: &str) -> Result<String, String> {
    let executable = Path::new(path);
    if !executable.is_file() || !executable.extension().and_then(|value| value.to_str()).is_some_and(|value| value.eq_ignore_ascii_case("exe")) {
        return Err("请选择现有的 .exe 软件文件".into());
    }
    if !matches!(shortcut, "LCONTROL+L" | "LCONTROL+F" | "TAB") {
        return Err("不支持的输入框定位方式".into());
    }
    let encode = |value: &str| value.as_bytes().iter().map(|byte| format!("{byte:02x}")).collect::<String>();
    let command = format!("\"UI_ACTION LAUNCH_FOCUS {} {}\"", encode(path), encode(shortcut));
    (command.len() <= 512).then_some(command).ok_or_else(|| "软件路径过长".into())
}

fn execute(action: &str) -> Result<(), String> {
    let mut parts = action.split_whitespace();
    match parts.next() {
        Some("COPY_ALL") => output::execute_sequence(&["LCONTROL+A", "LCONTROL+C"]),
        Some("CUT_ALL") => output::execute_sequence(&["LCONTROL+A", "LCONTROL+X"]),
        Some("FOCUS_INPUT") => {
            let shortcut = parts.next().ok_or("缺少输入框定位方式")?;
            if !matches!(shortcut, "LCONTROL+L" | "LCONTROL+F" | "TAB") {
                return Err("不支持的输入框定位方式".into());
            }
            output::execute_command(shortcut)
        }
        Some("LAUNCH_FOCUS") => {
            let path = decode_hex(parts.next().ok_or("缺少软件路径")?)?;
            let shortcut = decode_hex(parts.next().unwrap_or("4c434f4e54524f4c2b4c"))?;
            launch_and_focus(&path, &shortcut)
        }
        Some("SCREENSHOT") => screenshot(),
        _ => Err(format!("未知特殊动作：{action}")),
    }
}

fn launch_and_focus(path: &str, shortcut: &str) -> Result<(), String> {
    let executable = Path::new(path);
    if !executable.is_file() || !executable.extension().is_some_and(|value| value.eq_ignore_ascii_case("exe")) {
        return Err(format!("软件路径无效：{path}"));
    }
    let previous_window = unsafe { GetForegroundWindow() };
    let child = Command::new(executable).spawn().map_err(|error| format!("无法启动软件：{error}"))?;
    let mut activated = false;
    for _ in 0..30 {
        std::thread::sleep(Duration::from_millis(100));
        let foreground = unsafe { GetForegroundWindow() };
        if !foreground.is_null() && foreground != previous_window {
            activated = true;
            break;
        }
        let mut search = WindowSearch { process_id: child.id(), window: std::ptr::null_mut() };
        unsafe { EnumWindows(Some(find_process_window), &mut search as *mut WindowSearch as LPARAM) };
        if !search.window.is_null() {
            unsafe {
                ShowWindow(search.window, SW_RESTORE);
                SetForegroundWindow(search.window);
            }
            activated = true;
            break;
        }
    }
    if !activated {
        return Err("软件已启动，但未能确认其窗口已激活；未发送输入框快捷键".into());
    }
    std::thread::sleep(Duration::from_millis(120));
    output::execute_command(shortcut)
}

struct WindowSearch { process_id: u32, window: HWND }

unsafe extern "system" fn find_process_window(window: HWND, parameter: LPARAM) -> i32 {
    let search = unsafe { &mut *(parameter as *mut WindowSearch) };
    let mut process_id = 0;
    unsafe { GetWindowThreadProcessId(window, &mut process_id) };
    if process_id == search.process_id && unsafe { IsWindowVisible(window) } != 0 {
        search.window = window;
        return 0;
    }
    1
}

fn screenshot() -> Result<(), String> {
    log_action("screenshot invoked");
    // Try the modern URI scheme first (Snip & Sketch), then the legacy
    // SnippingTool.exe, then explorer as a last resort.
    let attempts: Vec<(&str, &str)> = vec![
        ("open", "ms-screenclip:"),
        ("open", "C:\\Windows\\System32\\SnippingTool.exe"),
        ("open", "C:\\Windows\\SysWOW64\\SnippingTool.exe"),
    ];
    for (operation, target) in attempts {
        let op = wide(operation);
        let tgt = wide(target);
        let result = unsafe { ShellExecuteW(std::ptr::null_mut(), op.as_ptr(), tgt.as_ptr(), std::ptr::null(), std::ptr::null(), SW_SHOWNORMAL) };
        log_action(&format!("ShellExecuteW({target}) -> {}", result as isize));
        if result as isize > 32 {
            return Ok(());
        }
    }
    Err("无法启动截图工具".into())
}

fn log_action(message: &str) {
    let dir = std::env::var_os("LOCALAPPDATA")
        .map(|p| std::path::PathBuf::from(p).join("JoyShockMapper"))
        .unwrap_or_else(|| ".".into());
    let _ = std::fs::OpenOptions::new().create(true).append(true)
        .open(dir.join("key-capture.log"))
        .map(|mut file| { use std::io::Write; let _ = writeln!(file, "[action] {message}"); });
}

fn wide(value: &str) -> Vec<u16> {
    value.encode_utf16().chain(std::iter::once(0)).collect()
}

fn decode_hex(value: &str) -> Result<String, String> {    if value.len() % 2 != 0 || value.len() > 480 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err("特殊动作参数损坏".into());
    }
    let bytes = (0..value.len()).step_by(2)
        .map(|index| u8::from_str_radix(&value[index..index + 2], 16).map_err(|error| error.to_string()))
        .collect::<Result<Vec<_>, _>>()?;
    String::from_utf8(bytes).map_err(|error| error.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn action_arguments_decode_utf8_paths() {
        assert_eq!(decode_hex("433a5c546f6f6c732e657865").unwrap(), "C:\\Tools.exe");
        assert!(decode_hex("xyz").is_err());
    }

    #[test]
    fn launch_action_only_accepts_existing_executables_and_known_focus_modes() {
        let path = std::env::temp_dir().join(format!("jsm-action-{}.exe", std::process::id()));
        std::fs::write(&path, []).unwrap();
        let command = launch_focus_command(path.to_str().unwrap(), "LCONTROL+L").unwrap();
        assert!(command.starts_with("\"UI_ACTION LAUNCH_FOCUS "));
        assert!(launch_focus_command(path.to_str().unwrap(), "ALT+F4").is_err());
        std::fs::remove_file(path).unwrap();
    }
}
