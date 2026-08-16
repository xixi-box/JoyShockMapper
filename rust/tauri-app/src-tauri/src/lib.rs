use joyshockmapper_ui_model::{DeviceProfile, UiSettings, is_profile_id};
use joyshockmapper_input_engine::{Binding, InputEngine, TriggeredAction};
use std::{fs, path::PathBuf, sync::{Arc, Mutex, atomic::{AtomicBool, Ordering}}};
use tauri::{
    Manager, State,
    menu::{Menu, MenuItem},
    tray::{MouseButton, MouseButtonState, TrayIconBuilder, TrayIconEvent},
};
use tauri_plugin_autostart::ManagerExt;

#[cfg(windows)]
mod output;
#[cfg(windows)]
mod embedded_core;
#[cfg(windows)]
mod dji_mic;
#[cfg(windows)]
mod special_actions;

struct SettingsState(Mutex<UiSettings>);
struct InputEngineState(Mutex<InputEngine>);

#[cfg(windows)]
#[tauri::command]
fn get_core_status(core: State<'_, embedded_core::EmbeddedCore>) -> embedded_core::CoreStatus { core.status() }

#[cfg(windows)]
#[tauri::command]
fn get_dji_mic_status(monitor: State<'_, dji_mic::DjiMicMonitor>) -> dji_mic::DjiMicStatus { monitor.status() }

#[cfg(windows)]
#[tauri::command]
fn set_dji_mic_setting(
    monitor: State<'_, dji_mic::DjiMicMonitor>,
    node: String,
    field: String,
    value: i16,
) -> Result<(), String> {
    monitor.set_setting(&node, &field, value)
}

#[cfg(windows)]
#[tauri::command]
fn reconnect_controllers(core: State<'_, embedded_core::EmbeddedCore>) -> Result<(), String> {
    core.command("RECONNECT_CONTROLLERS MERGE")
}

#[cfg(windows)]
#[tauri::command]
fn get_gyro_debug_status(core: State<'_, embedded_core::EmbeddedCore>) -> embedded_core::GyroDebugStatus {
    core.gyro_debug_status()
}

#[cfg(windows)]
#[tauri::command]
fn get_button_states(core: State<'_, embedded_core::EmbeddedCore>) -> embedded_core::ButtonStates {
    core.button_states()
}

#[cfg(windows)]
#[tauri::command]
fn calibrate_gyro(core: State<'_, embedded_core::EmbeddedCore>) -> Result<(), String> {
    core.command("RESTART_GYRO_CALIBRATION")?;
    std::thread::sleep(std::time::Duration::from_secs(4));
    core.command("FINISH_GYRO_CALIBRATION")
}

#[cfg(windows)]
#[tauri::command]
fn create_launch_focus_action(path: String, shortcut: String) -> Result<String, String> {
    special_actions::launch_focus_command(path.trim(), &shortcut)
}

#[cfg(windows)]
#[tauri::command]
fn configure_fly_mouse(
    core: State<'_, embedded_core::EmbeddedCore>,
    enabled: bool,
    hold_button: String,
    sensitivity: f32,
    precision_sensitivity: f32,
    response_threshold: f32,
    smoothing_threshold: f32,
) -> Result<(), String> {
    if !matches!(hold_button.as_str(), "ZL" | "ZR" | "L" | "R" | "L3" | "R3" | "MINUS" | "PLUS" | "CAPTURE" | "HOME") {
        return Err("invalid fly-mouse hold button".into());
    }
    if !sensitivity.is_finite() || !(0.1..=10.0).contains(&sensitivity)
        || !precision_sensitivity.is_finite() || !(0.1..=10.0).contains(&precision_sensitivity) {
        return Err("invalid fly-mouse sensitivity".into());
    }
    if !response_threshold.is_finite() || !(1.0..=500.0).contains(&response_threshold)
        || !smoothing_threshold.is_finite() || !(0.0..=30.0).contains(&smoothing_threshold) {
        return Err("invalid fly-mouse tuning".into());
    }
    apply_fly_mouse_settings(&core, enabled, &hold_button, sensitivity, precision_sensitivity, response_threshold, smoothing_threshold)
}

#[cfg(windows)]
fn apply_fly_mouse_settings(core: &embedded_core::EmbeddedCore, enabled: bool, hold_button: &str, sensitivity: f32, precision_sensitivity: f32, response_threshold: f32, smoothing_threshold: f32) -> Result<(), String> {
    core.command(&format!("MIN_GYRO_SENS = {precision_sensitivity}"))?;
    core.command(&format!("MAX_GYRO_SENS = {sensitivity}"))?;
    core.command("MIN_GYRO_THRESHOLD = 0")?;
    core.command(&format!("MAX_GYRO_THRESHOLD = {response_threshold}"))?;
    core.command(&format!("GYRO_SMOOTH_THRESHOLD = {smoothing_threshold}"))?;
    core.command("GYRO_SMOOTH_TIME = 0.05")?;
    core.command(&format!("GYRO_ON = {}", if enabled { hold_button } else { "NONE" }))
}

fn settings_path(app: &tauri::AppHandle) -> Result<PathBuf, String> {
    app.path()
        .app_config_dir()
        .map(|path| path.join("ui-settings.json"))
        .map_err(|error| error.to_string())
}

fn device_profiles_path() -> Result<PathBuf, String> {
    std::env::var_os("LOCALAPPDATA")
        .map(PathBuf::from)
        .map(|path| path.join("JoyShockMapper").join("devices"))
        .ok_or_else(|| "LOCALAPPDATA is unavailable".to_owned())
}

fn read_settings(app: &tauri::AppHandle) -> UiSettings {
    settings_path(app)
        .ok()
        .and_then(|path| fs::read_to_string(path).ok())
        .and_then(|json| serde_json::from_str(&json).ok())
        .unwrap_or_default()
}

fn prepare_core_directory(directory: &std::path::Path) -> std::io::Result<()> {
    fs::create_dir_all(directory.join("AutoLoad"))?;
    fs::create_dir_all(directory.join("GyroConfigs"))?;
    for (name, contents) in [
        ("OnReset.txt", include_str!("../../../../dist/OnReset.txt")),
        ("OnStartup.txt", include_str!("../../../../dist/OnStartup.txt")),
        ("OnReconnect.txt", include_str!("../../../../dist/OnReconnect.txt")),
    ] {
        let path = directory.join(name);
        if !path.exists() { fs::write(path, contents)?; }
    }
    Ok(())
}

#[tauri::command]
fn get_settings(state: State<'_, SettingsState>) -> Result<UiSettings, String> {
    state.0.lock().map(|value| value.clone()).map_err(|error| error.to_string())
}

#[tauri::command]
fn save_settings(app: tauri::AppHandle, state: State<'_, SettingsState>, settings: UiSettings) -> Result<(), String> {
    let path = settings_path(&app)?;
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).map_err(|error| error.to_string())?;
    }
    fs::write(path, serde_json::to_vec_pretty(&settings).map_err(|error| error.to_string())?)
        .map_err(|error| error.to_string())?;
    *state.0.lock().map_err(|error| error.to_string())? = settings;
    Ok(())
}

