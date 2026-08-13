import { useEffect, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { getCurrentWindow } from "@tauri-apps/api/window";
import { enable, disable } from "@tauri-apps/plugin-autostart";
import { JoyCons } from "./JoyCons";
import { ButtonMapping, ControllerButton, CoreStatus, DeviceProfile, TriggeredAction, UiSettings, defaults } from "./types";

type Page = "home" | "mappings" | "gyro" | "settings";
const holdButtons: ControllerButton[] = ["ZL", "ZR", "L", "R", "L3", "R3", "MINUS", "PLUS", "CAPTURE", "HOME"];

export default function App() {
  const [page, setPage] = useState<Page>("home");
  const [settings, setSettings] = useState<UiSettings>(defaults);
  const [ready, setReady] = useState(false);
  const [core, setCore] = useState<CoreStatus>({ running: false, deviceCount: 0, leftConnected: false, rightConnected: false });

  useEffect(() => {
    invoke<UiSettings>("get_settings").then(setSettings).finally(() => setReady(true));
	const appWindow = getCurrentWindow();
	const unlisten = appWindow.onCloseRequested(async event => {
		event.preventDefault();
		await appWindow.hide();
	});
	const refresh = () => invoke<CoreStatus>("get_core_status").then(setCore).catch(() => undefined);
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
    });
  }

  async function setStartup(value: boolean) {
    value ? await enable() : await disable();
    await update({ ...settings, startWithWindows: value });
  }

  const zh = settings.language !== "en";
  const pageTitle = page === "home" ? (zh ? "控制器总览" : "Controller overview")
    : page === "mappings" ? (zh ? "按键映射" : "Button mappings")
    : page === "gyro" ? (zh ? "体感飞鼠" : "Gyro mouse")
    : (zh ? "应用设置" : "Settings");

  return <main className="appShell">
    <aside>
      <div className="brand"><span className="brandMark">J</span><div><strong>JoyShock</strong><small>CONTROLLER STUDIO</small></div></div>
      <nav>
        <button className={page === "home" ? "active" : ""} onClick={() => setPage("home")}><span>⌂</span> {zh ? "首页" : "Home"}</button>
        <button className={page === "mappings" ? "active" : ""} onClick={() => setPage("mappings")}><span>⌘</span> {zh ? "按键映射" : "Mappings"}</button>
        <button className={page === "gyro" ? "active" : ""} onClick={() => setPage("gyro")}><span>◎</span> {zh ? "陀螺仪" : "Gyro"}</button>
        <button className={page === "settings" ? "active" : ""} onClick={() => setPage("settings")}><span>⚙</span> {zh ? "设置" : "Settings"}</button>
      </nav>
      <div className="engine"><i/>{zh ? "C++ 实时核心" : "C++ real-time core"} {core.running ? (zh ? "运行中" : "running") : (zh ? "未启动" : "stopped")}<small>{zh ? "Rust 界面 · 单进程融合" : "Rust UI · single process"}</small></div>
    </aside>
    <section className="workspace">
      <header><div><p>JOYSHOCKMAPPER</p><h1>{pageTitle}</h1></div><div className="headerActions"><button className="languageButton" onClick={() => update({ ...settings, language: zh ? "en" : "zh" })}>{zh ? "EN" : "中文"}</button><div className="status"><i/>{core.deviceCount > 0 ? (zh ? `已连接 ${core.deviceCount} 个设备` : `${core.deviceCount} device${core.deviceCount === 1 ? "" : "s"} connected`) : (zh ? "等待手柄" : "Waiting for controller")}</div></div></header>
      {!ready ? <div className="loading">{zh ? "正在载入 Rust 配置…" : "Loading Rust settings…"}</div> : page === "home" ? <Home core={core} zh={zh}/> : page === "mappings" ? <Mappings deviceCount={core.deviceCount} zh={zh}/> : page === "gyro" ? <Gyro settings={settings} update={update} zh={zh}/> : <Settings settings={settings} update={update} setStartup={setStartup} zh={zh}/>}
    </section>
  </main>;
}

