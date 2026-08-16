import { useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { getCurrentWindow } from "@tauri-apps/api/window";
import { JoyCons } from "./JoyCons";
import { ControllerButton, CoreStatus, DeviceProfile, DjiMicStatus, GyroDebugStatus, UiSettings, defaults } from "./types";
import { DjiMicVisual } from "./DjiMicVisual";

type Page = "home" | "gyro" | "settings";
type MappingMode = "single" | "double";
type VisualButton = { button: string; label: string; side: "left" | "right"; row: number; anchor: [number, number] };
const holdButtons: ControllerButton[] = ["ZL", "ZR", "L", "R", "L3", "R3", "MINUS", "PLUS", "CAPTURE", "HOME"];
const visualButtons: VisualButton[] = [
  { button: "ZL", label: "ZL", side: "left", row: 0, anchor: [365, 58] },
  { button: "L", label: "L", side: "left", row: 1, anchor: [370, 78] },
  { button: "MINUS", label: "−", side: "left", row: 2, anchor: [418, 105] },
  { button: "L3", label: "L3", side: "left", row: 3, anchor: [373, 185] },
  { button: "UP", label: "↑", side: "left", row: 4, anchor: [373, 285] },
  { button: "LEFT", label: "←", side: "left", row: 5, anchor: [346, 325] },
  { button: "RIGHT", label: "→", side: "left", row: 6, anchor: [400, 325] },
  { button: "DOWN", label: "↓", side: "left", row: 7, anchor: [373, 365] },
  { button: "CAPTURE", label: "截图", side: "left", row: 8, anchor: [373, 455] },
  { button: "LSL", label: "SL", side: "left", row: 9, anchor: [446, 430] },
  { button: "LSR", label: "SR", side: "left", row: 10, anchor: [446, 480] },
  { button: "ZR", label: "ZR", side: "right", row: 0, anchor: [635, 58] },
  { button: "R", label: "R", side: "right", row: 1, anchor: [630, 78] },
  { button: "PLUS", label: "+", side: "right", row: 2, anchor: [582, 105] },
  { button: "R3", label: "R3", side: "right", row: 3, anchor: [627, 355] },
  { button: "N", label: "X", side: "right", row: 4, anchor: [627, 170] },
  { button: "W", label: "Y", side: "right", row: 5, anchor: [600, 210] },
  { button: "E", label: "A", side: "right", row: 6, anchor: [654, 210] },
  { button: "S", label: "B", side: "right", row: 7, anchor: [627, 250] },
  { button: "HOME", label: "主页", side: "right", row: 8, anchor: [627, 455] },
  { button: "RSL", label: "SL", side: "right", row: 9, anchor: [554, 430] },
  { button: "RSR", label: "SR", side: "right", row: 10, anchor: [554, 480] },
];
const djiMicNotice = `dji-mic-mo — Copyright (c) 2026 usokawa_.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.`;

// Keyboard capture is handled by the Rust low-level hook (key_capture.rs),
// which emits JoyShockMapper key names directly for any combination,
// including system-reserved shortcuts such as Win+Shift+S.
const mouseButtonNames = ["LMOUSE", "MMOUSE", "RMOUSE", "BMOUSE", "FMOUSE"];
const chordCommand = (keys: string[]) => keys.length < 2 ? (keys[0] ?? "") : keys.map(key => `${key}\\`).join(" ");
const decodeHexUtf8 = (hex: string) => {
  try { return new TextDecoder().decode(Uint8Array.from(hex.match(/../g) ?? [], byte => Number.parseInt(byte, 16))); } catch { return ""; }
};
const displayCommand = (command: string, zh = true) => {
  if (command === '"UI_ACTION COPY_ALL"') return zh ? "复制输入框全部文字" : "Copy all input text";
  if (command === '"UI_ACTION CUT_ALL"') return zh ? "剪切输入框全部文字" : "Cut all input text";
  if (command === '"UI_ACTION SCREENSHOT"') return zh ? "系统截图" : "Screenshot";
  if (command === '"UI_ACTION FOCUS_INPUT LCONTROL+L"') return zh ? "聚焦当前软件的地址栏 / 搜索框" : "Focus current app address/search box";
  if (command === '"UI_ACTION FOCUS_INPUT LCONTROL+F"') return zh ? "聚焦当前软件的查找框" : "Focus current app find box";
  if (command === '"UI_ACTION FOCUS_INPUT TAB"') return zh ? "聚焦当前软件的下一个输入控件" : "Focus next control in current app";
  const launch = command.match(/^"UI_ACTION LAUNCH_FOCUS ([0-9a-f]+) ([0-9a-f]+)"$/i);
  if (launch) return `${zh ? "打开并定位输入框" : "Launch & focus input"} · ${decodeHexUtf8(launch[1]).split(/[\\/]/).pop()}`;
  const parts = command.trim().split(/\s+/);
  return parts.length > 1 && parts.every(part => part.endsWith("\\"))
    ? parts.map(part => part.slice(0, -1)).join(" + ")
    : command;
};

export default function App() {
  const [page, setPage] = useState<Page>("home");
  const [settings, setSettings] = useState<UiSettings>(defaults);
  const [ready, setReady] = useState(false);
  const [core, setCore] = useState<CoreStatus>({ running: false, deviceCount: 0, leftConnected: false, rightConnected: false });
  const [djiMic, setDjiMic] = useState<DjiMicStatus>({ receiverConnected: false, tx1Connected: false, batteryLevel: null, charging: null, inputLevel: null, receiver: {}, tx1: null, transmitter: null, error: null } as DjiMicStatus);

  useEffect(() => {
    invoke<UiSettings>("get_settings").then(setSettings).finally(() => setReady(true));
	const appWindow = getCurrentWindow();
	const unlisten = appWindow.onCloseRequested(async event => {
		event.preventDefault();
		await appWindow.hide();
	});
	const refresh = () => {
		void invoke<CoreStatus>("get_core_status").then(setCore).catch(() => undefined);
		void invoke<DjiMicStatus>("get_dji_mic_status").then(setDjiMic).catch(() => undefined);
	};
	void refresh();
	const timer = globalThis.setInterval(refresh, 750);
	return () => { globalThis.clearInterval(timer); void unlisten.then(dispose => dispose()); };
  }, []);

  async function update(next: UiSettings) {
    setSettings(next);
    await invoke("save_settings", { settings: next });
    await invoke("configure_fly_mouse", {
      enabled: next.flyMouse.enabled,
      holdButton: next.flyMouse.holdButton,
      sensitivity: next.flyMouse.sensitivity,
      precisionSensitivity: next.flyMouse.precisionSensitivity,
      responseThreshold: next.flyMouse.responseThreshold,
      smoothingThreshold: next.flyMouse.smoothingThreshold,
    });
  }

  async function setStartup(value: boolean) {
    await invoke("set_autostart", { enabled: value });
    await update({ ...settings, startWithWindows: value });
  }

  const zh = settings.language !== "en";
  const pageTitle = page === "home" ? (zh ? "控制器总览" : "Controller overview")
    : page === "gyro" ? (zh ? "体感飞鼠" : "Gyro mouse")
    : (zh ? "应用设置" : "Settings");

  return <main className="appShell">
    <aside>
      <div className="brand"><span className="brandMark">J</span><div><strong>JoyShock</strong><small>CONTROLLER STUDIO</small></div></div>
      <nav>
        <button className={page === "home" ? "active" : ""} onClick={() => setPage("home")}><span>⌂</span> {zh ? "首页" : "Home"}</button>
        <button className={page === "gyro" ? "active" : ""} onClick={() => setPage("gyro")}><span>◎</span> {zh ? "陀螺仪" : "Gyro"}</button>
        <button className={page === "settings" ? "active" : ""} onClick={() => setPage("settings")}><span>⚙</span> {zh ? "设置" : "Settings"}</button>
      </nav>
      <div className="engine"><i/>{zh ? "C++ 实时核心" : "C++ real-time core"} {core.running ? (zh ? "运行中" : "running") : (zh ? "未启动" : "stopped")}<small>{zh ? "Rust 界面 · 单进程融合" : "Rust UI · single process"}</small></div>
    </aside>
    <section className="workspace">
      <header><div><p>JOYSHOCKMAPPER</p><h1>{pageTitle}</h1></div><div className="headerActions"><button className="languageButton" onClick={() => update({ ...settings, language: zh ? "en" : "zh" })}>{zh ? "EN" : "中文"}</button><div className="status"><i/>{core.deviceCount > 0 ? (zh ? `已连接 ${core.deviceCount} 个设备` : `${core.deviceCount} device${core.deviceCount === 1 ? "" : "s"} connected`) : (zh ? "等待手柄" : "Waiting for controller")}</div></div></header>
      {!ready ? <div className="loading">{zh ? "正在载入 Rust 配置…" : "Loading Rust settings…"}</div> : page === "home" ? <Home core={core} djiMic={djiMic} zh={zh}/> : page === "gyro" ? <Gyro core={core} settings={settings} update={update} zh={zh}/> : <Settings settings={settings} update={update} setStartup={setStartup} zh={zh}/>}
    </section>
  </main>;
}

function Home({ core, djiMic, zh }: { core: CoreStatus; djiMic: DjiMicStatus; zh: boolean }) {
  const reconnect = () => invoke("reconnect_controllers");
  return <div className="homeDashboard"><Mappings core={core} reconnect={reconnect} zh={zh}/><DjiMicCard status={djiMic} zh={zh}/></div>;
}

function Mappings({ core, reconnect, zh }: { core: CoreStatus; reconnect: () => void; zh: boolean }) {
  const [profiles, setProfiles] = useState<string[]>([]);
  const [selected, setSelected] = useState("");
  const [profile, setProfile] = useState<DeviceProfile | null>(null);
  const [mode, setMode] = useState<MappingMode>("single");
  const [message, setMessage] = useState("");
  const saveTimer = useRef<number | null>(null);
  const saveVersion = useRef(0);
  const saveQueue = useRef<Promise<void>>(Promise.resolve());
  useEffect(() => { void invoke<string[]>("list_device_profiles").then(list => {
    const connectedProfile = core.activeProfileIds?.find(id => list.includes(id));
    setProfiles(list);
    setSelected(current => connectedProfile ?? (list.includes(current) ? current : (list[0] ?? "")));
  }); }, [core.deviceCount, core.activeProfileIds?.join("|")]);
  useEffect(() => { if (selected) void invoke<DeviceProfile>("load_device_profile", { id: selected }).then(setProfile); }, [selected]);
  useEffect(() => () => { if (saveTimer.current != null) globalThis.clearTimeout(saveTimer.current); }, []);
  const persist = (candidate: DeviceProfile, version: number, manual = false) => {
    saveQueue.current = saveQueue.current.catch(() => undefined).then(() => invoke("save_and_apply_device_profile", { profile: candidate }));
    void saveQueue.current.then(() => {
      if (version === saveVersion.current) setMessage(manual ? (zh ? "已保存并应用。" : "Saved and applied.") : (zh ? "已自动保存并应用" : "Automatically saved and applied"));
    }).catch(error => {
      if (version === saveVersion.current) setMessage(`${zh ? "保存失败，请点击右侧重试" : "Save failed; click retry"} · ${String(error)}`);
    });
  };
  const edit = (button: string, field: MappingMode, value: string) => {
    if (!profile) return;
    const mappings = profile.mappings.map(mapping => mapping.button === button ? { ...mapping, [field]: value } : mapping);
    const next = { ...profile, mappings };
    setProfile(next);
    setMessage(zh ? "正在保存并应用…" : "Saving and applying…");
    const version = ++saveVersion.current;
    if (saveTimer.current != null) globalThis.clearTimeout(saveTimer.current);
    saveTimer.current = globalThis.setTimeout(() => persist(next, version), 300);
  };
  const save = () => { if (!profile) return; if (saveTimer.current != null) globalThis.clearTimeout(saveTimer.current); persist(profile, ++saveVersion.current, true); };
  const mappingFor = (button: string) => profile?.mappings.find(mapping => mapping.button === button);
  const connectedButtons = visualButtons.filter(point => point.side === "left" ? core.leftConnected : core.rightConnected);
  const bothConnected = core.leftConnected && core.rightConnected;
  const calloutLayout = (point: VisualButton, index: number) => bothConnected
    ? { side: point.side, y: 24 + point.row * 54, anchor: point.anchor }
    : { side: index % 2 === 0 ? "left" as const : "right" as const, y: 55 + Math.floor(index / 2) * 92, anchor: [point.anchor[0] + (point.side === "left" ? 80 : -80), point.anchor[1]] as [number, number] };
  return <article className="controllerStage visualMapper">
    <div className="stageTitle visualToolbar">
      <div><b>{zh ? "Joy-Con 按键映射" : "Joy-Con button mapping"}</b><span>{zh ? "点击箭头外侧的映射框，然后直接按下单键或组合键" : "Select a callout, then press a key or chord"}</span></div>
      <div className="mapperActions">
        {connectedButtons.length > 0 && profiles.length > 0 && <select value={selected} onChange={event => setSelected(event.target.value)}>{profiles.map(id => <option key={id}>{id.replace("device_", zh ? "设备 " : "Device ").replace(".jsmprofile", "")}</option>)}</select>}
        {connectedButtons.length > 0 && <div className="modeSwitch" aria-label={zh ? "触发方式" : "Trigger mode"}><button className={mode === "single" ? "active" : ""} onClick={() => setMode("single")}>{zh ? "单击" : "Single"}</button><button className={mode === "double" ? "active" : ""} onClick={() => setMode("double")}>{zh ? "双击" : "Double"}</button></div>}
        <button className="refreshButton" onClick={reconnect}>{zh ? "刷新设备" : "Refresh"}</button>
      </div>
    </div>
    <div className="mappingCanvas">
      {(core.leftConnected || core.rightConnected) && <JoyCons leftConnected={core.leftConnected} rightConnected={core.rightConnected}/>}
      {profile && connectedButtons.length > 0 && <>
        <svg className="mappingConnectors" viewBox="0 0 1000 600" preserveAspectRatio="none" aria-hidden="true"><defs><marker id="mappingArrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0 0 8 4 0 8Z"/></marker></defs>{connectedButtons.map((point, index) => { const layout = calloutLayout(point, index); const startX = layout.side === "left" ? 232 : 768; const controlX = layout.side === "left" ? 300 : 700; return <path key={point.button} d={`M${startX} ${layout.y} Q${controlX} ${layout.y} ${layout.anchor[0]} ${layout.anchor[1]}`} markerEnd="url(#mappingArrow)"/>; })}</svg>
        {connectedButtons.map((point, index) => { const mapping = mappingFor(point.button); const layout = calloutLayout(point, index); return <div key={point.button} className={`mappingCallout ${layout.side}`} data-button={point.button} style={{ top: `${layout.y / 6}%` }}><strong>{point.label}</strong><MappingInput value={mapping?.[mode] ?? ""} onCommit={value => edit(point.button, mode, value)} placeholder={zh ? "点击后按键" : "Press a key"} recordingHint={zh ? "请按键或再次点击" : "Press keys or click again"} clearTitle={zh ? "清除" : "Clear"} zh={zh}/></div>; })}
      </>}
      {connectedButtons.length === 0 && <div className="waitingForController"><i/><b>{zh ? "等待连接" : "Waiting for controller"}</b><span>{zh ? "连接左侧或右侧 Joy-Con 后，这里会显示对应手柄与按键映射。" : "Connect either Joy-Con to see its controls and mappings."}</span></div>}
      {connectedButtons.length > 0 && !profile && <div className="emptyProfile visualEmpty"><b>{zh ? "正在建立设备档案" : "Creating device profile"}</b><p>{zh ? "档案建立后即可设置按键映射。" : "Mappings will be available when the profile is ready."}</p></div>}
    </div>
    <div className="mappingFooter visualFooter"><span>{message || (zh ? `当前编辑：${mode === "single" ? "单击" : "双击"}映射` : `Editing ${mode} mappings`)}</span><button disabled={!profile || connectedButtons.length === 0} onClick={save}>{zh ? "保存并应用" : "Save & apply"}</button></div>
  </article>;
}

function DjiMicCard({ status, zh }: { status: DjiMicStatus; zh: boolean }) {
  const [settingMessage, setSettingMessage] = useState("");
  const level = status.inputLevel ?? 0;
  const battery = status.batteryLevel == null ? "—" : `${status.batteryLevel} / 7`;
  const yesNo = (value: boolean | null | undefined) => value == null ? "—" : value ? (zh ? "开启" : "On") : (zh ? "关闭" : "Off");
  const rx = status.receiver ?? {};
  const tx = status.tx1;
  const common = status.transmitter;
  const write = async (node: string, field: string, value: number) => {
    setSettingMessage(zh ? "正在发送，等待设备回报…" : "Sending; waiting for device report…");
    try {
      await invoke("set_dji_mic_setting", { node, field, value });
      setSettingMessage(zh ? "已发送；显示值将在设备确认后更新" : "Sent; the value updates after device confirmation");
    } catch (error) {
      setSettingMessage(String(error));
    }
  };
  const canWrite = (node: string, field: string) => {
    if (!status.receiverConnected || !rx.deviceName) return false;
    if (node === "rx") {
      if (["stereo", "safetyTrack", "clippingControl", "plugFreeExternalSpeaker"].includes(field)) return true;
      if (["gainControl", "monitoringGain"].includes(field)) return rx.deviceName === "DJI Mic Mini 2";
      return rx.deviceName !== "DJI Mic Mini 2" && ["autoOff", "receiverOnOffWithCamera"].includes(field);
    }
    if (node === "tx") return !["noiseCancellation", "noiseCancellationStrong", "noiseCancellationViaButton"].includes(field) || rx.deviceName !== "DJI Mic Mini 2";
    if (node !== "tx1" || !tx?.deviceName) return false;
    if (["voiceToneRich", "voiceToneBright"].includes(field)) return tx.deviceName !== "DJI Mic Mini";
    return tx.deviceName === "DJI Mic Mini 2S" && ["recording", "transmitterGain", "fileOptionEditedFile", "float32Recording", "startupAutoRecording", "startupAutoRecording2s", "autoRecordingWithReceiver", "lowPowerAutoRecording", "loopRecording", "recStop", "vibration"].includes(field);
  };
  const detail = (label: string, value: unknown, node?: string, field?: string) => value == null || value === "" ? null : <div className="djiDetail" key={`${node}-${field}-${label}`}><small>{label}</small><strong>{typeof value === "boolean" ? yesNo(value) : String(value)}</strong>{node && field && typeof value === "boolean" && canWrite(node, field) && <button className={`djiSettingSwitch ${value ? "on" : ""}`} onClick={() => void write(node, field, value ? 0 : 1)} title={zh ? "修改后等待设备回报确认" : "Wait for device confirmation after changing"}><i/></button>}{node && field && typeof value === "number" && canWrite(node, field) && <select className="djiGainSelect" value={value} onChange={event => void write(node, field, Number(event.target.value))}>{(node === "rx" && field === "gainControl" ? [-12,-6,0,6,12] : Array.from({length:25},(_,index)=>index-12)).map(option => <option key={option} value={option}>{option}</option>)}</select>}</div>;
  return <aside className={`djiMicCard ${status.receiverConnected ? "connected" : ""} ${status.error ? "hasError" : ""}`}>
    <header className="djiPanelHeader"><div><span>DJI MIC</span><b>{zh ? "无线麦克风" : "Wireless microphone"}</b></div><em>{zh ? "USB 只读监测 / 已验证项可调" : "USB telemetry / verified controls"}</em></header>
    <DjiMicVisual connected={status.receiverConnected} txConnected={status.tx1Connected}/>
    <div className="djiOverview">
      <div className="djiIdentity"><span>{rx.deviceName ?? "RECEIVER"}</span><b>{status.error ? (zh ? "状态接口暂不可用 · 自动重试" : "Status interface unavailable · retrying") : status.receiverConnected ? (zh ? "接收器已连接" : "Receiver connected") : (zh ? "等待接收器" : "Waiting for receiver")}</b><small>{status.receiverConnected ? (zh ? "以下数据均来自设备实时状态包" : "All values below come from device packets") : (zh ? "尚无设备数据" : "No device data")}</small></div>
      <div className="djiSummary">
        <div className="djiMetric"><small>TX1</small><strong className={status.tx1Connected ? "online" : ""}>{status.tx1Connected ? (zh ? "已连接" : "Online") : (zh ? "未连接" : "Offline")}</strong></div>
        <div className="djiMetric"><small>{zh ? "电量" : "Battery"}</small><strong>{battery}{status.charging === true ? ` · ${zh ? "充电中" : "Charging"}` : ""}</strong></div>
        <div className="djiMetric inputMetric"><small>{zh ? "输入电平" : "Input level"}</small><div><i style={{ width: `${Math.min(100, level / 2.55)}%` }}/></div><strong>{status.inputLevel ?? "—"}</strong></div>
      </div>
      {status.error && <div className="djiDiagnostic"><b>{zh ? "Windows 尚未允许读取 MI_06" : "Windows has not made MI_06 readable"}</b><span>{zh ? "若刚安装 WinUSB，请重启 Windows；程序会继续自动重试。" : "Restart Windows after installing WinUSB; the app will keep retrying."}</span><code title={status.error}>{status.error}</code></div>}
    </div>
    {settingMessage && <div className="djiSettingMessage">{settingMessage}</div>}
    {(status.receiverConnected || status.tx1Connected) && <div className="djiTelemetryGroups">
        <section><h4>{zh ? "接收器实报" : "Receiver report"}</h4><div className="djiDetails">
          {detail(zh ? "设备名称" : "Device", rx.deviceName)}{detail(zh ? "固件版本" : "Firmware", rx.firmwareVersion)}{detail(zh ? "序列号" : "Serial", rx.serialNumber)}{detail(zh ? "地址尾码" : "Address suffix", rx.addressSuffix)}
          {detail(zh ? "接收器电量" : "RX battery", rx.batteryLevel == null ? null : `${rx.batteryLevel} / 7`)}{detail(zh ? "接收器充电" : "RX charging", rx.charging)}{detail(zh ? "立体声" : "Stereo", rx.stereo, "rx", "stereo")}{detail(zh ? "四声道" : "Quadraphonic", rx.quadraphonic)}
          {detail(zh ? "安全音轨" : "Safety track", rx.safetyTrack, "rx", "safetyTrack")}{detail(zh ? "接收增益" : "Gain", rx.gainControl, "rx", "gainControl")}{detail(zh ? "监听增益" : "Monitor gain", rx.monitoringGain, "rx", "monitoringGain")}{detail(zh ? "削波控制" : "Clipping control", rx.clippingControl, "rx", "clippingControl")}
          {detail(zh ? "自动关机" : "Auto off", rx.autoOff, "rx", "autoOff")}{detail(zh ? "随相机开关" : "Power with camera", rx.receiverOnOffWithCamera, "rx", "receiverOnOffWithCamera")}{detail(zh ? "免插外放" : "Plug-free speaker", rx.plugFreeExternalSpeaker, "rx", "plugFreeExternalSpeaker")}
        </div></section>
        <section><h4>{zh ? "TX1 实报" : "TX1 report"}</h4><div className="djiDetails">
          {detail(zh ? "设备名称" : "Device", tx?.deviceName)}{detail(zh ? "固件版本" : "Firmware", tx?.firmwareVersion)}{detail(zh ? "序列号" : "Serial", tx?.serialNumber)}{detail(zh ? "地址尾码" : "Address suffix", tx?.addressSuffix)}
          {detail(zh ? "电量" : "Battery", tx?.batteryLevel == null ? null : `${tx.batteryLevel} / 7`)}{detail(zh ? "充电" : "Charging", tx?.charging)}{detail(zh ? "输入电平" : "Input level", tx?.inputLevel)}{detail(zh ? "正在录制" : "Recording", tx?.recording, "tx1", "recording")}
          {detail(zh ? "总录制时长" : "Total record time", tx?.recordingTimeTotal)}{detail(zh ? "剩余录制时长" : "Remaining record time", tx?.recordingTimeRemaining)}{detail(zh ? "发射器增益" : "TX gain", tx?.transmitterGain, "tx1", "transmitterGain")}{detail(zh ? "浑厚音色" : "Rich tone", tx?.voiceToneRich, "tx1", "voiceToneRich")}
          {detail(zh ? "明亮音色" : "Bright tone", tx?.voiceToneBright, "tx1", "voiceToneBright")}{detail(zh ? "32 位浮点录音" : "32-bit float", tx?.float32Recording, "tx1", "float32Recording")}{detail(zh ? "循环录音" : "Loop recording", tx?.loopRecording, "tx1", "loopRecording")}{detail(zh ? "低电量自动录音" : "Low-power auto recording", tx?.lowPowerAutoRecording, "tx1", "lowPowerAutoRecording")}
          {detail(zh ? "开机自动录音" : "Startup auto recording", tx?.startupAutoRecording, "tx1", "startupAutoRecording")}{detail(zh ? "2 秒后自动录音" : "2s startup recording", tx?.startupAutoRecording2s, "tx1", "startupAutoRecording2s")}{detail(zh ? "随接收器自动录音" : "Record with receiver", tx?.autoRecordingWithReceiver, "tx1", "autoRecordingWithReceiver")}{detail(zh ? "停止录音控制" : "Record stop", tx?.recStop, "tx1", "recStop")}
          {detail(zh ? "振动" : "Vibration", tx?.vibration, "tx1", "vibration")}{detail(zh ? "生成编辑文件" : "Edited file option", tx?.fileOptionEditedFile, "tx1", "fileOptionEditedFile")}{detail(zh ? "降噪" : "Noise cancellation", common?.noiseCancellation, "tx", "noiseCancellation")}{detail(zh ? "强降噪" : "Strong cancellation", common?.noiseCancellationStrong, "tx", "noiseCancellationStrong")}
          {detail(zh ? "按键降噪" : "Button cancellation", common?.noiseCancellationViaButton, "tx", "noiseCancellationViaButton")}{detail(zh ? "低切" : "Low cut", common?.lowCut, "tx", "lowCut")}{detail(zh ? "削波控制" : "Clipping control", common?.clippingControl, "tx", "clippingControl")}{detail(zh ? "响度平衡" : "Loudness balance", common?.loudnessBalance, "tx", "loudnessBalance")}
          {detail(zh ? "自动关机" : "Auto off", common?.autoOff, "tx", "autoOff")}{detail(zh ? "关闭麦克风灯" : "Mic LED off", common?.micLedOff, "tx", "micLedOff")}
        </div></section>
      </div>}
    <details className="djiLicense"><summary>{zh ? "协议来源与许可" : "Protocol source & license"}</summary><pre>{djiMicNotice}</pre></details>
  </aside>;
}

function MappingInput({ value, onCommit, placeholder, recordingHint, clearTitle, zh }: { value: string; onCommit: (value: string) => void; placeholder: string; recordingHint: string; clearTitle: string; zh: boolean }) {
  const input = useRef<HTMLInputElement>(null);
  const held = useRef(new Set<string>());
  const captured = useRef<string[]>([]);
  const recordingRef = useRef(false);
  const hookActiveRef = useRef(false);
  const activationMouse = useRef<number | null>(null);
  const finishTimer = useRef<number | null>(null);
  const [recording, setRecording] = useState(false);
  const [draft, setDraft] = useState("");
  const [actionsOpen, setActionsOpen] = useState(false);
  const [editing, setEditing] = useState(false);
  const [editText, setEditText] = useState("");
  const editingRef = useRef(false);
  const cancelTimer = () => { if (finishTimer.current != null) { globalThis.clearTimeout(finishTimer.current); finishTimer.current = null; } };
  const startHook = () => { if (!hookActiveRef.current) { hookActiveRef.current = true; void invoke("start_key_capture"); } };
  const stopHook = () => { if (hookActiveRef.current) { hookActiveRef.current = false; void invoke("stop_key_capture"); } };
  const begin = () => { cancelTimer(); held.current.clear(); captured.current = []; setDraft(""); recordingRef.current = true; setRecording(true); startHook(); };
  const beginEdit = () => { setEditing(true); setEditText(value); editingRef.current = true; setActionsOpen(false); input.current?.focus(); };
  const capture = (name: string | null) => {
    if (!name || captured.current.includes(name)) return;
    captured.current.push(name);
    setDraft(captured.current.join("+"));
  };
  const finish = () => {
    cancelTimer();
    stopHook();
    if (captured.current.length) onCommit(chordCommand(captured.current));
    recordingRef.current = false;
    setRecording(false);
    input.current?.blur();
  };
  const finishAfterPause = () => {
    cancelTimer();
    if (!captured.current.length) return;
    finishTimer.current = globalThis.setTimeout(finish, 280);
  };
  useEffect(() => () => stopHook(), []);
  useEffect(() => {
    if (!recording) return;
    let unlisten: (() => void) | undefined;
    void listen<{ kind: string; key: string }>("key-captured", event => {
      if (!recordingRef.current) return;
      cancelTimer();
      if (event.payload.kind === "down") {
        held.current.add(`Key:${event.payload.key}`);
        capture(event.payload.key);
      } else {
        held.current.delete(`Key:${event.payload.key}`);
        if (held.current.size === 0) finishAfterPause();
      }
    }).then(dispose => { unlisten = dispose; });
    const mouseDown = (event: MouseEvent) => {
      if (!recordingRef.current) return;
      if (activationMouse.current === event.button) return;
      event.preventDefault(); event.stopPropagation(); cancelTimer();
      held.current.add(`Mouse:${event.button}`); capture(mouseButtonNames[event.button] ?? null);
    };
    const mouseUp = (event: MouseEvent) => {
      if (!recordingRef.current) return;
      if (activationMouse.current === event.button) { activationMouse.current = null; return; }
      event.preventDefault(); event.stopPropagation(); held.current.delete(`Mouse:${event.button}`);
      if (held.current.size === 0) finishAfterPause();
    };
    const wheel = (event: WheelEvent) => {
      if (!recordingRef.current) return;
      event.preventDefault(); event.stopPropagation(); cancelTimer();
      capture(event.deltaY < 0 ? "SCROLLUP" : "SCROLLDOWN"); finishAfterPause();
    };
    window.addEventListener("mousedown", mouseDown, true); window.addEventListener("mouseup", mouseUp, true);
    window.addEventListener("wheel", wheel, { capture: true, passive: false });
    return () => {
      unlisten?.();
      window.removeEventListener("mousedown", mouseDown, true); window.removeEventListener("mouseup", mouseUp, true);
      window.removeEventListener("wheel", wheel, true); cancelTimer();
    };
  }, [recording]);
  return <div className={`captureField${recording ? " recording" : ""}`}>
    <input ref={input}
      readOnly={!editing}
      value={editing ? editText : (recording ? draft : displayCommand(value, zh))}
      placeholder={recording ? recordingHint : placeholder}
      onMouseDown={event => { if (!recordingRef.current) activationMouse.current = event.button; }}
      onDoubleClick={() => { if (!recordingRef.current && !editing) beginEdit(); }}
      onFocus={() => { setActionsOpen(false); if (editingRef.current) { editingRef.current = false; return; } begin(); }}
      onChange={event => { if (editing) setEditText(event.target.value); }}
      onKeyDown={event => {
        if (!editing) return;
        if (event.key === "Enter") { onCommit(editText.trim()); setEditing(false); input.current?.blur(); }
        if (event.key === "Escape") { setEditing(false); input.current?.blur(); }
      }}
      onBlur={() => {
        if (editing) {
          const text = editText.trim();
          if (text && text !== value) onCommit(text);
          setEditing(false);
        }
      }}/>
    {!recording && !editing && <button type="button" className="actionButton" onMouseDown={event => event.preventDefault()} onClick={() => setActionsOpen(open => !open)} title={zh ? "特殊功能" : "Special actions"}>◇</button>}
    {value && !recording && <button type="button" className="clearButton" onMouseDown={event => event.preventDefault()} onClick={() => onCommit("")} title={clearTitle}>×</button>}
    {actionsOpen && <div className="actionMenu" onMouseDown={event => event.stopPropagation()}>
      <b>{zh ? "特殊功能" : "Special actions"}</b>
      <button onClick={() => { beginEdit(); }}>{zh ? "手动输入命令…" : "Type command…"}</button>
      <hr/>
      <button onClick={() => { onCommit('"UI_ACTION COPY_ALL"'); setActionsOpen(false); }}>{zh ? "复制输入框全部文字" : "Copy all input text"}</button>
      <button onClick={() => { onCommit('"UI_ACTION CUT_ALL"'); setActionsOpen(false); }}>{zh ? "剪切输入框全部文字" : "Cut all input text"}</button>
      <button onClick={() => { onCommit("LCONTROL\\ V\\"); setActionsOpen(false); }}>{zh ? "粘贴" : "Paste"}</button>
      <button onClick={() => { onCommit('"UI_ACTION SCREENSHOT"'); setActionsOpen(false); }}>{zh ? "系统截图" : "Screenshot"}</button>
      <hr/>
      <b>{zh ? "聚焦当前软件的输入位置" : "Focus input in the current app"}</b>
      <small className="actionHint">{zh ? "触发时对正在使用的软件发送快捷键，不需要填写路径。" : "Sends a shortcut to the app you are using; no path required."}</small>
      <button onClick={() => { onCommit('"UI_ACTION FOCUS_INPUT LCONTROL+L"'); setActionsOpen(false); }}>{zh ? "地址栏 / 搜索框（Ctrl+L）" : "Address/search box (Ctrl+L)"}</button>
      <button onClick={() => { onCommit('"UI_ACTION FOCUS_INPUT LCONTROL+F"'); setActionsOpen(false); }}>{zh ? "页面查找框（Ctrl+F）" : "Find box (Ctrl+F)"}</button>
      <button onClick={() => { onCommit('"UI_ACTION FOCUS_INPUT TAB"'); setActionsOpen(false); }}>{zh ? "下一个输入控件（Tab）" : "Next input control (Tab)"}</button>
    </div>}
  </div>;
}

function Gyro({ core, settings, update, zh }: { core: CoreStatus; settings: UiSettings; update: (v: UiSettings) => Promise<void>; zh: boolean }) {
  const fly = settings.flyMouse;
  const [telemetry, setTelemetry] = useState<GyroDebugStatus>({ inputX: 0, inputY: 0, outputX: 0, outputY: 0, active: false, sampleCount: 0 });
  const [sampleRate, setSampleRate] = useState(0);
  const [driftProgress, setDriftProgress] = useState(0);
  const [driftResult, setDriftResult] = useState<{ x: number; y: number; noise: number } | null>(null);
  const [calibrating, setCalibrating] = useState(false);
  const driftTest = useRef<{ started: number; lastCount: number; samples: Array<[number, number]> } | null>(null);
  const previousSample = useRef({ count: 0, time: performance.now() });
  useEffect(() => {
    const refresh = () => void invoke<GyroDebugStatus>("get_gyro_debug_status").then(next => {
      const now = performance.now();
      const elapsed = now - previousSample.current.time;
      if (elapsed > 0) setSampleRate(Math.max(0, Math.round((next.sampleCount - previousSample.current.count) * 1000 / elapsed)));
      previousSample.current = { count: next.sampleCount, time: now };
      setTelemetry(next);
      const test = driftTest.current;
      if (test && next.sampleCount !== test.lastCount) {
        test.lastCount = next.sampleCount;
        test.samples.push([next.inputX, next.inputY]);
        const progress = Math.min(1, (now - test.started) / 4000);
        setDriftProgress(progress);
        if (progress >= 1 && test.samples.length) {
          const x = test.samples.reduce((sum, sample) => sum + sample[0], 0) / test.samples.length;
          const y = test.samples.reduce((sum, sample) => sum + sample[1], 0) / test.samples.length;
          const noise = Math.sqrt(test.samples.reduce((sum, sample) => sum + (sample[0] - x) ** 2 + (sample[1] - y) ** 2, 0) / test.samples.length);
          setDriftResult({ x, y, noise }); driftTest.current = null;
        }
      }
    }).catch(() => undefined);
    refresh(); const timer = globalThis.setInterval(refresh, 100);
    return () => globalThis.clearInterval(timer);
  }, []);
  const patch = (next: Partial<UiSettings["flyMouse"]>) => update({ ...settings, flyMouse: { ...fly, ...next } });
  const startDriftTest = () => {
    if (!core.deviceCount) return;
    setDriftResult(null); setDriftProgress(0);
    driftTest.current = { started: performance.now(), lastCount: telemetry.sampleCount, samples: [] };
  };
  const calibrate = async () => {
    if (!core.deviceCount || calibrating) return;
    driftTest.current = null; setDriftResult(null); setDriftProgress(0); setCalibrating(true);
    try { await invoke("calibrate_gyro"); startDriftTest(); } finally { setCalibrating(false); }
  };
  const magnitude = Math.hypot(telemetry.inputX, telemetry.inputY);
  // Recent input history for the scope trail (kept as a ring buffer of x/y).
  const trail = useRef<Array<[number, number]>>([]);
  trail.current.push([telemetry.inputX, telemetry.inputY]);
  if (trail.current.length > 40) trail.current.splice(0, trail.current.length - 40);
  // Sensitivity response curve: input speed -> output sensitivity multiplier.
  // Below responseThreshold the precision sensitivity applies; above it the
  // fast sensitivity applies; the blend is a smooth transition.
  const responseThreshold = fly.responseThreshold;
  const precisionSensitivity = fly.precisionSensitivity;
  const fastSensitivity = fly.sensitivity;
  const curvePoints = (() => {
    const points: string[] = [];
    const maxSpeed = Math.max(responseThreshold * 3, 150);
    for (let speed = 0; speed <= maxSpeed; speed += maxSpeed / 60) {
      const blend = speed <= responseThreshold ? 0 : Math.min(1, (speed - responseThreshold) / Math.max(responseThreshold, 1));
      const smooth = 1 - Math.pow(1 - blend, 2);
      const sens = precisionSensitivity + (fastSensitivity - precisionSensitivity) * smooth;
      points.push(`${(speed / maxSpeed * 100).toFixed(1)},${(sens / Math.max(fastSensitivity, precisionSensitivity, 0.1) * 100).toFixed(1)}`);
    }
    return points.join(" ");
  })();
  const curveMaxSens = Math.max(fastSensitivity, precisionSensitivity, 0.1);
  const curveMaxSpeed = Math.max(responseThreshold * 3, 150);
  const workingBlend = magnitude <= responseThreshold ? 0 : Math.min(1, (magnitude - responseThreshold) / Math.max(responseThreshold, 1));
  const workingSens = precisionSensitivity + (fastSensitivity - precisionSensitivity) * (1 - Math.pow(1 - workingBlend, 2));
  const workingX = (Math.min(magnitude, curveMaxSpeed) / curveMaxSpeed * 100).toFixed(1);
  const workingY = (workingSens / curveMaxSens * 100).toFixed(1);
  const diagnostic = core.deviceCount === 0 ? (zh ? "等待手柄连接" : "Waiting for controller")
    : sampleRate === 0 ? (zh ? "未收到陀螺仪数据" : "No gyro samples")
    : !fly.enabled ? (zh ? "飞鼠已关闭" : "Fly mouse is disabled")
    : !telemetry.active ? (zh ? `数据正常，请按住 ${fly.holdButton}` : `Data ready; hold ${fly.holdButton}`)
    : magnitude < 1 ? (zh ? "已启用 · 手柄接近静止" : "Active · controller is nearly still")
    : (zh ? "已启用 · 正在输出鼠标移动" : "Active · producing mouse movement");
  const dotX = 50 + Math.max(-42, Math.min(42, telemetry.inputX / 4));
  const dotY = 50 + Math.max(-42, Math.min(42, telemetry.inputY / 4));
  const driftMagnitude = driftResult ? Math.hypot(driftResult.x, driftResult.y) : 0;
  const driftAngle = driftResult ? Math.atan2(driftResult.y, driftResult.x) * 180 / Math.PI : 0;
  const driftMarkerX = 50 + Math.max(-38, Math.min(38, (driftResult?.x ?? 0) * 12));
  const driftMarkerY = 50 + Math.max(-38, Math.min(38, (driftResult?.y ?? 0) * 12));
  const driftGrade = driftMagnitude < .5 ? (zh ? "优秀" : "Excellent") : driftMagnitude < 1.5 ? (zh ? "轻微偏移" : "Slight drift") : (zh ? "偏移明显" : "High drift");
  const driftAdvice = !driftResult ? (zh ? "把手柄平放在桌面上，完全不要碰它，然后开始检测。" : "Place the controller flat and do not touch it, then begin the test.")
    : driftMagnitude >= 1.5 ? (zh ? "这是静态零点偏移，不要靠降低灵敏度掩盖；请执行“自动校准并复测”。" : "This is zero-point drift. Recalibrate instead of hiding it with lower sensitivity.")
    : driftResult.noise >= 1.5 ? (zh ? "中心偏移不大，但抖动较多。先确认桌面稳定，再把“低速平滑范围”提高 0.5～1。" : "Bias is low but noise is high. Stabilize the controller, then raise low-speed smoothing by 0.5–1.")
    : (zh ? "静态表现正常。若实际瞄准太快或太慢，只调整精细移动灵敏度即可。" : "Static behavior is healthy. Adjust precision sensitivity only if aiming feels too fast or slow.");
  return <div className="panel gyroPanel">
    <div className="panelHead"><div><span>HOLD TO AIM</span><h2>{zh ? "按住启用，松开停止" : "Hold to enable, release to stop"}</h2><p>{zh ? "这里显示 C++ 核心的真实陀螺仪输入和鼠标输出，参数直接写入 JoyShockMapper 原生引擎。" : "Live values come from the C++ core; controls tune the native JoyShockMapper engine."}</p></div><Toggle value={fly.enabled} onChange={value => patch({ enabled: value })} zh={zh}/></div>
    <section className={`gyroMonitor ${telemetry.active ? "active" : ""}`}><div className="gyroScope"><svg className="scopeTrail" viewBox="0 0 100 100" preserveAspectRatio="none"><polyline points={trail.current.map(([x, y], index) => `${50 + Math.max(-45, Math.min(45, x / 4))},${50 + Math.max(-45, Math.min(45, y / 4))}`).join(" ")} /></svg><i style={{ left: `${dotX}%`, top: `${dotY}%` }}/><span>+</span></div><div className="gyroReadout"><b>{diagnostic}</b><div><span>{zh ? "水平输入" : "Horizontal input"}<strong>{telemetry.inputX.toFixed(1)} °/s</strong></span><span>{zh ? "垂直输入" : "Vertical input"}<strong>{telemetry.inputY.toFixed(1)} °/s</strong></span><span>{zh ? "鼠标输出 X" : "Mouse output X"}<strong>{telemetry.outputX.toFixed(2)}</strong></span><span>{zh ? "鼠标输出 Y" : "Mouse output Y"}<strong>{telemetry.outputY.toFixed(2)}</strong></span></div><small>{zh ? `数据更新率：${sampleRate} 次/秒 · ${telemetry.active ? "启用键已按住" : "启用键未按住"}` : `Sample rate: ${sampleRate}/s · ${telemetry.active ? "activation held" : "activation not held"}`}</small></div></section>
    <section className="sensitivityCurve"><div className="curveHead"><span>{zh ? "灵敏度响应曲线" : "Sensitivity response curve"}</span><small>{zh ? `横轴：输入角速度 (°/s) · 纵轴：输出灵敏度` : `X: input speed (°/s) · Y: output sensitivity`}</small></div><div className="curvePlot"><svg viewBox="0 0 100 100" preserveAspectRatio="none"><polyline className="curveLine" points={curvePoints}/><circle className="curveWorking" cx={workingX} cy={workingY} r={3}/></svg><span className="curvePivot" style={{ left: `${(responseThreshold / curveMaxSpeed * 100).toFixed(1)}%` }}/></div><div className="curveLegend"><span>{zh ? `精细 ${precisionSensitivity.toFixed(1)}` : `Precision ${precisionSensitivity.toFixed(1)}`}</span><span>{zh ? `阈值 ${responseThreshold}°/s` : `Threshold ${responseThreshold}°/s`}</span><span>{zh ? `快速 ${fastSensitivity.toFixed(1)}` : `Fast ${fastSensitivity.toFixed(1)}`}</span><span>{zh ? `当前 ${workingSens.toFixed(1)}` : `Now ${workingSens.toFixed(1)}`}</span></div></section>
    <section className="driftLab"><div className="driftTarget"><span className="north">{zh ? "上漂" : "UP"}</span><span className="south">{zh ? "下漂" : "DOWN"}</span><span className="west">{zh ? "左漂" : "LEFT"}</span><span className="east">{zh ? "右漂" : "RIGHT"}</span><div className="driftRings"/><i className="driftVector" style={{ width: `${Math.min(62, driftMagnitude * 25)}px`, transform: `rotate(${driftAngle}deg)` }}/><b style={{ left: `${driftMarkerX}%`, top: `${driftMarkerY}%` }}/></div><div className="driftReport"><small>{zh ? "4 秒静止偏移体检" : "4-second stationary drift check"}</small><h3>{driftResult ? `${driftGrade} · ${driftMagnitude.toFixed(2)} °/s` : driftTest.current ? (zh ? `检测中 ${Math.round(driftProgress * 100)}%` : `Testing ${Math.round(driftProgress * 100)}%`) : (zh ? "看看手柄会往哪边自己漂" : "See where the controller drifts")}</h3><div className="driftProgress"><i style={{ width: `${driftProgress * 100}%` }}/></div>{driftResult && <p>{zh ? `水平 ${driftResult.x > 0 ? "向右" : "向左"} ${Math.abs(driftResult.x).toFixed(2)}，垂直 ${driftResult.y > 0 ? "向下" : "向上"} ${Math.abs(driftResult.y).toFixed(2)}，抖动 ${driftResult.noise.toFixed(2)} °/s。` : `Horizontal ${driftResult.x.toFixed(2)}, vertical ${driftResult.y.toFixed(2)}, noise ${driftResult.noise.toFixed(2)} °/s.`}</p>}<strong>{driftAdvice}</strong><div className="driftActions"><button disabled={!core.deviceCount || !!driftTest.current || calibrating} onClick={startDriftTest}>{zh ? "开始静止检测" : "Start stationary test"}</button><button className="calibrateButton" disabled={!core.deviceCount || calibrating} onClick={() => void calibrate()}>{calibrating ? (zh ? "校准中，请保持不动…" : "Calibrating; keep still…") : (zh ? "自动校准并复测" : "Calibrate and retest")}</button></div></div></section>
    <div className="gyroHelp"><b>{zh ? "快速调试方法" : "Quick tuning"}</b><span>{zh ? "① 平放手柄，输入应接近 0　② 按住启用键，缓慢转动看光点与输出　③ 先调精细灵敏度，再调快速灵敏度；感觉拖尾就降低平滑。" : "① Hold still: input should approach 0. ② Hold activation and rotate slowly. ③ Tune precision first, then fast sensitivity; reduce smoothing if movement trails."}</span><button onClick={() => patch({ precisionSensitivity: 1.5, sensitivity: 4, responseThreshold: 80, smoothingThreshold: 2 })}>{zh ? "恢复易调试的推荐值" : "Restore tuning defaults"}</button></div>
    <div className="formGrid gyroGrid"><label>{zh ? "启用键" : "Activation button"}<select value={fly.holdButton} onChange={e => patch({ holdButton: e.target.value as ControllerButton })}>{holdButtons.map(button => <option key={button}>{button}</option>)}</select><small>{zh ? "只有按住这个键时才移动鼠标" : "Mouse moves only while this button is held"}</small></label><Range label={zh ? "精细移动灵敏度" : "Precision sensitivity"} value={fly.precisionSensitivity} min={0.1} max={10} step={0.1} onChange={value => patch({ precisionSensitivity: value })} hint={zh ? "慢速转动时使用" : "Used for slow motion"}/><Range label={zh ? "快速移动灵敏度" : "Fast sensitivity"} value={fly.sensitivity} min={0.1} max={10} step={0.1} onChange={value => patch({ sensitivity: value })} hint={zh ? "快速转动时逐渐过渡到此速度" : "Reached during fast motion"}/><Range label={zh ? "加速生效速度" : "Response threshold"} value={fly.responseThreshold} min={10} max={250} step={5} onChange={value => patch({ responseThreshold: value })} hint={zh ? "数值越小，越早进入快速灵敏度" : "Lower values reach fast sensitivity sooner"}/><Range label={zh ? "低速平滑范围" : "Low-speed smoothing"} value={fly.smoothingThreshold} min={0} max={15} step={0.5} onChange={value => patch({ smoothingThreshold: value })} hint={zh ? "0 为关闭；过高会感觉迟滞" : "0 disables it; high values add latency"}/></div>
  </div>;
}

function Range({ label, value, min, max, step, onChange, hint }: { label: string; value: number; min: number; max: number; step: number; onChange: (value: number) => void; hint: string }) {
  return <label>{label}<strong>{value.toFixed(step < 1 ? 1 : 0)}</strong><input type="range" min={min} max={max} step={step} value={value} onChange={event => onChange(Number(event.target.value))}/><small>{hint}</small></label>;
}

function Settings({ settings, update, setStartup, zh }: { settings: UiSettings; update: (v: UiSettings) => Promise<void>; setStartup: (v: boolean) => Promise<void>; zh: boolean }) {
  return <div className="panel settingsPanel"><Setting title={zh ? "最小化到系统托盘" : "Minimize to system tray"} desc={zh ? "点击最小化按钮时隐藏到托盘；关闭此项则保留在任务栏。" : "Hide in the tray when minimized; disable this to remain on the taskbar."}><Toggle value={settings.minimizeToTray} onChange={value => update({ ...settings, minimizeToTray: value })} zh={zh}/></Setting><Setting title={zh ? "关闭窗口时留在托盘" : "Keep running when closed"} desc={zh ? "点击右上角 × 只隐藏窗口。只有托盘右键菜单中的“退出”才会彻底结束程序。" : "The title-bar close button only hides the window. Use Quit in the tray menu to exit."}><span className="alwaysOn">{zh ? "始终开启" : "Always on"}</span></Setting><Setting title={zh ? "开机自动启动" : "Start with Windows"} desc={zh ? "登录 Windows 后自动运行，不需要管理员权限。" : "Start automatically after Windows sign-in without administrator rights."}><Toggle value={settings.startWithWindows} onChange={setStartup} zh={zh}/></Setting><div className="note">{zh ? "单击托盘图标可恢复主窗口；右键托盘图标可以彻底退出。" : "Left-click the tray icon to restore the window; right-click it to quit."}</div></div>;
}

function Toggle({ value, onChange, zh }: { value: boolean; onChange: (v: boolean) => void; zh: boolean }) { return <button className={`toggle ${value ? "on" : ""}`} onClick={() => onChange(!value)} aria-label={value ? (zh ? "关闭" : "Disable") : (zh ? "开启" : "Enable")}><i/></button>; }
function Setting({ title, desc, children }: React.PropsWithChildren<{ title: string; desc: string }>) { return <div className="setting"><div><h3>{title}</h3><p>{desc}</p></div>{children}</div>; }
