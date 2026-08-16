//! DJI Mic telemetry and whitelisted settings for the WinUSB-bound status interface.
//!
//! Packet framing, CRCs, block scanning and field rules are derived from
//! dji-mic-mo by usokawa_ (BSD-2-Clause):
//! https://github.com/usokawa/dji-mic-mo

use rusb::{Context, DeviceHandle, Error, TransferType, UsbContext};
use serde::Serialize;
use std::{
    sync::{
        atomic::{AtomicBool, Ordering},
        mpsc::{self, Receiver, Sender},
        Arc, Mutex,
    },
    thread::{self, JoinHandle},
    time::Duration,
};

const VID: u16 = 0x2ca3;
const PID: u16 = 0x4011;
const INTERFACE: u8 = 6;
const ENDPOINT_IN: u8 = 0x86;
const ENDPOINT_OUT: u8 = 0x06;

#[derive(Clone, Debug, Default, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct DjiMicStatus {
    pub receiver_connected: bool,
    pub tx1_connected: bool,
    pub battery_level: Option<u8>,
    pub charging: Option<bool>,
    pub input_level: Option<u8>,
    pub receiver: DjiReceiverState,
    pub tx1: Option<DjiTransmitterState>,
    pub transmitter: Option<DjiTransmitterSettings>,
    pub error: Option<String>,
}

#[derive(Clone, Debug, Default, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct DjiReceiverState {
    pub firmware_version: Option<String>,
    pub serial_number: Option<String>,
    pub address_suffix: Option<String>,
    pub device_name: Option<String>,
    pub battery_level: Option<u8>,
    pub charging: Option<bool>,
    pub stereo: Option<bool>,
    pub quadraphonic: Option<bool>,
    pub safety_track: Option<bool>,
    pub gain_control: Option<i8>,
    pub monitoring_gain: Option<i8>,
    pub clipping_control: Option<bool>,
    pub auto_off: Option<bool>,
    pub receiver_on_off_with_camera: Option<bool>,
    pub plug_free_external_speaker: Option<bool>,
}

#[derive(Clone, Debug, Default, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct DjiTransmitterState {
    pub firmware_version: Option<String>,
    pub serial_number: Option<String>,
    pub address_suffix: Option<String>,
    pub device_name: Option<String>,
    pub battery_level: Option<u8>,
    pub charging: Option<bool>,
    pub input_level: Option<u8>,
    pub recording_time_total: Option<f32>,
    pub recording_time_remaining: Option<f32>,
    pub recording: Option<bool>,
    pub transmitter_gain: Option<i8>,
    pub voice_tone_rich: Option<bool>,
    pub voice_tone_bright: Option<bool>,
    pub file_option_edited_file: Option<bool>,
    pub float32_recording: Option<bool>,
    pub startup_auto_recording: Option<bool>,
    pub startup_auto_recording_2s: Option<bool>,
    pub auto_recording_with_receiver: Option<bool>,
    pub low_power_auto_recording: Option<bool>,
    pub loop_recording: Option<bool>,
    pub rec_stop: Option<bool>,
    pub vibration: Option<bool>,
}

#[derive(Clone, Debug, Default, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct DjiTransmitterSettings {
    pub noise_cancellation: Option<bool>,
    pub noise_cancellation_strong: Option<bool>,
    pub noise_cancellation_via_button: Option<bool>,
    pub low_cut: Option<bool>,
    pub clipping_control: Option<bool>,
    pub loudness_balance: Option<bool>,
    pub auto_off: Option<bool>,
    pub mic_led_off: Option<bool>,
}

pub struct DjiMicMonitor {
    status: Arc<Mutex<DjiMicStatus>>,
    stop: Arc<AtomicBool>,
    worker: Mutex<Option<JoinHandle<()>>>,
    commands: Sender<DjiSettingCommand>,
}

#[derive(Debug)]
struct DjiSettingCommand {
    address: u16,
    command: u8,
    value: u8,
}

impl DjiMicMonitor {
    pub fn start() -> Self {
        let status = Arc::new(Mutex::new(DjiMicStatus::default()));
        let stop = Arc::new(AtomicBool::new(false));
        let worker_status = status.clone();
        let worker_stop = stop.clone();
        let (commands, command_rx) = mpsc::channel();
        let worker = thread::spawn(move || monitor_loop(worker_status, worker_stop, command_rx));
        Self {
            status,
            stop,
            worker: Mutex::new(Some(worker)),
            commands,
        }
    }

