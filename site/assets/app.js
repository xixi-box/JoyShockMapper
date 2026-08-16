/* ============================================================
   JoyShockMapper GUI — Official Site
   i18n + background FX + scroll animations
   ============================================================ */

const translations = {
  zh: {
    'nav.joycon': 'Vibe Coding',
    'nav.djimic': 'DJI Mic',
    'nav.features': '特性',
    'nav.download': '下载',
    'hero.badge': '为 AI 编程 (vibe coding) 打造 · 基于 JoyShockMapper',
    'hero.title1': '让手柄成为你的',
    'hero.title2': 'AI 编程指挥台',
    'hero.sub': '把 Joy-Con 按键映射成 AI 编程的高频操作——保存、运行、聚焦输入框、截图、发送消息。躺着也能写代码，vibe coding 就该这么顺。',
    'hero.download': '下载便携版',
    'hero.github': 'View on GitHub',
    'hero.upstream': '上游项目',
    'hero.gyro': 'Gyro On',
    'hero.map1': '按住 → 陀螺仪瞄准',
    'hero.map2': '单击 → 发送消息',
    'hero.map3': '双击 → Ctrl',
    'hero.screen': '实时映射中',
    'hero.jcon': 'Joy-Con ×2',
    'hero.djimic': 'DJI Mic',
    'focus.joycon.title': 'AI 编程辅助输入',
    'focus.joycon.desc': '把 AI 编程里的高频操作映射到手柄按键：保存、运行、聚焦输入框、截图、粘贴代码，一键触发。',
    'focus.mic.title': 'DJI Mic 实时监视',
    'focus.mic.desc': '原创绘制的接收器/TX 示意图，实时显示电量、充电、输入电平与连接状态，并支持白名单字段回写。',
    'focus.one.title': '一个便携 EXE',
    'focus.one.desc': 'Tauri/Rust 单文件，C++ 核心静态链接进同一进程，免安装即开即用。',
    'joycon.kicker': 'VIBE CODING',
    'joycon.title': '为 vibe coding 打造的手柄映射',
    'joycon.sub': '把 Joy-Con 变成 AI 编程的辅助输入设备：高频快捷键、系统动作，全在手边。',
    'jc1.title': '映射 AI 编程快捷键',
    'jc1.desc': '把保存、运行、聚焦输入框、截图、发送等常用操作映射到手柄按键，双手不离开手柄。',
    'jc2.title': '单击 / 双击 / 多键',
    'jc2.desc': '单击与双击动作分别映射，支持单键与多键组合快捷键输出。',
    'jc3.title': '左右 Joy-Con 独立配置',
    'jc3.desc': '按控制器身份自动保存并恢复映射；有序列号按序列号区分，否则左右侧分别共享。',
    'jc4.title': '虚拟手柄 / 键盘输出',
    'jc4.desc': 'SDL3 原生轮询，经 ViGEm 输出 Xbox/DS4 虚拟手柄，或直接模拟键盘鼠标。',
    'mic.kicker': 'DJI MIC',
    'mic.title': 'DJI Mic 实时状态监视',
    'mic.sub': '零侵入读取接收器状态，不动音频通道，不装额外驱动。',
    'mic.live': '接口已连接 · 状态包读取中',
    'mic1.title': '接收器与 TX 电量',
    'mic1.desc': '实时显示接收器、TX1 电量与充电状态，直观的示意图 + 数字。',
    'mic2.title': '输入电平与连接状态',
    'mic2.desc': '输入电平、TX1 断连/重连、接口被占用自动重试，状态一目了然。',
    'mic3.title': '零侵入设计',
    'mic3.desc': '仅匹配 VID 2CA3 / PID 4011，只读 Interface 6；不动 configuration、不切换 alt setting、不分离驱动，音频通道不受影响。',
    'mic4.title': '白名单字段回写',
    'mic4.desc': '仅对协议有明确命令、取值编码且型号匹配的字段显示控件；界面等待状态包回读确认，不伪造新值。',
    'mic5.title': '完整设备信息',
    'mic5.desc': '固件、设备名称、录制状态、增益、音色、降噪、低切等真实字段，未读到的值绝不凭空补值。',
    'features.kicker': 'FEATURES',
    'features.title': '为 AI 工作流而生',
    'features.sub': '把手柄变成 AI 编程工作流的得力助手：高频操作一键触发，专注写代码。',
    'f1.title': '手柄控制台界面',
    'f1.desc': '借鉴 JoyHarness 的深色手柄控制台：首页直接展示手柄示意图、键位连线与当前映射。',
    'f2.title': '中英文即时切换',
    'f2.desc': '默认简体中文，右上角一键切换 English / 中文，无需重启。',
    'f3.title': '单文件便携 EXE',
    'f3.desc': 'Tauri/Rust 单可执行文件，C++ 核心静态链接进同一进程，无需安装。',
    'f4.title': '兼容文本配置',
    'f4.desc': 'OnStartup.txt / OnReset.txt / GyroConfigs / AutoLoad 原有工作流保持可用。',
    'f5.title': '托盘与开机自启',
    'f5.desc': '最小化到系统托盘，映射与陀螺仪后台继续运行；开机自启无需管理员权限。',
    'f6.title': '菱形快捷键',
    'f6.desc': '复制/剪切/粘贴、系统截图、定位输入控件等特殊功能，普通按键由 C++ 实时执行。',
    'download.kicker': 'GET IT',
    'download.title': '下载',
    'download.sub': '便携版为单个 EXE，解压即用。WebView2 为 Windows 运行时组件；仅当启用 Xbox / DS4 虚拟手柄输出时才需要安装 ViGEm Bus 驱动。',
    'dl.releases': 'GitHub Releases',
    'dl.releases.desc': '最新便携版与 SHA256 校验和',
    'dl.source': '源码仓库',
    'dl.source.desc': '社区 fork · xixi-box/JoyShockMapper',
    'dl.upstream': '上游 Releases',
    'dl.upstream.desc': '稳定版与完整命令参考',
    'footer.text': 'MIT License · 上游由 Julian "Jibb" Smart 与 Nicolas Lessard 开发。本站为社区项目，与上游无隶属关系。',
    'footer.upstream': 'Upstream',
    'footer.license': 'License'
  },
  en: {
    'nav.joycon': 'Vibe Coding',
    'nav.djimic': 'DJI Mic',
    'nav.features': 'Features',
    'nav.download': 'Download',
    'hero.badge': 'Built for vibe coding · Based on JoyShockMapper',
    'hero.title1': 'Turn a gamepad into your',
    'hero.title2': 'AI coding command deck',
    'hero.sub': 'Map Joy-Con buttons to your most-used AI coding actions — save, run, focus the input box, screenshot, send. Keep your hands on the pad while you vibe.',
    'hero.download': 'Download portable build',
    'hero.github': 'View on GitHub',
    'hero.upstream': 'Upstream',
    'hero.gyro': 'Gyro On',
    'hero.map1': 'HOLD → Gyro aim',
    'hero.map2': 'TAP → Send',
    'hero.map3': 'DBL → Ctrl',
    'hero.screen': 'Live mapping',
    'hero.jcon': 'Joy-Con ×2',
    'hero.djimic': 'DJI Mic',
    'focus.joycon.title': 'AI coding input',
    'focus.joycon.desc': 'Bind high-frequency AI coding actions to gamepad buttons: save, run, focus the input box, screenshot, paste code — one tap away.',
    'focus.mic.title': 'DJI Mic live monitor',
    'focus.mic.desc': 'Hand-drawn receiver/TX view showing battery, charging, input level and link status in real time, with whitelisted field write-back.',
    'focus.one.title': 'One portable EXE',
    'focus.one.desc': 'Single Tauri/Rust executable with the C++ core statically linked in-process. Extract and run.',
    'joycon.kicker': 'VIBE CODING',
    'joycon.title': 'Gamepad mapping built for vibe coding',
    'joycon.sub': 'Turn Joy-Con into an auxiliary input device for AI coding: shortcuts and system actions, right under your thumbs.',
    'jc1.title': 'Map AI coding shortcuts',
    'jc1.desc': 'Bind save, run, focus input, screenshot, send and more to gamepad buttons — hands never leave the controller.',
    'jc2.title': 'Single / double / multi-key',
    'jc2.desc': 'Separate single-press and double-press bindings; single-key and multi-key shortcuts.',
    'jc3.title': 'Per-side Joy-Con profiles',
    'jc3.desc': 'Profiles saved and restored by controller identity — serial when available, otherwise per left/right model.',
    'jc4.title': 'Virtual controller / keyboard',
    'jc4.desc': 'Native SDL3 polling; output via ViGEm as Xbox/DS4 virtual controller, or direct keyboard/mouse.',
    'mic.kicker': 'DJI MIC',
    'mic.title': 'DJI Mic live status monitor',
    'mic.sub': 'Zero-intrusion status reading — audio channels untouched, no extra drivers.',
    'mic.live': 'Interface connected · reading status packets',
    'mic1.title': 'Receiver & TX battery',
    'mic1.desc': 'Live receiver, TX1 battery and charging state with an intuitive diagram + numbers.',
    'mic2.title': 'Input level & link status',
    'mic2.desc': 'Input level, TX1 disconnect/reconnect and auto-retry when the interface is busy.',
    'mic3.title': 'Zero-intrusion design',
    'mic3.desc': 'Matches VID 2CA3 / PID 4011 only, reads Interface 6; no configuration changes, no alt-setting switch, no driver detach, audio untouched.',
    'mic4.title': 'Whitelisted field write-back',
    'mic4.desc': 'Controls only shown for fields with explicit commands, encoding and matching model; the UI waits for status read-back confirmation, never fakes values.',
    'mic5.title': 'Full device info',
    'mic5.desc': 'Firmware, device name, recording state, gain, tone, noise cancel, low-cut and more — only real fields, never fabricated.',
    'features.kicker': 'FEATURES',
    'features.title': 'Made for the AI workflow',
    'features.sub': 'Turn a gamepad into a trusted assistant for your AI coding workflow: high-frequency actions one tap away.',
    'f1.title': 'Controller dashboard UI',
    'f1.desc': 'JoyHarness-inspired dark console: controller diagram, binding lines and live mapping on the home screen.',
    'f2.title': 'Instant EN / 中文 switching',
    'f2.desc': 'Simplified Chinese by default; switch to English from the top-right without restarting.',
    'f3.title': 'Single-file portable EXE',
    'f3.desc': 'One Tauri/Rust executable with the C++ core statically linked in-process. Nothing to install.',
    'f4.title': 'Text-config workflow kept',
    'f4.desc': 'OnStartup.txt / OnReset.txt / GyroConfigs / AutoLoad still work as before.',
    'f5.title': 'Tray & autostart',
    'f5.desc': 'Minimize to tray while mapping keeps running; autostart needs no admin rights.',
    'f6.title': 'Diamond shortcut actions',
    'f6.desc': 'Copy/cut/paste, system screenshot, input-focus targeting; normal keys executed by the C++ core in real time.',
    'download.kicker': 'GET IT',
    'download.title': 'Download',
    'download.sub': 'Portable build is a single EXE, extract and run. WebView2 is a Windows runtime component; ViGEm Bus is only needed for Xbox / DS4 virtual-controller output.',
    'dl.releases': 'GitHub Releases',
    'dl.releases.desc': 'Latest portable build + SHA256 checksum',
    'dl.source': 'Source code',
    'dl.source.desc': 'Community fork · xixi-box/JoyShockMapper',
    'dl.upstream': 'Upstream releases',
    'dl.upstream.desc': 'Stable builds and full command reference',
    'footer.text': 'MIT License · Upstream by Julian "Jibb" Smart and Nicolas Lessard. This is a community site, not affiliated with upstream.',
    'footer.upstream': 'Upstream',
    'footer.license': 'License'
  }
};

