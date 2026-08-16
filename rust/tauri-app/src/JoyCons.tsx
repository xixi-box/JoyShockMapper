type Props = { leftConnected?: boolean; rightConnected?: boolean };

function Stick({ x, y }: { x: number; y: number }) {
  return <g transform={`translate(${x} ${y})`}>
    <circle r="34" fill="#090e12" opacity=".5" transform="translate(2 4)"/>
    <circle r="32" fill="#171d21" stroke="#465158" strokeWidth="2"/>
    <circle r="23" fill="#30383e" stroke="#0a0d10" strokeWidth="4"/>
    <circle r="17" fill="#1b2125" stroke="#505a60" strokeWidth="1.5"/>
    <path d="M-12-5c8-5 16-5 24 0M-12 5c8 5 16 5 24 0" fill="none" stroke="#313a3f" strokeWidth="2" opacity=".8"/>
  </g>;
}

function RoundButton({ x, y, text }: { x: number; y: number; text: string }) {
  return <g transform={`translate(${x} ${y})`}><circle cy="2" r="12" fill="#070b0e" opacity=".5"/><circle r="11" fill="#171d21" stroke="#68747a" strokeWidth="1"/><text y="4" textAnchor="middle" className="buttonText">{text}</text></g>;
}

export function JoyCons({ leftConnected = false, rightConnected = false }: Props) {
  const leftX = rightConnected ? 102 : 185;
  const rightX = leftConnected ? 268 : 185;
  return <svg className="joycons" viewBox="0 0 520 520" role="img" aria-label="左右 Joy-Con">
    <defs>
      <linearGradient id="leftShell" x1="0" y1="0" x2="1" y2="1"><stop stopColor="#3ddcff"/><stop offset=".48" stopColor="#19bfe5"/><stop offset="1" stopColor="#0788aa"/></linearGradient>
      <linearGradient id="rightShell" x1="0" y1="0" x2="1" y2="1"><stop stopColor="#ff625d"/><stop offset=".5" stopColor="#ef3d47"/><stop offset="1" stopColor="#ba1730"/></linearGradient>
      <linearGradient id="rail" x1="0" x2="1"><stop stopColor="#070b0d"/><stop offset=".5" stopColor="#303a40"/><stop offset="1" stopColor="#080c0e"/></linearGradient>
      <filter id="joyShadow"><feDropShadow dx="0" dy="20" stdDeviation="14" floodColor="#000" floodOpacity=".55"/></filter>
      <filter id="softGlow"><feGaussianBlur stdDeviation="18"/></filter>
    </defs>
    <ellipse cx="260" cy="468" rx="152" ry="24" fill="#22c8ec" opacity=".12" filter="url(#softGlow)"/>

    {leftConnected && <g className="leftJoyCon" transform={`translate(${leftX} 28)`} filter="url(#joyShadow)">
      <path d="M84 0h66v452H77C31 452 0 416 0 369V85C0 38 36 0 84 0Z" fill="url(#leftShell)" stroke="#72e8ff" strokeOpacity=".4" strokeWidth="2"/>
      <path d="M82 0h68v20H82C45 20 22 49 22 86v278c0 38 25 68 61 68h67v20H77C31 452 0 416 0 369V85C0 38 36 0 82 0Z" fill="#fff" opacity=".07"/>
      <rect x="137" y="13" width="13" height="426" fill="url(#rail)"/><path d="M136 31v387" stroke="#7d898f" strokeWidth="1"/><circle cx="143" cy="45" r="2.5" fill="#929ca1"/><circle cx="143" cy="406" r="2.5" fill="#929ca1"/>
      <path d="M19 23C30 8 48 0 69 0h51l-4 20H63c-18 0-31 8-39 20Z" fill="#10161a" stroke="#5d686e"/><text x="64" y="16" className="shoulderText">L</text>
      <path d="M18 12C29 2 43-4 60-5h47l-7 17H54c-14 0-24 6-32 15Z" fill="#080c0f" stroke="#4b565c"/><text x="57" y="8" className="shoulderText">ZL</text>
      <rect x="91" y="47" width="31" height="8" rx="4" fill="#151b1f"/>
      <Stick x={70} y={128}/>
      <RoundButton x={70} y={230} text="▲"/><RoundButton x={103} y={263} text="▶"/><RoundButton x={70} y={296} text="▼"/><RoundButton x={37} y={263} text="◀"/>
      <g transform="translate(70 374)"><rect x="-13" y="-13" width="26" height="26" rx="3" fill="#12181c" stroke="#5e6970"/><circle r="6" fill="#4b565c"/></g>
      <g fill="#11171b">{[0,1,2,3].map(i=><rect key={i} x="108" y={356+i*17} width="13" height="5" rx="2.5"/>)}</g>
    </g>}

    {rightConnected && <g className="rightJoyCon" transform={`translate(${rightX} 28)`} filter="url(#joyShadow)">
      <path d="M0 0h66c48 0 84 38 84 85v284c0 47-31 83-77 83H0Z" fill="url(#rightShell)" stroke="#ff918c" strokeOpacity=".4" strokeWidth="2"/>
      <path d="M0 0h68c37 0 60 22 72 53-17-23-38-33-67-33H0Z" fill="#fff" opacity=".08"/>
      <rect width="13" height="426" x="0" y="13" fill="url(#rail)"/><path d="M14 31v387" stroke="#7d898f" strokeWidth="1"/><circle cx="7" cy="45" r="2.5" fill="#929ca1"/><circle cx="7" cy="406" r="2.5" fill="#929ca1"/>
      <path d="M30 0h51c21 0 39 8 50 23l-5 17c-8-12-21-20-39-20H34Z" fill="#10161a" stroke="#5d686e"/><text x="79" y="16" className="shoulderText">R</text>
      <path d="M43-5h47c17 1 31 7 42 17l-4 15c-8-9-18-15-32-15H50Z" fill="#080c0f" stroke="#4b565c"/><text x="91" y="8" className="shoulderText">ZR</text>
      <path d="M42 43h27M55.5 29v28" stroke="#151b1f" strokeWidth="8" strokeLinecap="round"/>
      <RoundButton x={80} y={115} text="X"/><RoundButton x={113} y={148} text="A"/><RoundButton x={80} y={181} text="B"/><RoundButton x={47} y={148} text="Y"/>
      <Stick x={80} y={275}/>
      <g transform="translate(80 374)"><circle r="15" fill="#12181c" stroke="#5e6970"/><circle r="10" fill="#252d32"/><path d="m-6 1 6-6 6 6v7H-6Z" fill="#77838a"/></g>
      <g fill="#11171b">{[0,1,2,3].map(i=><rect key={i} x="29" y={356+i*17} width="13" height="5" rx="2.5"/>)}</g>
    </g>}
  </svg>;
}
