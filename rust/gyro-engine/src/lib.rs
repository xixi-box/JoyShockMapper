use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy)]
pub struct GyroSettings {
    pub sensitivity_low: [f32; 2],
    pub sensitivity_high: [f32; 2],
    pub threshold_low: f32,
    pub threshold_high: f32,
    pub cutoff_speed: f32,
    pub cutoff_recovery: f32,
    pub real_world_calibration: f32,
    pub in_game_sensitivity: f32,
    pub os_mouse_speed: f32,
}

impl Default for GyroSettings {
    fn default() -> Self {
        Self {
            sensitivity_low: [1.0, 1.0],
            sensitivity_high: [1.0, 1.0],
            threshold_low: 0.0,
            threshold_high: 0.0,
            cutoff_speed: 0.0,
            cutoff_recovery: 0.0,
            real_world_calibration: 40.0,
            in_game_sensitivity: 1.0,
            os_mouse_speed: 1.0,
        }
    }
}

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct GyroFrame {
    pub dt_seconds: f32,
    pub hold_pressed: bool,
    pub gyro_x: f32,
    pub gyro_y: f32,
    pub gyro_z: f32,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct GyroOutput {
    pub velocity: [f32; 2],
    pub mouse_delta: [f32; 2],
    pub mouse_pixels: [i32; 2],
}

#[derive(Default)]
pub struct GyroEngine {
    accumulated: [f32; 2],
}

impl GyroEngine {
    /// Mirrors the current C++ LOCAL-space mouse path. JoyShockMapper defaults
    /// mouse X to -local Y and mouse Y to -local X.
    pub fn process(&mut self, frame: GyroFrame, settings: GyroSettings) -> GyroOutput {
        let mut gyro = [-frame.gyro_y, -frame.gyro_x];
        let mut magnitude = gyro[0].hypot(gyro[1]);

        if settings.cutoff_recovery > settings.cutoff_speed {
            let factor = (magnitude - settings.cutoff_speed)
                / (settings.cutoff_recovery - settings.cutoff_speed);
            if factor < 1.0 {
                if factor <= 0.0 {
                    gyro = [0.0, 0.0];
                    magnitude = 0.0;
                } else {
                    gyro[0] *= factor;
                    gyro[1] *= factor;
                    magnitude *= factor;
                }
            }
        } else if settings.cutoff_speed > 0.0 && magnitude < settings.cutoff_speed {
            gyro = [0.0, 0.0];
            magnitude = 0.0;
        }

        if !frame.hold_pressed {
            gyro = [0.0, 0.0];
        }

        let scaled_magnitude = (magnitude - settings.threshold_low).max(0.0);
        let threshold_range = settings.threshold_high - settings.threshold_low;
        let blend = if threshold_range <= 0.0 {
            if scaled_magnitude > 0.0 { 1.0 } else { 0.0 }
        } else {
            (scaled_magnitude / threshold_range).min(1.0)
        };
        let velocity = [
            gyro[0]
                * (settings.sensitivity_low[0] * (1.0 - blend)
                    + settings.sensitivity_high[0] * blend),
            gyro[1]
                * (settings.sensitivity_low[1] * (1.0 - blend)
                    + settings.sensitivity_high[1] * blend),
        ];
        let calibration = settings.real_world_calibration
            / settings.os_mouse_speed
            / settings.in_game_sensitivity;
        let mouse_delta = [
            velocity[0] * calibration * frame.dt_seconds,
            velocity[1] * calibration * frame.dt_seconds,
        ];
        self.accumulated[0] += mouse_delta[0];
        self.accumulated[1] += mouse_delta[1];
        let mouse_pixels = [
            self.accumulated[0].trunc() as i32,
            self.accumulated[1].trunc() as i32,
        ];
        self.accumulated[0] -= mouse_pixels[0] as f32;
        self.accumulated[1] -= mouse_pixels[1] as f32;
        GyroOutput {
            velocity,
            mouse_delta,
            mouse_pixels,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hold_gate_stops_output_immediately() {
        let mut engine = GyroEngine::default();
        let output = engine.process(
            GyroFrame {
                dt_seconds: 0.01,
                hold_pressed: false,
                gyro_x: 20.0,
                gyro_y: -30.0,
                gyro_z: 0.0,
            },
            GyroSettings::default(),
        );
        assert_eq!(output.velocity, [0.0, 0.0]);
        assert_eq!(output.mouse_pixels, [0, 0]);
    }

    #[test]
    fn subpixel_remainder_is_preserved() {
        let mut engine = GyroEngine::default();
        let frame = GyroFrame {
            dt_seconds: 0.001,
            hold_pressed: true,
            gyro_x: 0.0,
            gyro_y: -10.0,
            gyro_z: 0.0,
        };
        let outputs = (0..5)
            .map(|_| engine.process(frame, GyroSettings::default()).mouse_pixels[0])
            .collect::<Vec<_>>();
        assert_eq!(outputs, vec![0, 0, 1, 0, 1]);
    }
}