    pub fn status(&self) -> DjiMicStatus {
        self.status
            .lock()
            .map(|value| value.clone())
            .unwrap_or_default()
    }

    pub fn stop(&self) {
        self.stop.store(true, Ordering::Release);
        if let Ok(mut worker) = self.worker.lock() {
            if let Some(worker) = worker.take() {
                let _ = worker.join();
            }
        }
    }

    pub fn set_setting(&self, node: &str, field: &str, value: i16) -> Result<(), String> {
        let status = self.status();
        if !status.receiver_connected {
            return Err("DJI Mic 接收器未连接".into());
        }
        let receiver_name = status.receiver.device_name.as_deref();
        let tx1_name = status.tx1.as_ref().and_then(|tx| tx.device_name.as_deref());
        let (address, command, encoded) = writable_setting(node, field, value, receiver_name, tx1_name)?;
        self.commands
            .send(DjiSettingCommand { address, command, value: encoded })
            .map_err(|_| "DJI Mic 状态线程未运行".to_owned())
    }
}

impl Drop for DjiMicMonitor {
    fn drop(&mut self) {
        self.stop();
    }
}

fn monitor_loop(
    status: Arc<Mutex<DjiMicStatus>>,
    stop: Arc<AtomicBool>,
    commands: Receiver<DjiSettingCommand>,
) {
    let context = match Context::new() {
        Ok(context) => context,
        Err(error) => {
            set_error(&status, format!("libusb 初始化失败：{error}"));
            return;
        }
    };
    while !stop.load(Ordering::Acquire) {
        match open_exact_interface(&context) {
            Ok(Some((mut handle, transfer_type))) => {
                update(&status, |value| {
                    value.receiver_connected = true;
                    value.error = None;
                });
                read_device(&mut handle, transfer_type, &status, &stop, &commands);
                // Explicit release is attempted on every unplug/exit path. Dropping the
                // handle is the fallback and does not touch any other USB interface.
                let _ = handle.release_interface(INTERFACE);
                update(&status, disconnected);
            }
            Ok(None) => update(&status, disconnected),
            Err(error) => {
                update(&status, |value| {
                    disconnected(value);
                    value.error = Some(format!("无法读取 DJI Mic Interface 6：{error}"));
                });
            }
        }
        wait_or_stop(&stop, Duration::from_millis(600));
    }
    update(&status, disconnected);
}

fn open_exact_interface(
    context: &Context,
) -> Result<Option<(DeviceHandle<Context>, TransferType)>, Error> {
    for device in context.devices()?.iter() {
        let descriptor = device.device_descriptor()?;
        if descriptor.vendor_id() != VID || descriptor.product_id() != PID {
            continue;
        }
        let transfer_type = device
            .active_config_descriptor()?
            .interfaces()
            .filter(|interface| interface.number() == INTERFACE)
            .flat_map(|interface| interface.descriptors())
            .flat_map(|descriptor| descriptor.endpoint_descriptors())
            .find(|endpoint| endpoint.address() == ENDPOINT_IN)
            .map(|endpoint| endpoint.transfer_type())
            .ok_or(Error::NotFound)?;
        if !matches!(transfer_type, TransferType::Bulk | TransferType::Interrupt) {
            return Err(Error::NotSupported);
        }
        let handle = device.open()?;
        // Intentionally no set_configuration, set_alt_setting, or detach/attach.
        handle.claim_interface(INTERFACE)?;
        return Ok(Some((handle, transfer_type)));
    }
    Ok(None)
}

fn read_device(
    handle: &mut DeviceHandle<Context>,
    transfer_type: TransferType,
    status: &Arc<Mutex<DjiMicStatus>>,
    stop: &AtomicBool,
    commands: &Receiver<DjiSettingCommand>,
) {
    let mut framing = PacketFraming::default();
    let mut chunk = [0u8; 1024];
    let mut sequence = 0u16;
    while !stop.load(Ordering::Acquire) {
        while let Ok(command) = commands.try_recv() {
            let packet = setting_packet(sequence, &command);
            sequence = sequence.wrapping_add(1);
            let result = match transfer_type {
                TransferType::Bulk => handle.write_bulk(ENDPOINT_OUT, &packet, Duration::from_secs(1)),
                TransferType::Interrupt => handle.write_interrupt(ENDPOINT_OUT, &packet, Duration::from_secs(1)),
                _ => unreachable!(),
            };
            if result != Ok(packet.len()) {
                set_error(status, format!("DJI Mic 设置写入失败：{:?}", result.err()));
            }
        }
        let result = match transfer_type {
            TransferType::Bulk => {
                handle.read_bulk(ENDPOINT_IN, &mut chunk, Duration::from_millis(200))
            }
            TransferType::Interrupt => {
                handle.read_interrupt(ENDPOINT_IN, &mut chunk, Duration::from_millis(200))
            }
            _ => unreachable!(),
        };
        match result {
            Ok(size) => {
                for packet in framing.push(&chunk[..size]) {
                    parse_packet(&packet, status);
                }
            }
            Err(Error::Timeout) => {}
            Err(_) => break,
        }
    }
}

