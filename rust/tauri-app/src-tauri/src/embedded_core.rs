use serde::Serialize;
use std::{ffi::CString, sync::{Arc, Mutex}, thread::JoinHandle};

unsafe extern "C" {
    fn jsm_core_run(working_directory: *const std::ffi::c_char) -> i32;
    fn jsm_core_stop();
    fn jsm_core_submit_command(command: *const std::ffi::c_char);
    fn jsm_core_device_count() -> i32;
    fn jsm_core_device_mask() -> i32;
    fn jsm_core_ready() -> bool;
    fn jsm_core_active_profile_hash(split_type: i32) -> u64;
    #[cfg(test)] fn jsm_core_set_startup_virtual_controller(enabled: bool);
    #[cfg(test)] fn jsm_core_set_profile_directory(directory: *const std::ffi::c_char);
    #[cfg(test)] fn jsm_core_mapping_matches(button: *const std::ffi::c_char, command: *const std::ffi::c_char) -> bool;
    fn jsm_core_gyro_x() -> f32;
    fn jsm_core_gyro_y() -> f32;
    fn jsm_core_gyro_output_x() -> f32;
    fn jsm_core_gyro_output_y() -> f32;
    fn jsm_core_gyro_active() -> i32;
    fn jsm_core_gyro_sample_count() -> u64;
    fn jsm_core_validate_mapping(command: *const std::ffi::c_char) -> bool;
    #[cfg(test)] fn jsm_core_mouse_event_count() -> i32;
    fn jsm_core_left_buttons() -> i32;
    fn jsm_core_right_buttons() -> i32;
    fn jsm_core_left_connected() -> i32;
    fn jsm_core_right_connected() -> i32;
    fn jsm_core_left_trigger() -> f32;
    fn jsm_core_right_trigger() -> f32;
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{fs, time::{Duration, Instant}};

    fn wait_until(timeout: Duration, predicate: impl Fn() -> bool) -> bool {
        let deadline = Instant::now() + timeout;
        while Instant::now() < deadline {
            if predicate() { return true; }
            std::thread::sleep(Duration::from_millis(20));
        }
        false
    }

    #[test]
    fn embedded_cpp_core_loads_persisted_profile_on_startup_and_stops() {
        let directory = std::env::temp_dir().join(format!("jsm-core-test-{}", std::process::id()));
        fs::create_dir_all(&directory).unwrap();
        fs::write(directory.join("OnReset.txt"), "GYRO_SENS = 1\n").unwrap();
        fs::write(directory.join("OnStartup.txt"), "AUTOCONNECT = OFF\n").unwrap();
        let profile_directory = directory.join("devices");
        fs::create_dir_all(&profile_directory).unwrap();
        let expected_profile = "device_78ad0b61b0557e17.jsmprofile";
        fs::write(profile_directory.join(expected_profile), "# persisted profile\nN\tF24\n").unwrap();
        let profile_directory_c = CString::new(profile_directory.to_string_lossy().as_bytes()).unwrap();
        unsafe {
            jsm_core_set_profile_directory(profile_directory_c.as_ptr());
            jsm_core_set_startup_virtual_controller(true);
        }
        let core = EmbeddedCore::default();
        core.start(&directory.to_string_lossy()).unwrap();
        assert!(wait_until(Duration::from_secs(5), || core.status().running));
        assert!(core.validate_mapping("LCONTROL\\ LSHIFT\\ A\\").is_ok());
        assert!(core.validate_mapping("\"UI_ACTION FOCUS_INPUT LCONTROL+L\"").is_ok());
        assert!(wait_until(Duration::from_secs(5), || core.status().device_count >= 1));
        assert!(wait_until(Duration::from_secs(2), || !core.active_profile_ids().is_empty()));
        assert!(core.active_profile_ids().iter().any(|id| id == expected_profile));
        // Verify the core's live mapping without submitting a runtime command:
        // this is the close-and-reopen persistence path that regressed.
        let button = CString::new("N").unwrap();
        let command = CString::new("F24").unwrap();
        assert!(unsafe { jsm_core_mapping_matches(button.as_ptr(), command.as_ptr()) });

        core.stop();
        unsafe {
            jsm_core_set_startup_virtual_controller(false);
            jsm_core_set_profile_directory(std::ptr::null());
        }
        assert!(!core.status().running);
    }

    #[test]
    fn embedded_output_counters_are_callable() {
        assert!(unsafe { jsm_core_mouse_event_count() } >= 0);
    }
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct CoreStatus {
    pub running: bool,
    pub device_count: i32,
    pub left_connected: bool,
    pub right_connected: bool,
    pub active_profile_ids: Vec<String>,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct GyroDebugStatus {
    pub input_x: f32,
    pub input_y: f32,
    pub output_x: f32,
    pub output_y: f32,
    pub active: bool,
    pub sample_count: u64,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ButtonStates {
    pub left_buttons: u32,
    pub right_buttons: u32,
    pub left_connected: bool,
    pub right_connected: bool,
    pub left_trigger: f32,
    pub right_trigger: f32,
}

#[derive(Default)]
pub struct EmbeddedCore {
    thread: Arc<Mutex<Option<JoinHandle<i32>>>>,
}

impl EmbeddedCore {
    pub fn start(&self, working_directory: &str) -> Result<(), String> {
        let mut slot = self.thread.lock().map_err(|error| error.to_string())?;
        if slot.as_ref().is_some_and(|thread| !thread.is_finished()) { return Ok(()); }
        if let Some(thread) = slot.take() { let _ = thread.join(); }
        let directory = CString::new(working_directory).map_err(|error| error.to_string())?;
        *slot = Some(std::thread::spawn(move || unsafe { jsm_core_run(directory.as_ptr()) }));
        Ok(())
    }

    pub fn status(&self) -> CoreStatus {
        let running = self.thread.lock().ok().and_then(|slot| slot.as_ref().map(|thread| !thread.is_finished())).unwrap_or(false);
        let (device_count, mask) = if running { unsafe { (jsm_core_device_count(), jsm_core_device_mask()) } } else { (0, 0) };
        CoreStatus { running, device_count, left_connected: mask & 1 != 0, right_connected: mask & 2 != 0, active_profile_ids: if running { self.active_profile_ids() } else { Vec::new() } }
    }

    pub fn active_profile_ids(&self) -> Vec<String> {
        let mut ids = (1..=3).filter_map(|split_type| {
            let hash = unsafe { jsm_core_active_profile_hash(split_type) };
            (hash != 0).then(|| format!("device_{hash:016x}.jsmprofile"))
        }).collect::<Vec<_>>();
        ids.sort();
        ids.dedup();
        ids
    }

    pub fn command(&self, command: &str) -> Result<(), String> {
        let command = CString::new(command).map_err(|error| error.to_string())?;
        unsafe { jsm_core_submit_command(command.as_ptr()) };
        Ok(())
    }

    pub fn validate_mapping(&self, mapping: &str) -> Result<(), String> {
        let deadline = std::time::Instant::now() + std::time::Duration::from_secs(3);
        while !unsafe { jsm_core_ready() } {
            if std::time::Instant::now() >= deadline { return Err("按键映射核心尚未准备好，请稍后重试".into()); }
            std::thread::sleep(std::time::Duration::from_millis(20));
        }
        let mapping_c = CString::new(mapping).map_err(|error| error.to_string())?;
        if unsafe { jsm_core_validate_mapping(mapping_c.as_ptr()) } { Ok(()) }
        else { Err(format!("无法识别的按键映射：{mapping}")) }
    }

    pub fn gyro_debug_status(&self) -> GyroDebugStatus {
        GyroDebugStatus {
            input_x: unsafe { jsm_core_gyro_x() },
            input_y: unsafe { jsm_core_gyro_y() },
            output_x: unsafe { jsm_core_gyro_output_x() },
            output_y: unsafe { jsm_core_gyro_output_y() },
            active: unsafe { jsm_core_gyro_active() != 0 },
            sample_count: unsafe { jsm_core_gyro_sample_count() },
        }
    }

    pub fn button_states(&self) -> ButtonStates {
        ButtonStates {
            left_buttons: unsafe { jsm_core_left_buttons() as u32 },
            right_buttons: unsafe { jsm_core_right_buttons() as u32 },
            left_connected: unsafe { jsm_core_left_connected() != 0 },
            right_connected: unsafe { jsm_core_right_connected() != 0 },
            left_trigger: unsafe { jsm_core_left_trigger() },
            right_trigger: unsafe { jsm_core_right_trigger() },
        }
    }

    pub fn stop(&self) {
        let thread = self.thread.lock().ok().and_then(|mut slot| slot.take());
        if let Some(thread) = thread {
            if !thread.is_finished() { unsafe { jsm_core_stop() } }
            let _ = thread.join();
        }
    }
}

impl Drop for EmbeddedCore {
    fn drop(&mut self) { self.stop(); }
}
