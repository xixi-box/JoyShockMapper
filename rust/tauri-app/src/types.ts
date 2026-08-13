export type ControllerButton = "ZL" | "ZR" | "L" | "R" | "L3" | "R3" | "MINUS" | "PLUS" | "CAPTURE" | "HOME";

export interface UiSettings {
  language: "zh" | "en";
  minimizeToTray: boolean;
  startWithWindows: boolean;
  flyMouse: {
    enabled: boolean;
    holdButton: ControllerButton;
    sensitivity: number;
  };
}

export const defaults: UiSettings = {
  language: "zh",
  minimizeToTray: true,
  startWithWindows: false,
  flyMouse: { enabled: true, holdButton: "ZL", sensitivity: 1 }
};

export interface ButtonMapping { button: string; single: string; double: string }
export interface DeviceProfile { id: string; mappings: ButtonMapping[] }
export interface TriggeredAction { button: string; kind: "single" | "double"; command: string }
export interface CoreStatus { running: boolean; deviceCount: number; leftConnected: boolean; rightConnected: boolean }