#[derive(Default)]
struct PacketFraming {
    buffer: Vec<u8>,
}

impl PacketFraming {
    fn push(&mut self, bytes: &[u8]) -> Vec<Vec<u8>> {
        self.buffer.extend_from_slice(bytes);
        let mut packets = Vec::new();
        let mut offset = 0;
        while offset < self.buffer.len() {
            let Some(relative) = self.buffer[offset..].iter().position(|byte| *byte == 0x55) else {
                offset = self.buffer.len();
                break;
            };
            offset += relative;
            if self.buffer.len() - offset < 4 {
                break;
            }
            if crc8(&self.buffer[offset..offset + 3]) != self.buffer[offset + 3] {
                offset += 1;
                continue;
            }
            let size = u16::from_le_bytes([self.buffer[offset + 1], self.buffer[offset + 2]])
                as usize
                & 0x03ff;
            if size < 13 {
                offset += 1;
                continue;
            }
            if size > self.buffer.len() - offset {
                break;
            }
            let packet = &self.buffer[offset..offset + size];
            let expected = u16::from_le_bytes([packet[size - 2], packet[size - 1]]);
            if crc16(&packet[..size - 2]) != expected {
                offset += 1;
                continue;
            }
            packets.push(packet.to_vec());
            offset += size;
        }
        self.buffer.drain(..offset);
        packets
    }
}

fn parse_packet(packet: &[u8], status: &Arc<Mutex<DjiMicStatus>>) {
    if packet.len() < 13 || packet[9] != 0x5b || packet[10] != 0x03 {
        return;
    }
    let data = &packet[11..packet.len() - 2];
    let Some(&kind) = data.get(3) else { return };
    let tx1 = scan_tx1(data, kind);
    update(status, |value| {
        parse_receiver(data, kind, &mut value.receiver);
        if matches!(kind, 0x01 | 0x03) {
            value.tx1_connected = tx1.is_some();
            if tx1.is_none() {
                value.battery_level = None;
                value.charging = None;
                value.input_level = None;
                value.tx1 = None;
                value.transmitter = None;
            }
        }
        let Some(base) = tx1 else { return };
        value.tx1_connected = true;
        let (battery_level, charging, input_level) = {
            let tx1_state = value.tx1.get_or_insert_with(DjiTransmitterState::default);
            parse_tx1(data, kind, base, tx1_state);
            (
                tx1_state.battery_level,
                tx1_state.charging,
                tx1_state.input_level,
            )
        };
        let transmitter = value
            .transmitter
            .get_or_insert_with(DjiTransmitterSettings::default);
        parse_transmitter_settings(data, kind, base, transmitter);
        value.battery_level = battery_level;
        value.charging = charging;
        value.input_level = input_level;
    });
}

fn parse_receiver(data: &[u8], kind: u8, receiver: &mut DjiReceiverState) {
    match kind {
        0x01 => {
            receiver.firmware_version = version_at(data, 9);
            receiver.serial_number = fixed_string(data, 13, 14);
            receiver.address_suffix = fixed_string(data, 33, 6);
            receiver.device_name = variable_string(data, 45);
        }
        0x03 => {
            let Some(state) = byte_at(data, 10) else { return };
            if receiver.device_name.as_deref() != Some("DJI Mic Mini 2") {
                receiver.battery_level = Some((state >> 5) & 0x07);
                receiver.charging = Some(state & 0x10 != 0);
                receiver.auto_off = Some(state & 0x01 != 0);
                receiver.receiver_on_off_with_camera =
                    byte_at(data, 9).map(|flags| flags & 0x80 != 0);
            }
            receiver.stereo = Some(state & 0x04 != 0);
            if receiver.device_name.as_deref() == Some("DJI Mic Mini 2S") {
                receiver.quadraphonic = Some(state & 0x08 != 0);
            }
            receiver.gain_control = signed_at(data, 11);
            if receiver.device_name.as_deref() == Some("DJI Mic Mini 2") {
                receiver.monitoring_gain = signed_at(data, 16);
            }
            if let Some(options) = byte_at(data, 37) {
                receiver.safety_track = Some(options & 0x40 != 0);
                receiver.clipping_control = Some(options & 0x10 != 0);
                receiver.plug_free_external_speaker = Some(options & 0x02 != 0);
            }
        }
        _ => {}
    }
    normalize_receiver(receiver);
}