function Home({ core, zh }: { core: CoreStatus; zh: boolean }) {
  const reconnect = () => invoke("reconnect_controllers");
  return <div className="homeGrid"><article className="controllerStage"><div className="stageTitle"><div><b>{zh ? "左右 Joy-Con" : "Left and right Joy-Con"}</b><span>{zh ? "USB / 蓝牙设备会自动显示" : "USB / Bluetooth devices appear automatically"}</span></div><button onClick={reconnect}>{zh ? "刷新设备" : "Refresh"}</button></div><JoyCons leftConnected={core.leftConnected} rightConnected={core.rightConnected}/></article><article className="sideCard"><label>{zh ? "当前设备" : "CURRENT DEVICE"}</label><h2>{core.deviceCount > 0 ? (zh ? `已连接 ${core.deviceCount} 个` : `${core.deviceCount} connected`) : (zh ? "尚未连接" : "Not connected")}</h2><p>{zh ? "连接后 C++ 核心会自动恢复这套手柄上次保存的按键映射。" : "The C++ core restores the saved mappings for this controller when it connects."}</p><hr/><label>{zh ? "快捷提示" : "QUICK GUIDE"}</label><ul><li>{zh ? "蓝色：左 Joy-Con" : "Blue: left Joy-Con"}</li><li>{zh ? "红色：右 Joy-Con" : "Red: right Joy-Con"}</li><li>{zh ? "灰层：设备未连接" : "Gray overlay: disconnected"}</li></ul></article></div>;
}

function Mappings({ deviceCount, zh }: { deviceCount: number; zh: boolean }) {
  const [profiles, setProfiles] = useState<string[]>([]);
  const [selected, setSelected] = useState("");
  const [profile, setProfile] = useState<DeviceProfile | null>(null);
  const [message, setMessage] = useState("");
  useEffect(() => { void invoke<string[]>("list_device_profiles").then(list => { setProfiles(list); setSelected(current => list.includes(current) ? current : (list[0] ?? "")); }); }, [deviceCount]);
  useEffect(() => { if (selected) void invoke<DeviceProfile>("load_device_profile", { id: selected }).then(setProfile); }, [selected]);
  const edit = (index: number, field: "single" | "double", value: string) => {
    if (!profile) return;
    const mappings = profile.mappings.map((mapping, i) => i === index ? { ...mapping, [field]: value } : mapping);
    setProfile({ ...profile, mappings });
  };
  const save = async () => { if (!profile) return; await invoke("save_and_apply_device_profile", { profile }); setMessage(zh ? "已保存并应用，设备下次连接时也会自动恢复。" : "Saved and applied. It will be restored on the next connection.") };
  const preview = async (button: string, double: boolean) => {
    if (!profile) return;
    await invoke("load_profile_into_input_engine", { profile });
    const start = Math.floor(performance.now());
    const actions: TriggeredAction[] = [];
    actions.push(...await invoke<TriggeredAction[]>("preview_button_event", { button, pressed: true, nowMs: start }));
    await invoke("preview_button_event", { button, pressed: false, nowMs: start + 20 });
    if (double) {
      actions.push(...await invoke<TriggeredAction[]>("preview_button_event", { button, pressed: true, nowMs: start + 100 }));
      await invoke("preview_button_event", { button, pressed: false, nowMs: start + 120 });
    }
    actions.push(...await invoke<TriggeredAction[]>("preview_tick", { nowMs: start + 250 }));
    setMessage(actions.length ? `${zh ? "判定" : "Result"}：${actions.map(action => `${action.kind === "double" ? (zh ? "双击" : "double") : (zh ? "单击" : "single")} → ${action.command}`).join(zh ? "，" : ", ")}` : (zh ? "这个动作尚未设置映射。" : "No mapping is assigned to this action."))
  };
  return <div className="mappingPanel panel">
    <div className="mappingToolbar"><div><h2>{zh ? "设备档案" : "Device profile"}</h2><p>{zh ? "直接编辑与现有 C++ 引擎共用的基础和双击映射。" : "Edit single and double-press mappings used by the C++ engine."}</p></div>{profiles.length > 0 && <select value={selected} onChange={event => setSelected(event.target.value)}>{profiles.map(id => <option key={id}>{id.replace("device_", zh ? "设备 " : "Device ").replace(".jsmprofile", "")}</option>)}</select>}</div>
    {profiles.length === 0 ? <div className="emptyProfile"><b>{zh ? "还没有设备档案" : "No device profile yet"}</b><p>{zh ? "连接一次手柄后，这里会自动出现对应设备。" : "Connect a controller once and its profile will appear here automatically."}</p></div> : <><div className="mappingTable"><div className="mappingHeader"><span>{zh ? "手柄按键" : "Controller"}</span><span>{zh ? "单击映射" : "Single press"}</span><span>{zh ? "双击映射" : "Double press"}</span><span>{zh ? "时序预演" : "Preview"}</span></div>{profile?.mappings.map((mapping, index) => <MappingRow key={mapping.button} mapping={mapping} edit={(field, value) => edit(index, field, value)} preview={preview} zh={zh}/>)}</div><div className="mappingFooter"><span>{message}</span><button onClick={save}>{zh ? "保存到设备档案" : "Save device profile"}</button></div></>}
  </div>;
}

