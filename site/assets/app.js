document.addEventListener('DOMContentLoaded', () => {
  const translations = {
    zh: {
      'hero.title': '把手柄的潜力，全部释放到 PC',
      'hero.sub': '用 DualSense / DualShock 4 / Joy-Con / Pro Controller 玩游戏，支持陀螺仪瞄准、完整按键映射与文本配置工作流 —— 现在带上了现代图形界面。',
      'hero.download': '下载便携版',
      'features.title': '核心特性',
      'features.sub': '保留 JoyShockMapper 的映射核心与文本配置格式，并提供现代桌面操作界面。',
      'f1.title': '手柄控制台界面',
      'f1.desc': '借鉴 JoyHarness 的深色手柄控制台：首页直接展示手柄示意图、键位连线与当前映射。',
      'f2.title': '陀螺仪瞄准',
      'f2.desc': '按住启用陀螺仪鼠标移动，灵敏度、加速与平滑范围常用调节直接对应原生参数。',
      'f3.title': '单击 / 双击 / 多键映射',
      'f3.desc': '单击与双击动作可分别映射，支持单键与多键组合快捷键输出。',
      'f4.title': '每设备自动恢复',
      'f4.desc': '根据控制器身份自动保存并恢复映射，换设备即插即用。',
      'f5.title': '中英文即时切换',
      'f5.desc': '默认简体中文，右上角一键切换 English / 中文，无需重启。',
      'f6.title': '单文件便携 EXE',
      'f6.desc': 'Tauri/Rust 单可执行文件，C++ 核心静态链接进同一进程，无需安装。',
      'f7.title': '兼容文本配置工作流',
      'f7.desc': 'OnStartup.txt / OnReset.txt / GyroConfigs / AutoLoad 原有工作流保持可用。',
      'f8.title': '托盘与开机自启',
      'f8.desc': '最小化到系统托盘，映射与陀螺仪后台继续运行；开机自启无需管理员权限。',
      'arch.title': '运行架构',
      'download.title': '下载',
      'download.sub': '便携版为单个 EXE，解压即用。WebView2 为 Windows 运行时组件；仅当启用 Xbox / DS4 虚拟手柄输出时才需要安装 ViGEm Bus 驱动。',
      'dl.releases': 'GitHub Releases',
      'dl.releases.desc': '最新便携版与 SHA256 校验和',
      'dl.source': '源码仓库',
      'dl.source.desc': '社区 fork · xixi-box/JoyShockMapper',
      'dl.upstream': '上游 Releases',
      'dl.upstream.desc': '稳定版与完整命令参考',
      'build.title': '自行构建',
      'build.sub': 'Windows x64 环境需要 Visual Studio 2022 C++、CMake、Ninja、Node.js 与 Rust。',
      'build.note': '构建结果写入仓库根目录 JoyShockMapper.exe，同时生成 out/portable/JoyShockMapper.exe 供 CI 与发行打包。',
      'footer.text': 'MIT License · 上游由 Julian "Jibb" Smart 与 Nicolas Lessard 开发。本站为社区项目，与上游无隶属关系。'
    },
    en: {
      'hero.title': 'Unlock your controller\'s full potential on PC',
      'hero.sub': 'Play PC games with DualSense / DualShock 4 / Joy-Con / Pro Controller — gyro aim, full button mapping and the text-config workflow, now with a modern desktop GUI.',
      'hero.download': 'Download portable build',
      'features.title': 'Key Features',
      'features.sub': 'Keeps the JoyShockMapper mapping core and text-config format, wrapped in a modern desktop interface.',
      'f1.title': 'Controller dashboard UI',
      'f1.desc': 'JoyHarness-inspired dark console: controller diagram, binding lines and live mapping on the home screen.',
      'f2.title': 'Gyro aiming',
      'f2.desc': 'Hold-to-enable gyro mouse. Sensitivity, acceleration and smoothing map directly to native parameters.',
      'f3.title': 'Single / double / multi-key mapping',
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
      'arch.title': 'Runtime Architecture',
      'download.title': 'Download',
      'download.sub': 'Portable build is a single EXE, extract and run. WebView2 is a Windows runtime component; ViGEm Bus is only needed for Xbox / DS4 virtual-controller output.',
      'dl.releases': 'GitHub Releases',
      'dl.releases.desc': 'Latest portable build + SHA256 checksum',
      'dl.source': 'Source code',
      'dl.source.desc': 'Community fork · xixi-box/JoyShockMapper',
      'dl.upstream': 'Upstream releases',
      'dl.upstream.desc': 'Stable builds and full command reference',
      'build.title': 'Build from source',
      'build.sub': 'On Windows x64: Visual Studio 2022 C++, CMake, Ninja, Node.js and Rust.',
      'build.note': 'Output is written to JoyShockMapper.exe in the repo root, plus out/portable/JoyShockMapper.exe for CI and release packaging.',
      'footer.text': 'MIT License · Upstream by Julian "Jibb" Smart and Nicolas Lessard. This is a community site, not affiliated with upstream.'
    }
  };

  const langBtns = document.querySelectorAll('.lang-btn');
  let currentLang = 'zh';

  function applyLang(lang) {
    currentLang = lang;
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
      const original = btn.textContent;
      btn.textContent = '✓';
      setTimeout(() => {
        btn.textContent = original;
      }, 1200);
    });
  });
});
