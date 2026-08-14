/* ============================================================
   JoyShockMapper GUI — Official Site
   i18n + background FX + scroll animations
   ============================================================ */

const translations = {
  zh: {
    'nav.features': '特性',
    'nav.devices': '设备',
    'nav.arch': '架构',
    'nav.download': '下载',
    'nav.build': '构建',
    'hero.badge': '社区 GUI 版 · 基于 JoyShockMapper',
    'hero.title1': '把手柄的潜力',
    'hero.title2': '全部释放到 PC',
    'hero.sub': '用 DualSense / DualShock 4 / Joy-Con / Pro Controller 玩游戏，支持陀螺仪瞄准、完整按键映射与文本配置工作流 —— 现在带上了现代图形界面。',
    'hero.download': '下载便携版',
    'hero.github': 'View on GitHub',
    'hero.upstream': '上游项目',
    'hero.tag1': 'Gyro On',
    'hero.tag2': 'AutoLoad',
    'stats.devices': '手柄系列',
    'stats.exe': '便携 EXE',
    'stats.lang': '界面语言',
    'stats.install': '无需安装',
    'features.kicker': 'FEATURES',
    'features.title': '核心特性',
    'features.sub': '保留 JoyShockMapper 的映射核心与文本配置格式，并提供现代桌面操作界面。',
    'f1.title': '手柄控制台界面',
    'f1.desc': '借鉴 JoyHarness 的深色手柄控制台：首页直接展示手柄示意图、键位连线与当前映射。',
    'f2.title': '陀螺仪瞄准',
    'f2.desc': '按住启用陀螺仪鼠标移动，灵敏度、加速与平滑范围常用调节直接对应原生参数。',
    'f3.title': '单击 / 双击 / 多键',
    'f3.desc': '单击与双击动作可分别映射，支持单键与多键组合快捷键输出。',
    'f4.title': '每设备自动恢复',
    'f4.desc': '根据控制器身份自动保存并恢复映射，换设备即插即用。',
    'f5.title': '中英文即时切换',
    'f5.desc': '默认简体中文，右上角一键切换 English / 中文，无需重启。',
    'f6.title': '单文件便携 EXE',
    'f6.desc': 'Tauri/Rust 单可执行文件，C++ 核心静态链接进同一进程，无需安装。',
    'f7.title': '兼容文本配置',
    'f7.desc': 'OnStartup.txt / OnReset.txt / GyroConfigs / AutoLoad 原有工作流保持可用。',
    'f8.title': '托盘与开机自启',
    'f8.desc': '最小化到系统托盘，映射与陀螺仪后台继续运行；开机自启无需管理员权限。',
    'devices.kicker': 'DEVICES',
    'devices.title': '支持的设备',
    'devices.sub': '通过 SDL3 原生轮询，覆盖主流当代手柄。',
    'arch.kicker': 'ARCHITECTURE',
    'arch.title': '运行架构',
    'arch.sub': '两种语言、一条进程，成熟核心与现代化界面各司其职。',
    'arch.ui': '界面层',
    'arch.ui.desc': '窗口、设置、托盘、自启与持久化',
    'arch.core': '嵌入式核心',
    'arch.core.desc': '设备轮询 · 实时映射 · 成熟陀螺仪',
    'arch.note': 'C++ 核心编译为静态库，链接进同一进程 —— 不启动额外后端程序。',
    'download.kicker': 'GET IT',
    'download.title': '下载',
    'download.sub': '便携版为单个 EXE，解压即用。WebView2 为 Windows 运行时组件；仅当启用 Xbox / DS4 虚拟手柄输出时才需要安装 ViGEm Bus 驱动。',
    'dl.releases': 'GitHub Releases',
    'dl.releases.desc': '最新便携版与 SHA256 校验和',
    'dl.source': '源码仓库',
    'dl.source.desc': '社区 fork · xixi-box/JoyShockMapper',
    'dl.upstream': '上游 Releases',
    'dl.upstream.desc': '稳定版与完整命令参考',
    'build.kicker': 'BUILD',
    'build.title': '自行构建',
    'build.sub': 'Windows x64 环境需要 Visual Studio 2022 C++、CMake、Ninja、Node.js 与 Rust。',
    'build.copy': '复制',
    'build.note': '构建结果写入仓库根目录 JoyShockMapper.exe，同时生成 out/portable/JoyShockMapper.exe 供 CI 与发行打包。',
    'footer.text': 'MIT License · 上游由 Julian "Jibb" Smart 与 Nicolas Lessard 开发。本站为社区项目，与上游无隶属关系。',
    'footer.upstream': 'Upstream',
    'footer.license': 'License'
  },
  en: {
    'nav.features': 'Features',
    'nav.devices': 'Devices',
    'nav.arch': 'Architecture',
    'nav.download': 'Download',
    'nav.build': 'Build',
    'hero.badge': 'Community GUI · Based on JoyShockMapper',
    'hero.title1': 'Unlock your controller\'s',
    'hero.title2': 'full potential on PC',
    'hero.sub': 'Play PC games with DualSense / DualShock 4 / Joy-Con / Pro Controller — gyro aim, full button mapping and the text-config workflow, now with a modern desktop GUI.',
    'hero.download': 'Download portable build',
    'hero.github': 'View on GitHub',
    'hero.upstream': 'Upstream',
    'hero.tag1': 'Gyro On',
    'hero.tag2': 'AutoLoad',
    'stats.devices': 'Controller lines',
    'stats.exe': 'Portable EXE',
    'stats.lang': 'UI languages',
    'stats.install': 'Zero install',
    'features.kicker': 'FEATURES',
    'features.title': 'Key Features',
    'features.sub': 'Keeps the JoyShockMapper mapping core and text-config format, wrapped in a modern desktop interface.',
    'f1.title': 'Controller dashboard UI',
    'f1.desc': 'JoyHarness-inspired dark console: controller diagram, binding lines and live mapping on the home screen.',
    'f2.title': 'Gyro aiming',
    'f2.desc': 'Hold-to-enable gyro mouse. Sensitivity, acceleration and smoothing map directly to native parameters.',
    'f3.title': 'Single / double / multi-key',
    'f3.desc': 'Separate single-press and double-press bindings; single-key and multi-key shortcuts.',
    'f4.title': 'Per-device auto-restore',
    'f4.desc': 'Profiles saved and restored by controller identity — plug in and go.',
    'f5.title': 'Instant EN / 中文 switching',
    'f5.desc': 'Simplified Chinese by default; switch to English from the top-right without restarting.',
    'f6.title': 'Single-file portable EXE',
    'f6.desc': 'One Tauri/Rust executable with the C++ core statically linked in-process. Nothing to install.',
    'f7.title': 'Text-config workflow kept',
    'f7.desc': 'OnStartup.txt / OnReset.txt / GyroConfigs / AutoLoad still work as before.',
    'f8.title': 'Tray & autostart',
    'f8.desc': 'Minimize to tray while mapping keeps running; autostart needs no admin rights.',
    'devices.kicker': 'DEVICES',
    'devices.title': 'Supported Devices',
    'devices.sub': 'Native SDL3 polling across the current mainstream controller lineup.',
    'arch.kicker': 'ARCHITECTURE',
    'arch.title': 'Runtime Architecture',
    'arch.sub': 'Two languages, one process — a proven core and a modern UI, each doing its job.',
    'arch.ui': 'UI Layer',
    'arch.ui.desc': 'Window, settings, tray, autostart & persistence',
    'arch.core': 'Embedded Core',
    'arch.core.desc': 'Device polling · Live mapping · Mature gyro',
    'arch.note': 'The C++ core compiles to a static library linked into the same process — no extra backend is launched.',
    'download.kicker': 'GET IT',
    'download.title': 'Download',
    'download.sub': 'Portable build is a single EXE, extract and run. WebView2 is a Windows runtime component; ViGEm Bus is only needed for Xbox / DS4 virtual-controller output.',
    'dl.releases': 'GitHub Releases',
    'dl.releases.desc': 'Latest portable build + SHA256 checksum',
    'dl.source': 'Source code',
    'dl.source.desc': 'Community fork · xixi-box/JoyShockMapper',
    'dl.upstream': 'Upstream releases',
    'dl.upstream.desc': 'Stable builds and full command reference',
    'build.kicker': 'BUILD',
    'build.title': 'Build from source',
    'build.sub': 'On Windows x64: Visual Studio 2022 C++, CMake, Ninja, Node.js and Rust.',
    'build.copy': 'Copy',
    'build.note': 'Output is written to JoyShockMapper.exe in the repo root, plus out/portable/JoyShockMapper.exe for CI and release packaging.',
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