function MappingRow({ mapping, edit, preview, zh }: { mapping: ButtonMapping; edit: (field: "single" | "double", value: string) => void; preview: (button: string, double: boolean) => void; zh: boolean }) {
  return <div className="mappingRow"><strong>{mapping.button}</strong><input value={mapping.single} placeholder={zh ? "例如 SPACE" : "e.g. SPACE"} onChange={event => edit("single", event.target.value.toUpperCase())}/><input value={mapping.double} placeholder={zh ? "未设置" : "Not set"} onChange={event => edit("double", event.target.value.toUpperCase())}/><div className="previewActions"><button onClick={() => preview(mapping.button, false)}>{zh ? "单击" : "Single"}</button><button onClick={() => preview(mapping.button, true)}>{zh ? "双击" : "Double"}</button></div></div>;
}

function Gyro({ settings, update, zh }: { settings: UiSettings; update: (v: UiSettings) => Promise<void>; zh: boolean }) {
  const fly = settings.flyMouse;
  const patch = (next: Partial<UiSettings["flyMouse"]>) => update({ ...settings, flyMouse: { ...fly, ...next } });
  return <div className="panel"><div className="panelHead"><div><span>HOLD TO AIM</span><h2>{zh ? "按住启用，松开停止" : "Hold to enable, release to stop"}</h2><p>{zh ? "默认按住 ZL 才把手柄转动转换为鼠标，避免操作系统桌面时误触。" : "By default, gyro motion becomes mouse movement only while ZL is held."}</p></div><Toggle value={fly.enabled} onChange={value => patch({ enabled: value })} zh={zh}/></div><div className="formGrid"><label>{zh ? "启用键" : "Activation button"}<select value={fly.holdButton} onChange={e => patch({ holdButton: e.target.value as ControllerButton })}>{holdButtons.map(button => <option key={button}>{button}</option>)}</select></label><label>{zh ? "灵敏度" : "Sensitivity"}<strong>{fly.sensitivity.toFixed(1)}</strong><input type="range" min="0.1" max="10" step="0.1" value={fly.sensitivity} onChange={e => patch({ sensitivity: Number(e.target.value) })}/></label></div><div className="logic"><span className="keycap">{fly.holdButton}</span><div className="line"/><span>{zh ? "按住" : "Hold"}</span><div className="line active"/><span className="mouse">↗ {zh ? "鼠标移动" : "Mouse movement"}</span></div></div>;
}

function Settings({ settings, update, setStartup, zh }: { settings: UiSettings; update: (v: UiSettings) => Promise<void>; setStartup: (v: boolean) => Promise<void>; zh: boolean }) {
  return <div className="panel settingsPanel"><Setting title={zh ? "最小化到系统托盘" : "Minimize to system tray"} desc={zh ? "点击最小化按钮时隐藏到托盘；关闭此项则保留在任务栏。" : "Hide in the tray when minimized; disable this to remain on the taskbar."}><Toggle value={settings.minimizeToTray} onChange={value => update({ ...settings, minimizeToTray: value })} zh={zh}/></Setting><Setting title={zh ? "关闭窗口时留在托盘" : "Keep running when closed"} desc={zh ? "点击右上角 × 只隐藏窗口。只有托盘右键菜单中的“退出”才会彻底结束程序。" : "The title-bar close button only hides the window. Use Quit in the tray menu to exit."}><span className="alwaysOn">{zh ? "始终开启" : "Always on"}</span></Setting><Setting title={zh ? "开机自动启动" : "Start with Windows"} desc={zh ? "登录 Windows 后自动运行，不需要管理员权限。" : "Start automatically after Windows sign-in without administrator rights."}><Toggle value={settings.startWithWindows} onChange={setStartup} zh={zh}/></Setting><div className="note">{zh ? "单击托盘图标可恢复主窗口；右键托盘图标可以彻底退出。" : "Left-click the tray icon to restore the window; right-click it to quit."}</div></div>;
}

function Toggle({ value, onChange, zh }: { value: boolean; onChange: (v: boolean) => void; zh: boolean }) { return <button className={`toggle ${value ? "on" : ""}`} onClick={() => onChange(!value)} aria-label={value ? (zh ? "关闭" : "Disable") : (zh ? "开启" : "Enable")}><i/></button>; }
function Setting({ title, desc, children }: React.PropsWithChildren<{ title: string; desc: string }>) { return <div className="setting"><div><h3>{title}</h3><p>{desc}</p></div>{children}</div>; }