#[tauri::command]
fn set_autostart(app: tauri::AppHandle, enabled: bool) -> Result<(), String> {
    if enabled {
        app.autolaunch().enable()
    } else {
        app.autolaunch().disable()
    }
    .map_err(|error| error.to_string())
}

#[tauri::command]
fn list_device_profiles() -> Result<Vec<String>, String> {
    let directory = device_profiles_path()?;
    if !directory.exists() { return Ok(Vec::new()); }
    let mut profiles = fs::read_dir(directory)
        .map_err(|error| error.to_string())?
        .filter_map(Result::ok)
        .filter_map(|entry| entry.file_name().into_string().ok())
        .filter(|name| is_profile_id(name))
        .collect::<Vec<_>>();
    profiles.sort();
    Ok(profiles)
}

#[tauri::command]
fn load_device_profile(id: String) -> Result<DeviceProfile, String> {
    if !is_profile_id(&id) { return Err("invalid device profile id".into()); }
    let source = fs::read_to_string(device_profiles_path()?.join(&id)).map_err(|error| error.to_string())?;
    Ok(DeviceProfile::parse(id, &source))
}

#[tauri::command]
fn save_device_profile(profile: DeviceProfile) -> Result<(), String> {
    let serialized = profile.serialize()?;
    let directory = device_profiles_path()?;
    fs::create_dir_all(&directory).map_err(|error| error.to_string())?;
    let destination = directory.join(&profile.id);
    let temporary = directory.join(format!("{}.tmp", profile.id));
    fs::write(&temporary, serialized).map_err(|error| error.to_string())?;
    if destination.exists() { fs::remove_file(&destination).map_err(|error| error.to_string())?; }
    fs::rename(temporary, destination).map_err(|error| error.to_string())
}

