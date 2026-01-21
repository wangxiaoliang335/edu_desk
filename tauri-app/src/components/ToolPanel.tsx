import { useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow } from '@tauri-apps/api/window';

const tools = [
    { id: 'countdown', name: '倒计时', icon: '⏱️', color: 'from-sage-400 to-sage-600', pos: 0 }, // Top-Left
    { id: 'schedule', name: '课表', icon: '📅', color: 'from-clay-400 to-clay-600', pos: 1 },    // Top-Center
    { id: 'calendar', name: '校历', icon: '🏫', color: 'from-ink-400 to-ink-600', pos: 2 },      // Top-Right
    { id: 'filebox', name: '云盒', icon: '📁', color: 'from-sage-500 to-clay-500', pos: 3 },     // Mid-Left
    // BALL in center (pos 4)
    { id: 'settings', name: '设置', icon: '⚙️', color: 'from-ink-500 to-sage-500', pos: 5 },     // Mid-Right
    { id: 'back', name: '返回', icon: '🔙', color: 'from-slate-400 to-slate-600', pos: 6 },      // Bottom-Left
    { id: 'exit', name: '退出', icon: '🚪', color: 'from-ink-700 to-black', pos: 7 },           // Bottom-Center
    { id: 'minimize', name: '最小化', icon: '➖', color: 'from-blue-400 to-blue-600', pos: 8 },  // Bottom-Right
];

export default function ToolPanel() {
    useEffect(() => {
        document.documentElement.style.background = 'transparent';
        document.body.style.background = 'transparent';
        const root = document.getElementById('root');
        if (root) root.style.background = 'transparent';

        // Temporarily disabled auto-hide to debug dragging behavior
        // const handleBlur = () => {
        //    getCurrentWindow().hide().catch(console.error);
        // };
        // const unlistenPromise = getCurrentWindow().listen('tauri://blur', handleBlur);

        // return () => {
        //   unlistenPromise.then((unlisten) => unlisten());
        // };
    }, []);

    const handleToolClick = (toolId: string) => {
        if (toolId === 'countdown') {
            invoke('open_countdown_minimal_window').catch(console.error);
        } else if (toolId === 'schedule') {
            invoke('open_teacher_schedule_window').catch(console.error);
        } else if (toolId === 'calendar') {
            invoke('open_school_calendar_window').catch(console.error);
        } else if (toolId === 'filebox') {
            invoke('open_file_box_window', { boxId: 'main' }).catch(console.error);
        } else if (toolId === 'exit') {
            invoke('exit_app');
        } else if (toolId === 'back') {
            invoke('open_desktop_ball').catch(console.error);
            getCurrentWindow().hide().catch(console.error);
        } else if (toolId === 'minimize') {
            getCurrentWindow().hide().catch(console.error);
        }
    };

    const renderTool = (pos: number) => {
        const tool = tools.find(t => t.pos === pos);
        if (!tool) return <div key={pos} />;

        return (
            <button
                key={tool.id}
                onClick={() => handleToolClick(tool.id)}
                className="flex flex-col items-center justify-center group transition-all"
            >
                <div className={`w-12 h-12 rounded-xl bg-gradient-to-br ${tool.color} flex items-center justify-center text-xl shadow-sm group-hover:scale-110 group-active:scale-95 transition-all duration-300`}>
                    <span className="drop-shadow-sm">{tool.icon}</span>
                </div>
                <span className="mt-1 text-[9px] font-bold text-sage-900/60 uppercase tracking-wider">{tool.name}</span>
            </button>
        );
    };

    return (
        <div
            className="w-[300px] h-[300px] flex items-center justify-center p-2 animate-in fade-in zoom-in duration-300 cursor-move select-none bg-transparent"
            data-tauri-drag-region="true"
        >
            <div
                className="w-full h-full bg-white rounded-[2.5rem] shadow-[0_15px_40px_rgba(0,0,0,0.15)] border-2 border-sage-100 flex flex-col items-center justify-center relative overflow-hidden"
                data-tauri-drag-region="true"
            >

                {/* 3x3 Grid */}
                <div
                    className="grid grid-cols-3 grid-rows-3 gap-2 w-full h-full p-4 relative z-10"
                    data-tauri-drag-region="true"
                >
                    {renderTool(0)}
                    {renderTool(1)}
                    {renderTool(2)}

                    {renderTool(3)}

                    {/* Center Ball Visual */}
                    <div className="flex items-center justify-center" data-tauri-drag-region="true">
                        <div
                            className="w-16 h-16 rounded-full bg-gradient-to-br from-[#6b8c7e] to-[#8fb0a1] shadow-lg border-2 border-white flex items-center justify-center overflow-hidden cursor-move"
                            data-tauri-drag-region="true"
                        >
                            <div className="w-full h-full flex items-center justify-center text-white font-black text-xl" data-tauri-drag-region="true">校</div>
                        </div>
                    </div>

                    {renderTool(5)}

                    {renderTool(6)}
                    {renderTool(7)}
                    {renderTool(8)}
                </div>
            </div>
        </div>
    );
}
