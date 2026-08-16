from pathlib import Path
from playwright.sync_api import sync_playwright


PROFILE_BUTTONS = ["ZL", "L", "MINUS", "L3", "UP", "LEFT", "RIGHT", "DOWN", "CAPTURE", "LSL", "LSR", "ZR", "R", "PLUS", "R3", "N", "W", "E", "S", "HOME", "RSL", "RSR"]


def main():
    Path("out").mkdir(exist_ok=True)
    with sync_playwright() as playwright:
        browser = playwright.chromium.launch(headless=True)
        page = browser.new_page(viewport={"width": 1360, "height": 840})
        page.add_init_script("""
            const callbacks = new Map(); let nextCallback = 1; let gyroSamples = 0;
            window.__TAURI_INTERNALS__ = {
              metadata: { currentWindow: { label: 'main' }, currentWebview: { label: 'main' } },
              transformCallback(callback) { const id = nextCallback++; callbacks.set(id, callback); return id; },
              unregisterCallback(id) { callbacks.delete(id); },
              convertFileSrc(path) { return path; },
              invoke(command) {
                if (command === 'get_settings') return Promise.resolve({ language:'zh', minimizeToTray:true, startWithWindows:true, flyMouse:{ enabled:true, holdButton:'ZL', sensitivity:1, precisionSensitivity:.7, responseThreshold:75, smoothingThreshold:2 } });
                if (command === 'get_core_status') return Promise.resolve({ running:true, deviceCount:2, leftConnected:true, rightConnected:true, activeProfileIds:['device_0123456789abcdef.jsmprofile'] });
                if (command === 'get_gyro_debug_status') return Promise.resolve({ inputX:12.5, inputY:-4.25, outputX:2.1, outputY:-.7, active:true, sampleCount:(gyroSamples += 12) });
                if (command === 'get_dji_mic_status') return Promise.resolve({ receiverConnected:false, tx1Connected:false, batteryLevel:null, charging:null, inputLevel:null, receiver:{}, tx1:null, transmitter:null, error:null });
                if (command === 'list_device_profiles') return Promise.resolve(['device_0123456789abcdef.jsmprofile']);
                if (command === 'load_device_profile') return Promise.resolve({ id:'device_0123456789abcdef.jsmprofile', mappings: window.__PROFILE_BUTTONS__.map(button => ({ button, single:'', double:'' })) });
                return Promise.resolve(null);
              }
            };
        """.replace("window.__PROFILE_BUTTONS__", repr(PROFILE_BUTTONS)))
        errors = []
        page.on("console", lambda message: errors.append(message.text) if message.type == "error" else None)
        page.goto("http://127.0.0.1:1420")
        page.wait_for_load_state("networkidle")
        page.locator(".mappingCallout").first.wait_for()
        assert page.locator(".mappingCallout").count() == len(PROFILE_BUTTONS)
        page.locator(".mappingCallout .actionButton").first.click()
        menu = page.locator(".actionMenu")
        menu.wait_for()
        box = menu.bounding_box()
        assert box and box["x"] >= 0 and box["y"] >= 0 and box["x"] + box["width"] <= 1360 and box["y"] + box["height"] <= 840
        assert "复制输入框全部文字" in menu.inner_text()
        assert "不需要填写路径" in menu.inner_text()
        page.keyboard.press("Escape")
        first_input = page.locator(".mappingCallout input").first
        first_input.click()
        page.keyboard.down("Control")
        page.keyboard.down("Shift")
        page.keyboard.press("a")
        page.keyboard.up("Shift")
        page.keyboard.up("Control")
        page.wait_for_timeout(400)
        assert "LCONTROL + LSHIFT + A" in first_input.input_value()
        page.screenshot(path=str(Path("out/ui-special-actions.png").resolve()), full_page=True)
        page.get_by_role("button", name="陀螺仪").click()
        page.locator(".gyroGrid").wait_for()
        assert page.locator(".gyroGrid label").count() == 5
        assert "12.5 °/s" in page.locator(".gyroMonitor").inner_text()
        page.get_by_role("button", name="开始静止检测").click()
        page.wait_for_timeout(4300)
        assert "偏移明显" in page.locator(".driftReport").inner_text()
        assert "自动校准并复测" in page.locator(".driftReport").inner_text()
        page.screenshot(path=str(Path("out/ui-smoke.png").resolve()), full_page=True)
        assert not errors, errors
        browser.close()


if __name__ == "__main__":
    main()