fn parse_tx1(data: &[u8], kind: u8, base: usize, tx: &mut DjiTransmitterState) {
    match kind {
        0x01 => {
            tx.firmware_version = version_at(data, base + 6);
            tx.serial_number = fixed_string(data, base + 10, 14);
            tx.address_suffix = fixed_string(data, base + 30, 6);
            tx.device_name = variable_string(data, base + 42);
        }
        0x03 => {
            if let Some(state) = byte_at(data, base + 7) {
                tx.battery_level = Some((state >> 2) & 0x07);
                tx.charging = Some(state & 0x02 != 0);
            }
            let mini_2s = tx.device_name.as_deref() == Some("DJI Mic Mini 2S");
            if mini_2s {
                tx.recording_time_total = fixed16_at(data, base + 14);
                tx.recording_time_remaining = fixed16_at(data, base + 16);
                tx.transmitter_gain = signed_at(data, base + 13);
            }
            if let Some(flags) = byte_at(data, base + 9) {
                if mini_2s {
                    tx.recording = Some(flags & 0x10 != 0);
                    tx.float32_recording = Some(flags & 0x08 != 0);
                    tx.vibration = Some(flags & 0x01 != 0);
                }
                if tx.device_name.as_deref() != Some("DJI Mic Mini") {
                    tx.voice_tone_rich = Some(flags & 0x40 != 0);
                    tx.voice_tone_bright = Some(flags & 0x80 != 0);
                }
            }
            if mini_2s && let Some(flags) = byte_at(data, base + 8) {
                tx.startup_auto_recording = Some(flags & 0x80 != 0);
            }
            if mini_2s && let Some(flags) = byte_at(data, base + 11) {
                tx.low_power_auto_recording = Some(flags & 0x10 != 0);
                tx.loop_recording = Some(flags & 0x08 != 0);
                tx.rec_stop = Some(flags & 0x20 != 0);
            }
            if mini_2s && let Some(flags) = byte_at(data, base + 31) {
                tx.file_option_edited_file = Some(flags & 0x80 != 0);
                tx.startup_auto_recording_2s = Some(flags & 0x10 != 0);
                tx.auto_recording_with_receiver = Some(flags & 0x20 != 0);
            }
        }
        0x05 => tx.input_level = byte_at(data, base + 6),
        _ => {}
    }
    normalize_tx1(tx);
}

fn normalize_receiver(receiver: &mut DjiReceiverState) {
    match receiver.device_name.as_deref() {
        Some("DJI Mic Mini 2") => {
            receiver.battery_level = None;
            receiver.charging = None;
            receiver.quadraphonic = None;
            receiver.auto_off = None;
            receiver.receiver_on_off_with_camera = None;
        }
        Some("DJI Mic Mini 2S") => receiver.monitoring_gain = None,
        Some(_) => {
            receiver.quadraphonic = None;
            receiver.monitoring_gain = None;
        }
        None => {}
    }
}

fn normalize_tx1(tx: &mut DjiTransmitterState) {
    if tx.device_name.as_deref() == Some("DJI Mic Mini") {
        tx.voice_tone_rich = None;
        tx.voice_tone_bright = None;
    }
    if tx.device_name.as_deref().is_some_and(|name| name != "DJI Mic Mini 2S") {
        tx.recording_time_total = None;
        tx.recording_time_remaining = None;
        tx.recording = None;
        tx.transmitter_gain = None;
        tx.file_option_edited_file = None;
        tx.float32_recording = None;
        tx.startup_auto_recording = None;
        tx.startup_auto_recording_2s = None;
        tx.auto_recording_with_receiver = None;
        tx.low_power_auto_recording = None;
        tx.loop_recording = None;
        tx.rec_stop = None;
        tx.vibration = None;
    }
}