#[cfg(windows)]
fn apply_profile_to_core(core: &embedded_core::EmbeddedCore, profile: &DeviceProfile) -> Result<(), String> {
    for mapping in &profile.mappings {
        core.command(&format!("{} = NONE", mapping.button))?;
        core.command(&format!("{0},{0} = NONE", mapping.button))?;
        if !mapping.single.is_empty() { core.command(&format!("{} = {}", mapping.button, mapping.single))?; }
        if !mapping.double.is_empty() { core.command(&format!("{0},{0} = {1}", mapping.button, mapping.double))?; }
    }
    Ok(())
}

#[tauri::command]
fn save_and_apply_device_profile(
    profile: DeviceProfile,
    #[cfg(windows)] core: State<'_, embedded_core::EmbeddedCore>,
) -> Result<(), String> {
    #[cfg(windows)]
    for mapping in &profile.mappings {
        if !mapping.single.is_empty() { core.validate_mapping(&mapping.single)?; }
        if !mapping.double.is_empty() { core.validate_mapping(&mapping.double)?; }
    }
    #[cfg(windows)]
    {
        let active_profiles = core.active_profile_ids();
        if active_profiles.is_empty() {
            save_device_profile(profile.clone())?;
        } else {
            for id in active_profiles {
                let mut active_profile = profile.clone();
                active_profile.id = id;
                save_device_profile(active_profile)?;
            }
        }
    }
    #[cfg(not(windows))]
    save_device_profile(profile.clone())?;
    #[cfg(windows)]
    apply_profile_to_core(&core, &profile)?;
    Ok(())
}

#[tauri::command]
fn load_profile_into_input_engine(state: State<'_, InputEngineState>, profile: DeviceProfile) -> Result<(), String> {
    profile.validate()?;
    let mut engine = InputEngine::new(200);
    for mapping in profile.mappings {
        engine.bind(mapping.button, Binding { single: mapping.single, double: mapping.double });
    }
    *state.0.lock().map_err(|error| error.to_string())? = engine;
    Ok(())
}

#[tauri::command]
fn preview_button_event(
    state: State<'_, InputEngineState>, button: String, pressed: bool, now_ms: u64,
) -> Result<Vec<TriggeredAction>, String> {
    let mut engine = state.0.lock().map_err(|error| error.to_string())?;
    let actions = if pressed { engine.press(&button, now_ms) } else { engine.release(&button); Vec::new() };
    #[cfg(windows)]
    for action in &actions { output::execute_command(&action.command)?; }
    Ok(actions)
}