document.addEventListener('DOMContentLoaded', () => {
  // ---------- i18n ----------
  const langBtns = document.querySelectorAll('.lang-btn');
  let currentLang = localStorage.getItem('jsm-lang') || 'zh';

  function applyLang(lang) {
    currentLang = lang;
    localStorage.setItem('jsm-lang', lang);
    langBtns.forEach((btn) => btn.classList.toggle('active', btn.dataset.lang === lang));
    const dict = translations[lang];
    if (!dict) return;
    document.querySelectorAll('[data-i18n]').forEach((el) => {
      const key = el.dataset.i18n;
      if (dict[key]) el.textContent = dict[key];
    });
    document.documentElement.lang = lang === 'en' ? 'en' : 'zh-CN';
  }

  langBtns.forEach((btn) => btn.addEventListener('click', () => applyLang(btn.dataset.lang)));
  applyLang(currentLang);

  // ---------- Header scroll state ----------
  const header = document.getElementById('site-header');
  const progressBar = document.getElementById('progress-bar');

  function onScroll() {
    const y = window.scrollY;
    header.classList.toggle('scrolled', y > 20);
    const max = document.documentElement.scrollHeight - window.innerHeight;
    const pct = max > 0 ? (y / max) * 100 : 0;
    progressBar.style.width = pct + '%';
  }
  window.addEventListener('scroll', onScroll, { passive: true });
  onScroll();

  // ---------- Mobile menu ----------
  const menuBtn = document.getElementById('menu-btn');
  const navLinks = document.getElementById('nav-links');
  menuBtn.addEventListener('click', () => {
    menuBtn.classList.toggle('open');
    navLinks.classList.toggle('open');
  });
  navLinks.querySelectorAll('a').forEach((a) =>
    a.addEventListener('click', () => {
      menuBtn.classList.remove('open');
      navLinks.classList.remove('open');
    })
  );

  // ---------- Reveal on scroll ----------
  const revealEls = document.querySelectorAll('.reveal');
  const revealObserver = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          entry.target.classList.add('visible');
          revealObserver.unobserve(entry.target);
        }
      });
    },
    { threshold: 0.12 }
  );
  revealEls.forEach((el) => revealObserver.observe(el));

  // ---------- Animated counters ----------
  const counterEls = document.querySelectorAll('[data-count]');
  const counterObserver = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (!entry.isIntersecting) return;
        const el = entry.target;
        const target = Number(el.dataset.count);
        const duration = 1200;
        const start = performance.now();
        function tick(now) {
          const p = Math.min((now - start) / duration, 1);
          const eased = 1 - Math.pow(1 - p, 3);
          el.textContent = Math.round(target * eased);
          if (p < 1) requestAnimationFrame(tick);
        }
        requestAnimationFrame(tick);
        counterObserver.unobserve(el);
      });
    },
    { threshold: 0.6 }
  );
  counterEls.forEach((el) => counterObserver.observe(el));

  // ---------- Copy button ----------
  document.querySelectorAll('.copy-btn').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const text = btn.dataset.copy;
      try {
        await navigator.clipboard.writeText(text);
      } catch {
        const ta = document.createElement('textarea');
        ta.value = text;
        document.body.appendChild(ta);
        ta.select();
        document.execCommand('copy');
        ta.remove();
      }
      const textEl = btn.querySelector('.copy-text');
      const original = textEl.textContent;
      textEl.textContent = '✓';
      textEl.classList.add('done');
      setTimeout(() => {
        textEl.textContent = original;
        textEl.classList.remove('done');
      }, 1200);
    });
  });

  // ---------- Background particle canvas ----------
  const canvas = document.getElementById('fx-canvas');
  const ctx = canvas.getContext('2d');
  let particles = [];
  let rafId;

  function resizeCanvas() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
  }

  function initParticles() {
    const count = Math.min(70, Math.floor(window.innerWidth / 22));
    particles = Array.from({ length: count }, () => ({
      x: Math.random() * canvas.width,
      y: Math.random() * canvas.height,
      vx: (Math.random() - 0.5) * 0.35,
      vy: (Math.random() - 0.5) * 0.35,
      r: Math.random() * 1.8 + 0.6,
      a: Math.random() * 0.35 + 0.1
    }));
  }

  function drawParticles() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    const linkDist = 130;

    particles.forEach((p, i) => {
      p.x += p.vx;
      p.y += p.vy;
      if (p.x < 0 || p.x > canvas.width) p.vx *= -1;
      if (p.y < 0 || p.y > canvas.height) p.vy *= -1;

      ctx.beginPath();
      ctx.arc(p.x, p.y, p.r, 0, Math.PI * 2);
      ctx.fillStyle = `rgba(217, 119, 87, ${p.a})`;
      ctx.fill();

      for (let j = i + 1; j < particles.length; j++) {
        const q = particles[j];
        const dx = p.x - q.x;
        const dy = p.y - q.y;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist < linkDist) {
          ctx.beginPath();
          ctx.moveTo(p.x, p.y);
          ctx.lineTo(q.x, q.y);
          ctx.strokeStyle = `rgba(217, 119, 87, ${0.08 * (1 - dist / linkDist)})`;
          ctx.lineWidth = 0.6;
          ctx.stroke();
        }
      }
    });

    rafId = requestAnimationFrame(drawParticles);
  }

  resizeCanvas();
  initParticles();
  drawParticles();

  window.addEventListener('resize', () => {
    resizeCanvas();
    initParticles();
  });

  // Respect reduced motion for the canvas
  if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
    cancelAnimationFrame(rafId);
  }
});