fn parse_transmitter_settings(
    data: &[u8],
    kind: u8,
    base: usize,
    settings: &mut DjiTransmitterSettings,
) {
    if kind != 0x03 {
        return;
    }
    if let Some(flags) = byte_at(data, base + 6) {
        settings.noise_cancellation_strong = Some(flags & 0x20 != 0);
        settings.noise_cancellation_via_button = Some(flags & 0x80 != 0);
        settings.auto_off = Some(flags & 0x10 != 0);
        settings.mic_led_off = Some(flags & 0x02 != 0);
    }
    if let Some(flags) = byte_at(data, base + 7) {
        settings.noise_cancellation = Some(flags & 0x01 != 0);
    }
    settings.clipping_control = byte_at(data, base + 8).map(|flags| flags & 0x04 != 0);
    settings.low_cut = byte_at(data, base + 9).map(|flags| flags & 0x20 != 0);
    settings.loudness_balance = byte_at(data, base + 11).map(|flags| flags & 0x80 != 0);
}

fn byte_at(data: &[u8], offset: usize) -> Option<u8> {
    data.get(offset).copied()
}

fn signed_at(data: &[u8], offset: usize) -> Option<i8> {
    byte_at(data, offset).map(|value| value as i8)
}

fn fixed16_at(data: &[u8], offset: usize) -> Option<f32> {
    let bytes = data.get(offset..offset + 2)?;
    Some(u16::from_le_bytes([bytes[0], bytes[1]]) as f32 / 10.0)
}

fn version_at(data: &[u8], offset: usize) -> Option<String> {
    let bytes = data.get(offset..offset + 4)?;
    Some(
        bytes
            .iter()
            .rev()
            .map(|byte| format!("{byte:02}"))
            .collect::<Vec<_>>()
            .join("."),
    )
}

fn fixed_string(data: &[u8], offset: usize, size: usize) -> Option<String> {
    clean_string(data.get(offset..offset + size)?)
}

fn variable_string(data: &[u8], offset: usize) -> Option<String> {
    let size = *data.get(offset.checked_sub(1)?)? as usize;
    clean_string(data.get(offset..offset + size)?)
}

fn clean_string(bytes: &[u8]) -> Option<String> {
    let value = String::from_utf8_lossy(bytes)
        .trim_matches(char::from(0))
        .trim()
        .to_owned();
    (!value.is_empty()).then_some(value)
}

fn scan_tx1(data: &[u8], kind: u8) -> Option<usize> {
    match kind {
        0x01 if data.len() >= 45 => {
            let mut offset = 45usize.checked_add(*data.get(44)? as usize)?;
            while offset + 42 <= data.len() {
                if data[offset] == 0x01 && data[offset + 1] == 0x01 {
                    return Some(offset);
                }
                offset = offset.checked_add(42 + data[offset + 41] as usize)?;
            }
            None
        }
        0x03 if data.len() >= 41 => (41..data.len().saturating_sub(31))
            .step_by(32)
            .find(|offset| data[*offset] == 0x02 && data[*offset + 1] == 0x01),
        0x05 if data.len() >= 10 => (3..data.len().saturating_sub(6))
            .step_by(7)
            .find(|offset| data[*offset] == 0x05 && data[*offset + 1] == 0x01),
        _ => None,
    }
}

