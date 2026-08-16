export function DjiMicVisual({ connected, txConnected }: { connected: boolean; txConnected: boolean }) {
  return <svg className={`djiMicVisual${connected ? " connected" : ""}`} viewBox="0 0 260 150" role="img" aria-label="DJI Mic 接收器与发射器示意图">
    <defs>
      <linearGradient id="djiBody" x1="0" y1="0" x2="1" y2="1"><stop stopColor="#34383d"/><stop offset=".55" stopColor="#16191c"/><stop offset="1" stopColor="#090a0c"/></linearGradient>
      <linearGradient id="djiGlass" x1="0" y1="0" x2="0" y2="1"><stop stopColor="#27313b"/><stop offset="1" stopColor="#07090b"/></linearGradient>
      <filter id="djiShadow" x="-40%" y="-40%" width="180%" height="200%"><feDropShadow dx="0" dy="8" stdDeviation="8" floodOpacity=".25"/></filter>
    </defs>
    <ellipse cx="128" cy="134" rx="104" ry="9" fill="#090b0d" opacity=".12"/>
    <g filter="url(#djiShadow)">
      <g className="djiReceiver">
        <rect x="42" y="31" width="126" height="88" rx="19" fill="url(#djiBody)" stroke="#51565b" strokeWidth="1.2"/>
        <rect x="52" y="42" width="106" height="55" rx="10" fill="url(#djiGlass)" stroke="#0b0d0f"/>
        <text x="64" y="60" fill="#f4f7f8" fontSize="9" fontWeight="700">RX</text>
        <circle cx="65" cy="79" r="4.5" fill={connected ? "#67e8a5" : "#687078"}/>
        <path d="M77 80h22m4 0h13" stroke="#c9d3dc" strokeWidth="3" strokeLinecap="round" opacity=".8"/>
        <path d="M125 75v10m5-14v18m5-10v6m5-13v16" stroke={connected ? "#6ee7b7" : "#69737c"} strokeWidth="2.5" strokeLinecap="round"/>
        <text x="56" y="111" fill="#aeb5bb" fontSize="7" letterSpacing="1.7">DJI MIC</text>
        <circle cx="151" cy="108" r="2.5" fill={connected ? "#67e8a5" : "#555b60"}/>
      </g>
      <g className="djiTransmitter">
        <rect x="169" y="18" width="61" height="94" rx="16" fill="url(#djiBody)" stroke="#555b61" strokeWidth="1.2"/>
        <rect x="181" y="29" width="37" height="7" rx="3.5" fill="#08090a"/>
        <circle cx="199.5" cy="48" r="3" fill={txConnected ? "#67e8a5" : "#646b71"}/>
        <circle cx="199.5" cy="70" r="11" fill="#0c0e10" stroke="#3d4247"/>
        <circle cx="199.5" cy="70" r="4" fill="#20252a"/>
        <text x="183" y="99" fill="#b9c0c6" fontSize="7" letterSpacing="1.25">DJI MIC</text>
        <path d="M188 112v10c0 5 4 9 9 9h5c5 0 9-4 9-9v-10" fill="none" stroke="#181b1e" strokeWidth="5"/>
      </g>
    </g>
  </svg>;
}
