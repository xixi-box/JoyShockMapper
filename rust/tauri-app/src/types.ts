export type ControllerButton = "ZL" | "ZR" | "L" | "R" | "L3" | "R3" | "MINUS" | "PLUS" | "CAPTURE" | "HOME";

export interface UiSettings {
  language: "zh" | "en";
  minimizeToTray: boolean;
  startWithWindows: boolean;
  flyMouse: {
    enabled: boolean;
    holdButton: ControllerButton;
    sensitivity: number;
    precisionSensitivity: number;
    responseThreshold: number;
    smoothingThreshold: number;
  };
}

export const defaults: UiSettings = {
  language: "zh",
  minimizeToTray: true,
  startWithWindows: false,
  flyMouse: { enabled: true, holdButton: "ZL", sensitivity: 1, precisionSensitivity: 0.7, responseThreshold: 75, smoothingThreshold: 2 }
};

export interface ButtonMapping { button: string; single: string; double: string }
export interface DeviceProfile { id: string; mappings: ButtonMapping[] }
export interface CoreStatus { running: boolean; deviceCount: number; leftConnected: boolean; rightConnected: boolean; activeProfileIds?: string[] }
export interface GyroDebugStatus { inputX: number; inputY: number; outputX: number; outputY: number; active: boolean; sampleCount: number }
export interface ButtonStates { leftButtons: number; rightButtons: number; leftConnected: boolean; rightConnected: boolean; leftTrigger: number; rightTrigger: number }
export interface DjiReceiverState {
  firmwareVersion: string | null; serialNumber: string | null; addressSuffix: string | null; deviceName: string | null;
  batteryLevel: number | null; charging: boolean | null; stereo: boolean | null; quadraphonic: boolean | null;
  safetyTrack: boolean | null; gainControl: number | null; monitoringGain: number | null; clippingControl: boolean | null;
  autoOff: boolean | null; receiverOnOffWithCamera: boolean | null; plugFreeExternalSpeaker: boolean | null;
}
export interface DjiTransmitterState {
  firmwareVersion: string | null; serialNumber: string | null; addressSuffix: string | null; deviceName: string | null;
  batteryLevel: number | null; charging: boolean | null; inputLevel: number | null; recordingTimeTotal: number | null;
  recordingTimeRemaining: number | null; recording: boolean | null; transmitterGain: number | null; voiceToneRich: boolean | null;
  voiceToneBright: boolean | null; fileOptionEditedFile: boolean | null; float32Recording: boolean | null;
  startupAutoRecording: boolean | null; startupAutoRecording2s: boolean | null; autoRecordingWithReceiver: boolean | null;
  lowPowerAutoRecording: boolean | null; loopRecording: boolean | null; recStop: boolean | null; vibration: boolean | null;
}
export interface DjiTransmitterSettings {
  noiseCancellation: boolean | null; noiseCancellationStrong: boolean | null; noiseCancellationViaButton: boolean | null;
  lowCut: boolean | null; clippingControl: boolean | null; loudnessBalance: boolean | null; autoOff: boolean | null; micLedOff: boolean | null;
}
export interface DjiMicStatus {
  receiverConnected: boolean; tx1Connected: boolean; batteryLevel: number | null; charging: boolean | null; inputLevel: number | null;
  receiver: DjiReceiverState; tx1: DjiTransmitterState | null; transmitter: DjiTransmitterSettings | null; error: string | null;
}