fn writable_setting(
    node: &str,
    field: &str,
    value: i16,
    receiver_name: Option<&str>,
    tx1_name: Option<&str>,
) -> Result<(u16, u8, u8), String> {
    let boolean = |true_value: u8| match value {
        0 => Ok(0),
        1 => Ok(true_value),
        _ => Err("开关值必须是 0 或 1".to_owned()),
    };
    let gain = || {
        i8::try_from(value)
            .ok()
            .filter(|gain| (-12..=12).contains(gain))
            .map(|gain| gain as u8)
            .ok_or_else(|| "增益必须是 -12 到 12 的整数".to_owned())
    };
    let stepped_gain = || {
        [-12i16, -6, 0, 6, 12]
            .contains(&value)
            .then_some(value as i8 as u8)
            .ok_or_else(|| "接收增益仅支持 -12、-6、0、6、12".to_owned())
    };
    let receiver_known = receiver_name.ok_or_else(|| "尚未读取到接收器型号，不能安全写入".to_owned())?;
    match (node, field) {
        ("rx", "stereo") => Ok((0x0000, 0x08, boolean(2)?)),
        ("rx", "safetyTrack") => Ok((0x0000, 0x21, boolean(1)?)),
        ("rx", "gainControl") if receiver_known == "DJI Mic Mini 2" => {
            Ok((0x0000, 0x39, stepped_gain()?))
        }
        ("rx", "monitoringGain") if receiver_known == "DJI Mic Mini 2" => {
            Ok((0x0000, 0x26, gain()?))
        }
        ("rx", "clippingControl") => Ok((0x0000, 0x1e, boolean(1)?)),
        ("rx", "autoOff") if receiver_known != "DJI Mic Mini 2" => {
            Ok((0x0000, 0x10, boolean(1)?))
        }
        ("rx", "receiverOnOffWithCamera") if receiver_known != "DJI Mic Mini 2" => {
            Ok((0x0000, 0x20, boolean(1)?))
        }
        ("rx", "plugFreeExternalSpeaker") => Ok((0x0000, 0x23, boolean(1)?)),
        ("tx", "noiseCancellation") if receiver_known != "DJI Mic Mini 2" => {
            Ok((0xffff, 0x38, boolean(1)?))
        }
        ("tx", "noiseCancellationStrong") if receiver_known != "DJI Mic Mini 2" => {
            Ok((0xffff, 0x37, boolean(1)?))
        }
        ("tx", "noiseCancellationViaButton") if receiver_known != "DJI Mic Mini 2" => {
            Ok((0xffff, 0x0f, boolean(1)?))
        }
        ("tx", "lowCut") => Ok((0xffff, 0x03, boolean(1)?)),
        ("tx", "clippingControl") => Ok((0xffff, 0x24, boolean(1)?)),
        ("tx", "loudnessBalance") => Ok((0xffff, 0x2c, boolean(2)?)),
        ("tx", "autoOff") => Ok((0xffff, 0x10, boolean(1)?)),
        ("tx", "micLedOff") => Ok((0xffff, 0x0a, boolean(2)?)),
        ("tx1", "recording") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x02, boolean(1)?))
        }
        ("tx1", "transmitterGain") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x39, gain()?))
        }
        ("tx1", "voiceToneRich") if tx1_name != Some("DJI Mic Mini") => {
            Ok((0x0001, 0x29, boolean(1)?))
        }
        ("tx1", "voiceToneBright") if tx1_name != Some("DJI Mic Mini") => {
            Ok((0x0001, 0x29, boolean(2)?))
        }
        ("tx1", "fileOptionEditedFile") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x3d, boolean(2)?))
        }
        ("tx1", "float32Recording") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x0c, boolean(1)?))
        }
        ("tx1", "startupAutoRecording") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x2e, boolean(1)?))
        }
        ("tx1", "startupAutoRecording2s") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x3e, boolean(1)?))
        }
        ("tx1", "autoRecordingWithReceiver") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x3e, boolean(2)?))
        }
        ("tx1", "lowPowerAutoRecording") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x2b, boolean(1)?))
        }
        ("tx1", "loopRecording") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x2a, boolean(1)?))
        }
        ("tx1", "recStop") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x0b, boolean(1)?))
        }
        ("tx1", "vibration") if tx1_name == Some("DJI Mic Mini 2S") => {
            Ok((0x0001, 0x04, boolean(1)?))
        }
        _ => Err("当前设备型号不支持修改此参数".to_owned()),
    }
}

fn setting_packet(sequence: u16, command: &DjiSettingCommand) -> [u8; 22] {
    let mut packet = [0u8; 22];
    packet[0] = 0x55;
    packet[1..3].copy_from_slice(&[0x16, 0x04]);
    packet[3] = crc8(&packet[..3]);
    packet[4..6].copy_from_slice(&[0x02, 0x5a]);
    packet[6..8].copy_from_slice(&sequence.to_le_bytes());
    packet[8..20].copy_from_slice(&[
        0x40,
        0x5b,
        0x01,
        0x02,
        command.address as u8,
        (command.address >> 8) as u8,
        0x00,
        0x00,
        command.command,
        0x00,
        0x01,
        command.value,
    ]);
    let checksum = crc16(&packet[..20]);
    packet[20..].copy_from_slice(&checksum.to_le_bytes());
    packet
}

fn crc8(bytes: &[u8]) -> u8 {
    bytes.iter().fold(0x77, |mut crc, byte| {
        crc ^= byte;
        for _ in 0..8 {
            crc = (crc >> 1) ^ if crc & 1 != 0 { 0x8c } else { 0 };
        }
        crc
    })
}

fn crc16(bytes: &[u8]) -> u16 {
    bytes.iter().fold(0x3692, |mut crc, byte| {
        crc ^= *byte as u16;
        for _ in 0..8 {
            crc = (crc >> 1) ^ if crc & 1 != 0 { 0x8408 } else { 0 };
        }
        crc
    })
}

