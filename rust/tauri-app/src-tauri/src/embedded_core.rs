use serde::Serialize;
use std::{ffi::CString, sync::{Arc, Mutex}, thread::JoinHandle};

unsafe extern "C" {
    fn jsm_core_run(working_directory: *const std::ffi::c_char) -> i32;
    fn jsm_core_stop();
    fn jsm_core_submit_command(command: *const std::ffi::c_char);
    fn jsm_core_device_count() -> i32;
    fn jsm_core_device_mask() -> i32;
    #[cfg(test)] fn jsm_core_attach_virtual_controller() -> i32;
    #[cfg(test)] fn jsm_core_set_virtual_button(button: i32, down: bool) -> bool;
    #[cfg(test)] fn jsm_core_set_virtual_motion(
        gyro_x_dps: f32, gyro_y_dps: f32, gyro_z_dps: f32,
        accel_x_g: f32, accel_y_g: f32, accel_z_g: f32,
    ) -> bool;
    #[cfg(test)] fn jsm_core_mouse_event_count() -> i32;
    #[cfg(test)] fn jsm_core_key_event_count() -> i32;
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
    fn embedded_cpp_core_starts_maps_a_virtual_button_and_stops() {
        let directory = std::env::temp_dir().join(format!("jsm-core-test-{}", std::process::id()));
        fs::create_dir_all(&directory).unwrap();
        fs::write(directory.join("OnReset.txt"), "N = F24\nGYRO_SENS = 1\n").unwrap();
        fs::write(directory.join("OnStartup.txt"), "AUTOCONNECT = OFF\n").unwrap();
        let core = EmbeddedCore::default();
        core.start(&directory.to_string_lossy()).unwrap();
        assert!(wait_until(Duration::from_secs(5), || core.status().running));
        assert!(unsafe { jsm_core_attach_virtual_controller() } > 0);
        core.command("RECONNECT_CONTROLLERS").unwrap();
        assert!(wait_until(Duration::from_secs(5), || core.status().device_count == 1));
        for input in ["S", "E", "W", "N"] {
            core.command(&format!("{input} = F24")).unwrap();
        }
        let before = unsafe { jsm_core_key_event_count() };
        for button in 0..4 {
            assert!(unsafe { jsm_core_set_virtual_button(button, true) });
            std::thread::sleep(Duration::from_millis(80));
            assert!(unsafe { jsm_core_set_virtual_button(button, false) });
            std::thread::sleep(Duration::from_millis(30));
        }
        assert!(wait_until(Duration::from_secs(2), || unsafe { jsm_core_key_event_count() } > before));

        core.command("AUTO_CALIBRATE_GYRO = OFF").unwrap();
        core.command("GYRO_SPACE = LOCAL").unwrap();
        core.command("GYRO_SENS = 2").unwrap();
        core.command("GYRO_ON = S").unwrap();
        let mouse_before = unsafe { jsm_core_mouse_event_count() };
        assert!(unsafe { jsm_core_set_virtual_button(0, true) });
        for _ in 0..30 {
            assert!(unsafe { jsm_core_set_virtual_motion(0.0, 90.0, 0.0, 0.0, 0.0, 1.0) });
            std::thread::sleep(Duration::from_millis(8));
        }
        assert!(unsafe { jsm_core_set_virtual_button(0, false) });
        assert!(wait_until(Duration::from_secs(2), || unsafe { jsm_core_mouse_event_count() } > mouse_before));
        core.stop();
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
        CoreStatus { running, device_count, left_connected: mask & 1 != 0, right_connected: mask & 2 != 0 }
    }

    pub fn command(&self, command: &str) -> Result<(), String> {
        let command = CString::new(command).map_err(|error| error.to_string())?;
        unsafe { jsm_core_submit_command(command.as_ptr()) };
        Ok(())
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
