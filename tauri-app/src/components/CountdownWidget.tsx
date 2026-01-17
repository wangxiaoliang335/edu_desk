import { useState, useEffect } from 'react';
import { useDraggable } from '../hooks/useDraggable';


interface CountdownWidgetProps {
    visible: boolean;
    config: {
        title: string;
        targetDate: string;
    };
    onEdit: () => void;
    isWindowMode?: boolean;
    mode?: 'full' | 'minimal';
    onRequestModeChange?: (mode: 'full' | 'minimal') => void;
}

const CountdownWidget = ({ visible, config, onEdit, isWindowMode = false, mode: forcedMode, onRequestModeChange }: CountdownWidgetProps) => {
    // Always start in minimal mode as requested
    const [mode, setMode] = useState<'full' | 'minimal'>('minimal');
    const [daysLeft, setDaysLeft] = useState(0);
    const { style, handleMouseDown } = useDraggable();
    const effectiveMode = forcedMode ?? mode;

    useEffect(() => {
        if (forcedMode) return;
        localStorage.setItem('countdown_mode', mode);
    }, [mode, forcedMode]);

    useEffect(() => {
        const calculateDays = () => {
            const now = new Date();
            const target = new Date(config.targetDate);
            const diffTime = target.getTime() - now.getTime();
            const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
            setDaysLeft(diffDays);
        };
        calculateDays();
        const timer = setInterval(calculateDays, 60000); // Update every minute
        return () => clearInterval(timer);
    }, [config.targetDate]);



    if (!visible) return null;

    const requestMode = (nextMode: 'full' | 'minimal') => {
        if (forcedMode) {
            onRequestModeChange?.(nextMode);
        } else {
            setMode(nextMode);
        }
    };

    const handleDoubleClick = () => {
        if (effectiveMode === 'minimal') {
            requestMode('full');
        } else {
            onEdit();
        }
    };

    if (effectiveMode === 'minimal') {

        const minimalClass = isWindowMode
            ? "w-full h-full select-none overflow-hidden cursor-move flex items-center justify-center"
            : "fixed top-[100px] left-1/2 -translate-x-1/2 z-[40] cursor-move select-none animate-in fade-in zoom-in-95 duration-300";

        const draggingProps = isWindowMode
            ? { "data-tauri-drag-region": "true" }
            : { onMouseDown: handleMouseDown, style };

        const pillClass = isWindowMode
            ? "w-full h-full rounded-full text-slate-700 font-semibold text-[15px] flex items-center justify-center gap-2 text-center cursor-default bg-gradient-to-r from-pink-50 via-amber-50 to-sky-50 border border-amber-100 shadow-[0_8px_18px_rgba(30,41,59,0.14)]"
            : "bg-sage-600/90 backdrop-blur-md text-white px-6 py-2 rounded-full shadow-[0_8px_20px_rgb(0,0,0,0.15)] border border-white/20 flex items-center gap-3 ring-4 ring-white/30 transition-all hover:scale-105 active:scale-95 cursor-default";

        return (
            <div
                className={minimalClass}
                onDoubleClick={handleDoubleClick}
                {...(isWindowMode ? draggingProps : draggingProps)}
            >
                <div
                    {...(isWindowMode ? draggingProps : {})}
                    className={pillClass}
                >
                    <span className="pointer-events-none">{config.title}</span>
                    <span className="pointer-events-none">还有</span>
                    <span className="text-xl font-black text-amber-600 pointer-events-none">{daysLeft > 0 ? daysLeft : 0}</span>
                    <span className="pointer-events-none">天</span>
                </div>
            </div>
        );
    }

    // Full Mode
    const fullClass = isWindowMode
        ? "w-fit h-fit flex flex-col items-center justify-center font-sans overflow-hidden bg-transparent"
        : "fixed top-20 left-1/2 -translate-x-1/2 z-[40] flex flex-col items-center animate-in slide-in-from-top-4 fade-in duration-500 font-sans";

    const containerProps = isWindowMode
        ? {}
        : { style, onMouseDown: handleMouseDown }; // Window mode doesn't drag container, it drags header

    return (
        <div
            className={fullClass}
            {...containerProps}
        >
            <div
                className={isWindowMode
                    ? "w-[320px] bg-paper rounded-[1.5rem] shadow-none border border-sage-100/60 overflow-hidden"
                    : "w-[320px] bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-[0_20px_50px_rgb(0,0,0,0.1)] border border-white/60 ring-1 ring-sage-100/50 overflow-hidden group hover:shadow-[0_25px_60px_rgb(0,0,0,0.15)] transition-all duration-300"
                }
                onDoubleClick={onEdit}
            >
                {/* Header */}
                <div
                    {...(isWindowMode ? { "data-tauri-drag-region": "true" } : { onMouseDown: handleMouseDown })}
                    onDoubleClick={onEdit}
                    className={isWindowMode
                        ? "h-12 bg-gradient-to-r from-sage-400 to-sage-500 flex items-center justify-between px-5 cursor-move select-none relative overflow-hidden"
                        : "h-14 bg-gradient-to-r from-sage-400 to-sage-500 flex items-center justify-between px-5 cursor-move select-none relative overflow-hidden"
                    }
                >
                    {isWindowMode && (
                        <>
                            <div className="absolute -top-2 -left-2 h-6 w-6 bg-paper rounded-br-[14px]" />
                            <div className="absolute -top-2 -right-2 h-6 w-6 bg-paper rounded-bl-[14px]" />
                        </>
                    )}
                    {/* Shine effect */}
                    <div className="absolute inset-0 bg-white/10 translate-x-[-100%] group-hover:translate-x-[100%] transition-transform duration-1000 rotate-45 pointer-events-none"></div>

                    <div className="flex items-center gap-2 text-white pointer-events-none flex-1">
                        <div className="w-6 h-6 rounded-full bg-white/20 flex items-center justify-center text-xs font-bold border border-white/30">
                            1
                        </div>
                        <span className="font-bold text-sm tracking-wide text-shadow-sm">{config.title} 还有</span>
                    </div>

                    <button
                        onClick={(event) => {
                            event.stopPropagation();
                            if (isWindowMode && onRequestModeChange) {
                                onRequestModeChange('minimal');
                                return;
                            }
                            requestMode('minimal');
                        }}
                        onDoubleClick={(event) => {
                            event.stopPropagation();
                            event.preventDefault();
                            if (isWindowMode && onRequestModeChange) {
                                onRequestModeChange('minimal');
                            }
                        }}
                        onMouseDown={(event) => {
                            event.stopPropagation();
                            event.preventDefault();
                        }}
                        onPointerDown={(event) => {
                            event.stopPropagation();
                            event.preventDefault();
                        }}
                        {...(isWindowMode ? { "data-tauri-drag-region": "false" } : {})}
                        className="bg-white/20 hover:bg-white/30 text-white text-xs font-bold px-3 py-1 rounded-lg border border-white/20 backdrop-blur-sm transition-colors flex items-center gap-1 active:scale-95"
                    >
                        <span>极简</span>
                    </button>
                </div>

                {/* Body */}
                <div
                    className="p-8 bg-white/50 flex flex-col items-center justify-center relative select-none"
                    onDoubleClick={onEdit}
                >
                    <div className="flex items-baseline gap-2 scale-110">
                        <span className="text-8xl font-black text-ink-800 tracking-tighter drop-shadow-sm custom-number-font">
                            {daysLeft > 0 ? daysLeft : 0}
                        </span>
                        <span className="text-2xl font-bold text-sage-500 mb-2">天</span>
                    </div>
                    {daysLeft <= 0 && (
                        <span className="mt-2 px-3 py-1 bg-clay-100 text-clay-600 rounded-full text-xs font-bold">
                            已到达目标日期
                        </span>
                    )}
                </div>

                {/* Footer */}
                <div
                    className="bg-sage-50/80 border-t border-sage-100/50 p-3 flex flex-col items-center justify-center text-xs font-medium text-ink-400 select-none"
                    onDoubleClick={onEdit}
                >
                    <div className="flex items-center gap-2 opacity-80">
                        <span className="w-1.5 h-1.5 rounded-full bg-clay-400"></span>
                        <span>目标日: {config.targetDate} {new Date(config.targetDate).toLocaleDateString('zh-CN', { weekday: 'long' })}</span>
                        <span className="w-1.5 h-1.5 rounded-full bg-clay-400"></span>
                    </div>
                </div>
            </div>
            {/* Hint Tooltip - Only show in non-window mode or if window mode is large enough? Window mode size clips this. Hide in Window Mode */}
            {!isWindowMode && (
                <div className="mt-4 opacity-0 group-hover:opacity-100 transition-opacity duration-300 bg-black/60 backdrop-blur-md text-white text-xs px-3 py-1.5 rounded-full pointer-events-none shadow-lg transform translate-y-2 group-hover:translate-y-0">
                    双击卡片编辑 · 点击极简切换
                </div>
            )}
        </div>
    );
};

export default CountdownWidget;