fn disconnected(value: &mut DjiMicStatus) {
    value.receiver_connected = false;
    value.tx1_connected = false;
    value.battery_level = None;
    value.charging = None;
    value.input_level = None;
    value.receiver = DjiReceiverState::default();
    value.tx1 = None;
    value.transmitter = None;
    value.error = None;
}

fn set_error(status: &Arc<Mutex<DjiMicStatus>>, error: String) {
    update(status, |value| value.error = Some(error));
}

fn update(status: &Arc<Mutex<DjiMicStatus>>, action: impl FnOnce(&mut DjiMicStatus)) {
    if let Ok(mut value) = status.lock() {
        action(&mut value);
    }
}

fn wait_or_stop(stop: &AtomicBool, duration: Duration) {
    let slices = duration.as_millis() / 50;
    for _ in 0..slices {
        if stop.load(Ordering::Acquire) {
            break;
        }
        thread::sleep(Duration::from_millis(50));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::Instant;

    fn packet(data: &[u8]) -> Vec<u8> {
        let size = 11 + data.len() + 2;
        let mut packet = vec![0u8; size];
        packet[0] = 0x55;
        let header = (size as u16) | 0x0400;
        packet[1..3].copy_from_slice(&header.to_le_bytes());
        packet[3] = crc8(&packet[..3]);
        packet[9] = 0x5b;
        packet[10] = 0x03;
        packet[11..11 + data.len()].copy_from_slice(data);
        let checksum = crc16(&packet[..size - 2]);
        packet[size - 2..].copy_from_slice(&checksum.to_le_bytes());
        packet
    }

    #[test]
    fn framing_recovers_split_packet_and_ignores_noise() {
        let packet = packet(&[0x05, 0, 0, 0x05, 0x01, 0, 0, 0, 0, 88]);
        let mut framing = PacketFraming::default();
        assert!(framing.push(&[1, 2, 3, packet[0], packet[1]]).is_empty());
        assert_eq!(framing.push(&packet[2..]), vec![packet]);
    }

    #[test]
    fn rules_parse_tx1_battery_charging_and_input_level() {
        let status = Arc::new(Mutex::new(DjiMicStatus::default()));
        let mut state = vec![0u8; 73];
        state[3] = 0x03;
        state[41] = 0x02;
        state[42] = 0x01;
        state[48] = (3 << 2) | 0x02;
        parse_packet(&packet(&state), &status);
        let mut level = vec![0u8; 10];
        level[3] = 0x05;
        level[3] = 0x05;
        level[4] = 0x01;
        level[9] = 87;
        parse_packet(&packet(&level), &status);
        let value = status.lock().unwrap().clone();
        assert!(value.tx1_connected);
        assert_eq!(value.battery_level, Some(3));
        assert_eq!(value.charging, Some(true));
        assert_eq!(value.input_level, Some(87));
    }

    #[test]
    fn rules_parse_receiver_and_extended_tx1_telemetry() {
        let status = Arc::new(Mutex::new(DjiMicStatus::default()));
        let mut identity = vec![0u8; 108];
        identity[3] = 0x01;
        identity[9..13].copy_from_slice(&[4, 3, 2, 1]);
        identity[13..18].copy_from_slice(b"RX123");
        identity[44] = 6;
        identity[45..51].copy_from_slice(b"DJI RX");
        identity[51] = 0x01;
        identity[52] = 0x01;
        identity[57..61].copy_from_slice(&[8, 7, 6, 5]);
        identity[61..66].copy_from_slice(b"TX123");
        identity[92] = 15;
        identity[93..108].copy_from_slice(b"DJI Mic Mini 2S");
        parse_packet(&packet(&identity), &status);

        let mut state = vec![0u8; 73];
        state[3] = 0x03;
        state[10] = (5 << 5) | 0x10 | 0x04;
        state[11] = (-6i8) as u8;
        state[41] = 0x02;
        state[42] = 0x01;
        state[47] = 0x20 | 0x10 | 0x02;
        state[48] = (6 << 2) | 0x02 | 0x01;
        state[49] = 0x04 | 0x80;
        state[50] = 0x10 | 0x08 | 0x01;
        state[52] = 0x20 | 0x10 | 0x08;
        state[54] = (-3i8) as u8;
        state[55..57].copy_from_slice(&125u16.to_le_bytes());
        state[57..59].copy_from_slice(&84u16.to_le_bytes());
        state[72] = 0x80 | 0x20 | 0x10;
        parse_packet(&packet(&state), &status);

        let value = status.lock().unwrap().clone();
        assert_eq!(value.receiver.firmware_version.as_deref(), Some("01.02.03.04"));
        assert_eq!(value.receiver.device_name.as_deref(), Some("DJI RX"));
        assert_eq!(value.receiver.battery_level, Some(5));
        assert_eq!(value.receiver.gain_control, Some(-6));
        let tx = value.tx1.unwrap();
        assert_eq!(tx.battery_level, Some(6));
        assert_eq!(tx.recording_time_total, Some(12.5));
        assert_eq!(tx.recording_time_remaining, Some(8.4));
        assert_eq!(tx.transmitter_gain, Some(-3));
        assert_eq!(tx.loop_recording, Some(true));
        assert_eq!(value.transmitter.unwrap().noise_cancellation_strong, Some(true));
    }

    #[test]
    fn status_packet_without_tx1_clears_previous_values() {
        let status = Arc::new(Mutex::new(DjiMicStatus {
            receiver_connected: true,
            tx1_connected: true,
            battery_level: Some(1),
            charging: Some(false),
            input_level: Some(20),
            ..DjiMicStatus::default()
        }));
        let mut state = vec![0u8; 41];
        state[3] = 0x03;
        parse_packet(&packet(&state), &status);
        let value = status.lock().unwrap().clone();
        assert!(!value.tx1_connected);
        assert_eq!(value.battery_level, None);
        assert_eq!(value.input_level, None);
    }

    #[test]
    fn monitor_stop_joins_worker_and_releases_interface_promptly() {
        let monitor = DjiMicMonitor::start();
        thread::sleep(Duration::from_millis(100));
        let started = Instant::now();
        monitor.stop();
        assert!(started.elapsed() < Duration::from_secs(1));
        assert!(monitor.worker.lock().unwrap().is_none());
    }

    #[test]
    fn setting_packets_match_protocol_and_reject_wrong_models() {
        let (_, command, value) = writable_setting(
            "tx",
            "lowCut",
            1,
            Some("DJI Mic 2"),
            Some("DJI Mic 2 TX"),
        )
        .unwrap();
        let packet = setting_packet(0x1234, &DjiSettingCommand { address: 0xffff, command, value });
        assert_eq!(&packet[6..8], &0x1234u16.to_le_bytes());
        assert_eq!(&packet[8..20], &[0x40, 0x5b, 0x01, 0x02, 0xff, 0xff, 0, 0, 0x03, 0, 1, 1]);
        assert_eq!(crc8(&packet[..3]), packet[3]);
        assert_eq!(crc16(&packet[..20]), u16::from_le_bytes([packet[20], packet[21]]));
        assert!(writable_setting("tx1", "recording", 1, Some("DJI Mic 2"), Some("DJI Mic 2 TX")).is_err());
        assert!(writable_setting("rx", "gainControl", 5, Some("DJI Mic Mini 2"), None).is_err());
    }

    #[test]
    #[ignore = "requires the DJI Mic WinUSB interface"]
    fn live_receiver_stream_reports_telemetry_and_releases_cleanly() {
        let monitor = DjiMicMonitor::start();
        let deadline = Instant::now() + Duration::from_secs(5);
        let mut value = monitor.status();
        while Instant::now() < deadline {
            value = monitor.status();
            if value.receiver_connected && (!value.tx1_connected || value.input_level.is_some()) {
                break;
            }
            thread::sleep(Duration::from_millis(100));
        }
        println!("DJI live status: {value:?}");
        assert!(
            value.receiver_connected,
            "DJI Mic receiver was not readable on MI_06"
        );
        if value.tx1_connected {
            assert!(value.battery_level.is_some());
            assert!(value.input_level.is_some());
        }
        if let Some(low_cut) = value
            .transmitter
            .as_ref()
            .and_then(|settings| settings.low_cut)
        {
            monitor
                .set_setting("tx", "lowCut", i16::from(low_cut))
                .expect("the current low-cut value should be writable without changing it");
            thread::sleep(Duration::from_millis(350));
            let confirmed = monitor.status();
            assert!(confirmed.receiver_connected);
            assert_eq!(confirmed.error, None);
            assert_eq!(
                confirmed.transmitter.and_then(|settings| settings.low_cut),
                Some(low_cut)
            );
        }
        monitor.stop();
        assert!(monitor.worker.lock().unwrap().is_none());
    }
}
