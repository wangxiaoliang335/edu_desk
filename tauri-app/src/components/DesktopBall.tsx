import { useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';

import { getCurrentWindow } from '@tauri-apps/api/window';

export default function DesktopBall() {
    useEffect(() => {
        // Force transparency for the window root
        document.documentElement.style.background = 'transparent';
        document.body.style.background = 'transparent';
        const root = document.getElementById('root');
        if (root) root.style.background = 'transparent';
    }, []);

    const handleDoubleClick = async () => {
        // Coordinate calculation is now handled more accurately in the Rust backend
        invoke('open_tool_panel').catch(console.error);
        getCurrentWindow().hide();
    };

    return (
        <div
            className="w-20 h-20 flex items-center justify-center bg-transparent cursor-move group"
            onDoubleClick={handleDoubleClick}
            data-tauri-drag-region="true"
        >
            {/* Outer Glow / Ring */}
            <div
                className="absolute inset-0 rounded-full border-2 border-white/30 group-hover:border-white/50 transition-colors duration-500 animate-pulse"
                data-tauri-drag-region="true"
            />

            {/* Main Ball Container */}
            <div
                className="relative w-16 h-16 rounded-full overflow-hidden bg-white/10 backdrop-blur-md border border-white/20 shadow-xl group-active:scale-95 transition-transform duration-200"
                data-tauri-drag-region="true"
            >

                {/* Liquid Wave Effect */}
                <div className="absolute bottom-[-10%] left-[-50%] w-[200%] h-[120%] bg-gradient-to-t from-sage-500/60 to-sage-400/40 opacity-70 animate-wave rounded-[40%]" data-tauri-drag-region="true" />
                <div className="absolute bottom-[-15%] left-[-45%] w-[190%] h-[110%] bg-gradient-to-t from-clay-500/40 to-clay-400/20 opacity-50 animate-wave-slow rounded-[38%]" data-tauri-drag-region="true" />

                {/* School Logo Placeholder */}
                <div className="absolute inset-0 flex items-center justify-center p-3 z-10" data-tauri-drag-region="true">
                    <div className="w-full h-full flex items-center justify-center rounded-full bg-white/20 backdrop-blur-sm border border-white/40 shadow-inner" data-tauri-drag-region="true">
                        <span className="text-white font-black text-xl select-none" data-tauri-drag-region="true">校</span>
                    </div>
                </div>
            </div>

            <style>{`
        @keyframes wave {
          0% { transform: rotate(0deg); }
          100% { transform: rotate(360deg); }
        }
        .animate-wave {
          animation: wave 5s linear infinite;
        }
        .animate-wave-slow {
          animation: wave 8s linear infinite reverse;
        }
      `}</style>
        </div>
    );
}
