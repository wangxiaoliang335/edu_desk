import { useEffect, useState } from 'react';
import { X, Clock, Calendar as CalendarIcon, Save } from 'lucide-react';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { invoke } from '@tauri-apps/api/core';
import { emit } from '@tauri-apps/api/event';

const loadCountdownConfig = () => {
    const saved = localStorage.getItem('countdown_config');
    return saved ? JSON.parse(saved) : {
        title: '距离高考',
        targetDate: new Date(new Date().getFullYear(), 5, 7).toISOString().split('T')[0]
    };
};

const CountdownEditWindow = () => {
    const initial = loadCountdownConfig();
    const [targetDate, setTargetDate] = useState(initial.targetDate);
    const [title, setTitle] = useState(initial.title);
    const [daysLeft, setDaysLeft] = useState(0);

    useEffect(() => {
        const now = new Date();
        const target = new Date(targetDate);
        const diffTime = target.getTime() - now.getTime();
        const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
        setDaysLeft(diffDays);
    }, [targetDate]);

    const closeAndReturn = async () => {
        await invoke('open_countdown_minimal_window');
        await getCurrentWindow().close();
    };

    const handleSave = async () => {
        const next = { title, targetDate };
        localStorage.setItem('countdown_config', JSON.stringify(next));
        await emit('countdown-config-updated');
        await closeAndReturn();
    };

    return (
        <div className="w-full h-full bg-paper flex items-center justify-center">
            <div className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[450px] overflow-hidden border border-white/60 ring-1 ring-sage-100/50 flex flex-col">
                {/* Header */}
                <div
                    data-tauri-drag-region="true"
                    className="p-5 flex items-center justify-between border-b border-sage-100/50 bg-white/40 backdrop-blur-md select-none"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-2xl bg-gradient-to-br from-clay-400 to-clay-600 text-white flex items-center justify-center shadow-lg shadow-clay-500/20">
                            <Clock size={20} />
                        </div>
                        <span className="font-bold text-ink-800 text-lg tracking-tight">倒计时设置</span>
                    </div>
                    <button
                        onClick={closeAndReturn}
                        className="w-9 h-9 flex items-center justify-center rounded-full text-sage-400 hover:text-clay-600 hover:bg-clay-50 transition-all duration-300"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-8 flex flex-col items-center bg-white/30">
                    {/* Preview Card */}
                    <div className="bg-gradient-to-br from-white to-clay-50 border border-clay-100 rounded-3xl p-6 w-full flex flex-col items-center justify-center mb-6 shadow-sm relative overflow-hidden">
                        <div className="absolute top-0 right-0 p-3 opacity-10">
                            <Clock size={80} className="text-clay-500" />
                        </div>
                        <span className="text-ink-500 font-bold mb-2 tracking-wide uppercase text-xs">{title || '事件名称'}</span>
                        <div className="flex items-baseline gap-2 z-10">
                            <span className="text-7xl font-black text-clay-600 tracking-tighter drop-shadow-sm">{daysLeft > 0 ? daysLeft : 0}</span>
                            <span className="text-clay-400 font-bold text-lg">天</span>
                        </div>
                        {daysLeft <= 0 && (
                            <span className="text-xs text-clay-500 mt-2 font-bold bg-clay-100 px-3 py-1 rounded-full">已到达或过期!</span>
                        )}
                    </div>

                    {/* Settings */}
                    <div className="w-full space-y-5">
                        <div className="space-y-2">
                            <label className="text-xs font-bold text-sage-500 flex items-center gap-1 uppercase tracking-wide">事件名称</label>
                            <input
                                type="text"
                                value={title}
                                onChange={(e) => setTitle(e.target.value)}
                                className="w-full text-sm font-bold text-ink-700 bg-white/80 border border-sage-200 rounded-xl p-3 outline-none focus:ring-2 focus:ring-clay-100 focus:border-clay-400 transition-all shadow-sm"
                                placeholder="例如：距高考还有"
                            />
                        </div>
                        <div className="space-y-2">
                            <label className="text-xs font-bold text-sage-500 flex items-center gap-1 uppercase tracking-wide">
                                <CalendarIcon size={12} /> 目标日期
                            </label>
                            <input
                                type="date"
                                value={targetDate}
                                onChange={(e) => setTargetDate(e.target.value)}
                                className="w-full text-sm font-bold text-ink-700 bg-white/80 border border-sage-200 rounded-xl p-3 outline-none focus:ring-2 focus:ring-clay-100 focus:border-clay-400 transition-all shadow-sm"
                            />
                        </div>
                    </div>
                </div>

                {/* Footer */}
                <div className="p-5 bg-white/40 border-t border-sage-100/50 flex justify-end gap-3 backdrop-blur-md">
                    <button
                        onClick={closeAndReturn}
                        className="px-5 py-2.5 text-sm font-bold text-sage-500 hover:bg-white hover:text-sage-700 rounded-xl border border-transparent hover:border-sage-200 transition-all"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleSave}
                        className="px-6 py-2.5 text-sm font-bold text-white bg-clay-500 hover:bg-clay-600 rounded-xl shadow-lg shadow-clay-500/20 flex items-center gap-2 transition-all hover:-translate-y-0.5"
                    >
                        <Save size={16} /> 保存设置
                    </button>
                </div>
            </div>
        </div>
    );
};

export default CountdownEditWindow;
