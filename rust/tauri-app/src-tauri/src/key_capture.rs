use std::{
    mem,
    sync::{
        Mutex,
        mpsc::{self, Sender},
    },
};

use tauri::{AppHandle, Emitter};
use windows_sys::Win32::{
    Foundation::{LPARAM, LRESULT, WPARAM},
    System::Threading::GetCurrentThreadId,
    UI::{
        Input::KeyboardAndMouse::{
            VK_ADD, VK_BACK, VK_CAPITAL, VK_DECIMAL, VK_DELETE, VK_DIVIDE, VK_DOWN, VK_END,
            VK_ESCAPE, VK_F1, VK_F24, VK_HOME, VK_INSERT, VK_LCONTROL, VK_LMENU, VK_LSHIFT, VK_LWIN,
            VK_LEFT, VK_MEDIA_NEXT_TRACK, VK_MEDIA_PREV_TRACK, VK_MEDIA_STOP, VK_MULTIPLY, VK_NEXT,
            VK_NUMLOCK, VK_NUMPAD0, VK_NUMPAD9, VK_OEM_1, VK_OEM_2, VK_OEM_3, VK_OEM_4, VK_OEM_5,
            VK_OEM_6, VK_OEM_7, VK_OEM_COMMA, VK_OEM_MINUS, VK_OEM_PERIOD, VK_OEM_PLUS, VK_PRIOR,
            VK_RCONTROL, VK_RETURN, VK_RIGHT, VK_RMENU, VK_RSHIFT, VK_RWIN, VK_SCROLL, VK_SPACE,
            VK_SUBTRACT, VK_TAB, VK_UP, VK_VOLUME_DOWN, VK_VOLUME_MUTE, VK_VOLUME_UP,
        },
        WindowsAndMessaging::{
            CallNextHookEx, DispatchMessageW, GetMessageW, KBDLLHOOKSTRUCT, PM_NOREMOVE,
            PeekMessageW, PostThreadMessageW, SetWindowsHookExW, TranslateMessage,
            UnhookWindowsHookEx, MSG, WH_KEYBOARD_LL, WM_KEYDOWN, WM_KEYUP, WM_QUIT, WM_SYSKEYDOWN,
            WM_SYSKEYUP,
        },
    },
};

static SENDER: Mutex<Option<Sender<CapturedKey>>> = Mutex::new(None);
static HOOK_THREAD_ID: Mutex<Option<u32>> = Mutex::new(None);

fn log(message: &str) {
    let _ = std::fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(std::path::Path::new(&std::env::var_os("LOCALAPPDATA").map_or_else(|| ".".into(), |p| std::path::PathBuf::from(p).join("JoyShockMapper"))).join("key-capture.log"))
        .map(|mut file| { use std::io::Write; let _ = writeln!(file, "{} {message}", std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_millis()).unwrap_or(0)); });
}

#[derive(Clone, serde::Serialize)]
struct CapturedKey {
    kind: &'static str,
    key: String,
}

