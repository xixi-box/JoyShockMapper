fn main() {
    #[cfg(windows)]
    link_embedded_core();
    tauri_build::build()
}

#[cfg(windows)]
fn link_embedded_core() {
    let manifest = std::path::PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    let root = manifest.join("../../..").canonicalize().unwrap();
    let build = root.join("out/build/embedded-core-ninja");
    let core = build.join("JoyShockMapper/jsm_embedded_core.lib");
    if !core.exists() {
        panic!(
            "embedded C++ core is missing at {}. Run rust/build-portable.ps1 first",
            core.display()
        );
    }
    println!("cargo:rerun-if-changed={}", root.join("JoyShockMapper/src").display());
    for path in [
        build.join("JoyShockMapper"),
        build.join("_deps/sdl3-build"),
        build.join("_deps/vigemclient-build"),
    ] {
        println!("cargo:rustc-link-search=native={}", path.display());
    }
    for library in [
        "jsm_embedded_core", "SDL3-static", "ViGEmClient", "wsock32", "wininet", "ws2_32",
        "comctl32", "winmm", "imm32", "ole32", "oleaut32", "version", "uuid", "advapi32",
        "setupapi", "shell32", "dinput8",
    ] {
        println!("cargo:rustc-link-lib={library}");
    }
}