#[tauri::command]
fn preview_tick(state: State<'_, InputEngineState>, now_ms: u64) -> Result<Vec<TriggeredAction>, String> {
    let actions = state.0.lock().map_err(|error| error.to_string())?.tick(now_ms);
    #[cfg(windows)]
    for action in &actions { output::execute_command(&action.command)?; }
    Ok(actions)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
	#[cfg(windows)]
	{
		// Single-instance guard: a second launch would otherwise show a
		// duplicate tray icon and fight the first process for the controllers.
		if !acquire_single_instance() {
			std::process::exit(0);
		}
	}
	let background_start = std::env::args_os().any(|argument| argument == "--background");
	let quit_requested = Arc::new(AtomicBool::new(false));
	let tray_quit_requested = quit_requested.clone();
	let app = tauri::Builder::default()
        .plugin(tauri_plugin_process::init())
        .plugin(tauri_plugin_autostart::Builder::new().arg("--background").build())
        .setup(move |app| {
            let startup_settings = read_settings(app.handle());
            if startup_settings.start_with_windows {
                // Refresh legacy startup entries so existing installations also
                // gain the background flag after upgrading.
                let _ = app.autolaunch().disable();
                let _ = app.autolaunch().enable();
            }
            app.manage(SettingsState(Mutex::new(startup_settings)));
            app.manage(InputEngineState(Mutex::new(InputEngine::new(200))));
			#[cfg(windows)]
			{
				special_actions::start();
				let core = embedded_core::EmbeddedCore::default();
				let data_dir = app.path().app_data_dir()?.join("core");
				prepare_core_directory(&data_dir)?;
				core.start(&data_dir.to_string_lossy()).map_err(std::io::Error::other)?;
				let settings = read_settings(app.handle());
				let fly = settings.fly_mouse;
				let button = serde_json::to_value(fly.hold_button)?.as_str().unwrap_or("ZL").to_owned();
				apply_fly_mouse_settings(&core, fly.enabled, &button, fly.sensitivity, fly.precision_sensitivity, fly.response_threshold, fly.smoothing_threshold).map_err(std::io::Error::other)?;
				app.manage(core);
				app.manage(dji_mic::DjiMicMonitor::start());
			}
            let open = MenuItem::with_id(app, "open", "打开 / Open JoyShockMapper", true, None::<&str>)?;
            let quit = MenuItem::with_id(app, "quit", "退出 / Quit", true, None::<&str>)?;
            let menu = Menu::with_items(app, &[&open, &quit])?;
            let mut tray = TrayIconBuilder::new()
                .tooltip("JoyShockMapper")
                .menu(&menu)
                .show_menu_on_left_click(false)
                .on_menu_event(move |app, event| match event.id.as_ref() {
                    "open" => show_main_window(app),
					"quit" => {
						tray_quit_requested.store(true, Ordering::Release);
						app.exit(0);
					},
                    _ => {}
                })
                .on_tray_icon_event(|tray, event| {
                    if let TrayIconEvent::Click {
                        button: MouseButton::Left,
                        button_state: MouseButtonState::Up,
                        ..
                    } = event
                    {
                        show_main_window(tray.app_handle());
                    }
                });
            if let Some(icon) = app.default_window_icon() {
                tray = tray.icon(icon.clone());
            }
            tray.build(app)?;
			if !background_start {
				show_main_window(app.handle());
			}

			// Windows does not guarantee a useful resize event when the native
			// minimize button is used. Poll the native state at low frequency so
			// minimizing to tray is deterministic across WebView2 versions.
			let app_handle = app.handle().clone();
			std::thread::spawn(move || loop {
				std::thread::sleep(std::time::Duration::from_millis(120));
				let Some(window) = app_handle.get_webview_window("main") else { break };
				let should_hide = app_handle.state::<SettingsState>().0.lock()
					.map(|settings| settings.minimize_to_tray).unwrap_or(false);
				if should_hide && window.is_minimized().unwrap_or(false) {
					let _ = window.hide();
				}
			});
            Ok(())
        })
        .on_window_event(|window, event| {
			if let tauri::WindowEvent::CloseRequested { api, .. } = event {
				api.prevent_close();
				let _ = window.hide();
				return;
			}
            if matches!(event, tauri::WindowEvent::Resized(_) | tauri::WindowEvent::Focused(false)) {
                let should_hide = window
                    .state::<SettingsState>()
                    .0
                    .lock()
                    .map(|settings| settings.minimize_to_tray)
                    .unwrap_or(false);
                if should_hide && window.is_minimized().unwrap_or(false) {
                    let _ = window.hide();
                }
            }
        })
        .invoke_handler(tauri::generate_handler![
            get_settings, save_settings, set_autostart,
            list_device_profiles, load_device_profile, save_device_profile,
			save_and_apply_device_profile,
            load_profile_into_input_engine, preview_button_event, preview_tick
			, get_core_status, reconnect_controllers
			, configure_fly_mouse
			, get_gyro_debug_status, calibrate_gyro
			, get_button_states
			, create_launch_focus_action
			, get_dji_mic_status
			, set_dji_mic_setting
        ])
		.build(tauri::generate_context!())
		.expect("failed to build JoyShockMapper Tauri shell");
	app.run(move |app_handle, event| {
		if let tauri::RunEvent::ExitRequested { api, .. } = event {
			if !quit_requested.load(Ordering::Acquire) {
				api.prevent_exit();
			} else {
				#[cfg(windows)]
				app_handle.state::<embedded_core::EmbeddedCore>().stop();
				#[cfg(windows)]
				app_handle.state::<dji_mic::DjiMicMonitor>().stop();
			}
		}
	});
}

fn show_main_window(app: &tauri::AppHandle) {
    if let Some(window) = app.get_webview_window("main") {
        let _ = window.show();
        let _ = window.unminimize();
        let _ = window.set_focus();
    }
}

#[cfg(windows)]
fn acquire_single_instance() -> bool {
    use windows_sys::Win32::Foundation::{ERROR_ALREADY_EXISTS, GetLastError};
    use windows_sys::Win32::System::Threading::CreateMutexW;
    static MUTEX_HANDLE: std::sync::OnceLock<isize> = std::sync::OnceLock::new();
    // Keep the handle alive for the lifetime of the process so the named mutex
    // is not released (which would let a second instance start).
    let wide_name = encode_wide("JoyShockMapper_GlobalSingleInstance");
    let handle = unsafe {
        CreateMutexW(std::ptr::null(), 0, wide_name.as_ptr())
    };
    if handle.is_null() {
        return true; // Failed to create the guard; allow startup rather than block it.
    }
    let _ = MUTEX_HANDLE.set(handle as isize);
    unsafe { GetLastError() != ERROR_ALREADY_EXISTS }
}

#[cfg(windows)]
fn encode_wide(value: &str) -> Vec<u16> {
    value.encode_utf16().chain(std::iter::once(0)).collect()
}