/// Begin capturing keyboard input at the OS level so system-reserved
/// combinations (e.g. Win+Shift+S) are recorded before the shell sees them.
pub fn start(app: AppHandle) -> Result<(), String> {
    let mut slot = SENDER.lock().map_err(|error| error.to_string())?;
    if slot.is_some() {
        log("start: already capturing");
        return Ok(());
    }

    let (sender, receiver) = mpsc::channel::<CapturedKey>();
    *slot = Some(sender);
    drop(slot);

    // Dedicated thread owns the low-level hook and pumps its message queue.
    // A low-level keyboard hook only receives callbacks while the installing
    // thread runs a message loop. The thread must first create its message
    // queue (via a PeekMessage) BEFORE installing the hook, otherwise the
    // system cannot deliver hook callbacks to it.
    std::thread::spawn(move || {
        let mut message = unsafe { mem::zeroed::<MSG>() };
        unsafe { PeekMessageW(&mut message, std::ptr::null_mut(), 0, 0, PM_NOREMOVE) };

        let hook = unsafe {
            SetWindowsHookExW(WH_KEYBOARD_LL, Some(keyboard_proc), std::ptr::null_mut(), 0)
        };
        if hook.is_null() {
            log("hook install FAILED (null)");
            let _ = SENDER.lock().map(|mut slot| *slot = None);
            return;
        }
        log("hook installed OK");
        let thread_id = unsafe { GetCurrentThreadId() };
        let _ = HOOK_THREAD_ID.lock().map(|mut slot| *slot = Some(thread_id));

        while unsafe { GetMessageW(&mut message, std::ptr::null_mut(), 0, 0) } > 0 {
            unsafe {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        log("message loop exited; unhooking");
        unsafe { UnhookWindowsHookEx(hook) };
        let _ = HOOK_THREAD_ID.lock().map(|mut slot| *slot = None);
    });

    std::thread::spawn(move || {
        while let Ok(event) = receiver.recv() {
            if app.emit("key-captured", event).is_err() {
                break;
            }
        }
    });

    Ok(())
}

pub fn stop() {
    let _ = SENDER.lock().map(|mut slot| *slot = None);
    let thread_id = HOOK_THREAD_ID.lock().ok().and_then(|slot| *slot);
    if let Some(thread_id) = thread_id {
        unsafe { PostThreadMessageW(thread_id, WM_QUIT, 0, 0) };
    }
}

unsafe extern "system" fn keyboard_proc(code: i32, w_param: WPARAM, l_param: LPARAM) -> LRESULT {
    if code < 0 {
        return unsafe { CallNextHookEx(std::ptr::null_mut(), code, w_param, l_param) };
    }
    let key_up = match w_param as u32 {
        WM_KEYDOWN | WM_SYSKEYDOWN => false,
        WM_KEYUP | WM_SYSKEYUP => true,
        _ => return unsafe { CallNextHookEx(std::ptr::null_mut(), code, w_param, l_param) },
    };
    let info = unsafe { &*(l_param as *const KBDLLHOOKSTRUCT) };
    if let Some(key) = virtual_key_name(info.vkCode as u16) {
        if let Ok(slot) = SENDER.lock() {
            if let Some(sender) = slot.as_ref() {
                let _ = sender.send(CapturedKey {
                    kind: if key_up { "up" } else { "down" },
                    key,
                });
            }
        }
        // Swallow the event so the system shortcut (e.g. Win+Shift+S) does not
        // fire while a mapping is being recorded.
        return 1;
    }
    unsafe { CallNextHookEx(std::ptr::null_mut(), code, w_param, l_param) }
}

fn virtual_key_name(vk: u16) -> Option<String> {
    let name: Option<&str> = match vk {
        VK_LSHIFT => Some("LSHIFT"),
        VK_RSHIFT => Some("RSHIFT"),
        VK_LCONTROL => Some("LCONTROL"),
        VK_RCONTROL => Some("RCONTROL"),
        VK_LMENU => Some("LALT"),
        VK_RMENU => Some("RALT"),
        VK_LWIN => Some("LWINDOWS"),
        VK_RWIN => Some("RWINDOWS"),
        VK_SPACE => Some("SPACE"),
        VK_RETURN => Some("ENTER"),
        VK_ESCAPE => Some("ESC"),
        VK_TAB => Some("TAB"),
        VK_BACK => Some("BACKSPACE"),
        VK_LEFT => Some("LEFT"),
        VK_RIGHT => Some("RIGHT"),
        VK_UP => Some("UP"),
        VK_DOWN => Some("DOWN"),
        VK_PRIOR => Some("PAGEUP"),
        VK_NEXT => Some("PAGEDOWN"),
        VK_HOME => Some("HOME"),
        VK_END => Some("END"),
        VK_INSERT => Some("INSERT"),
        VK_DELETE => Some("DELETE"),
        VK_CAPITAL => Some("CAPS_LOCK"),
        VK_SCROLL => Some("SCROLL_LOCK"),
        VK_NUMLOCK => Some("NUM_LOCK"),
        VK_VOLUME_MUTE => Some("MUTE"),
        VK_VOLUME_DOWN => Some("VOLUME_DOWN"),
        VK_VOLUME_UP => Some("VOLUME_UP"),
        VK_MEDIA_NEXT_TRACK => Some("NEXT_TRACK"),
        VK_MEDIA_PREV_TRACK => Some("PREV_TRACK"),
        VK_MEDIA_STOP => Some("STOP_TRACK"),
        VK_MULTIPLY => Some("MULTIPLY"),
        VK_ADD => Some("ADD"),
        VK_SUBTRACT => Some("SUBTRACT"),
        VK_DECIMAL => Some("DECIMAL"),
        VK_DIVIDE => Some("DIVIDE"),
        VK_OEM_1 => Some(";"),
        VK_OEM_COMMA => Some(","),
        VK_OEM_PERIOD => Some("."),
        VK_OEM_2 => Some("/"),
        VK_OEM_3 => Some("`"),
        VK_OEM_4 => Some("["),
        VK_OEM_6 => Some("]"),
        VK_OEM_5 => Some("\\"),
        VK_OEM_MINUS => Some("-"),
        VK_OEM_PLUS => Some("+"),
        VK_OEM_7 => Some("'"),
        _ => None,
    };
    if let Some(name) = name {
        return Some(name.to_owned());
    }
    if (VK_F1..=VK_F24).contains(&vk) {
        return Some(format!("F{}", vk - VK_F1 + 1));
    }
    if (VK_NUMPAD0..=VK_NUMPAD9).contains(&vk) {
        return Some(format!("N{}", vk - VK_NUMPAD0));
    }
    if (0x41..=0x5A).contains(&vk) {
        return Some((vk as u8 as char).to_string());
    }
    if (0x30..=0x39).contains(&vk) {
        return Some((vk as u8 as char).to_string());
    }
    None
}
