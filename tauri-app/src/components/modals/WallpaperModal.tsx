import { useRef, useState, useEffect } from 'react';
import { X, Image as ImageIcon, Upload, Check, Download, RefreshCw, Loader2, Calendar } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessageWS } from '../../utils/websocket';

interface WallpaperModalProps {
    isOpen: boolean;
    onClose: () => void;
    groupId?: string;
}

interface Wallpaper {
    id: number;
    name: string;
    image_url: string;
    is_current?: number;
}

interface BackendResponse {
    code: number;
    message: string;
    data: {
        wallpapers: Wallpaper[];
    };
}

type DayOfWeek = 'monday' | 'tuesday' | 'wednesday' | 'thursday' | 'friday' | 'saturday' | 'sunday';
type WeeklySchedule = Record<DayOfWeek, Wallpaper | null>;

const DAYS: { key: DayOfWeek; label: string }[] = [
    { key: 'monday', label: '周一' },
    { key: 'tuesday', label: '周二' },
    { key: 'wednesday', label: '周三' },
    { key: 'thursday', label: '周四' },
    { key: 'friday', label: '周五' },
    { key: 'saturday', label: '周六' },
    { key: 'sunday', label: '周日' },
];

const WallpaperModal = ({ isOpen, onClose, groupId }: WallpaperModalProps) => {
    const [activeTab, setActiveTab] = useState<'class' | 'library'>('class');
    const [wallpapers, setWallpapers] = useState<Wallpaper[]>([]);
    const [selectedId, setSelectedId] = useState<number | null>(null);
    const [loading, setLoading] = useState(false);
    const [actionLoading, setActionLoading] = useState(false);
    const [uploading, setUploading] = useState(false);
    const [error, setError] = useState<string | null>(null);

    // Weekly wallpaper state
    const [showWeeklyPanel, setShowWeeklyPanel] = useState(false);
    const [weeklySchedule, setWeeklySchedule] = useState<WeeklySchedule>({
        monday: null, tuesday: null, wednesday: null, thursday: null,
        friday: null, saturday: null, sunday: null
    });
    const [weeklyEnabled, setWeeklyEnabled] = useState(false);
    const [savingWeekly, setSavingWeekly] = useState(false);

    // Custom Drag and Drop State
    const [dragState, setDragState] = useState<{
        item: Wallpaper;
        x: number;
        y: number;
        isDragging: boolean;
    } | null>(null);

    const fileInputRef = useRef<HTMLInputElement>(null);
    const { style, handleMouseDown } = useDraggable();

    // Fetch wallpapers when modal opens
    useEffect(() => {
        if (isOpen && groupId) {
            fetchWallpapers();
            fetchWeeklySchedule();
            setSelectedId(null);
        }
    }, [isOpen, groupId, activeTab]);

    // Custom Drag Effect
    useEffect(() => {
        if (!dragState?.isDragging) return;

        const handleMouseMove = (e: MouseEvent) => {
            setDragState(prev => prev ? { ...prev, x: e.clientX, y: e.clientY } : null);
        };

        const handleMouseUp = (e: MouseEvent) => {
            if (dragState) {
                // Check if dropped on a day using ElementFromPoint
                // We point to the cursor location
                const elements = document.elementsFromPoint(e.clientX, e.clientY);
                // Find any element with data-drop-zone attribute
                const dropZone = elements.find(el => el.getAttribute('data-drop-zone'));

                if (dropZone) {
                    const dayKey = dropZone.getAttribute('data-drop-zone') as DayOfWeek;
                    if (dayKey) {
                        console.log('[WallpaperModal] Custom Drop on', dayKey);
                        setWeeklySchedule(prev => ({ ...prev, [dayKey]: dragState.item }));
                    }
                }
            }
            setDragState(null);
        };

        window.addEventListener('mousemove', handleMouseMove);
        window.addEventListener('mouseup', handleMouseUp);

        return () => {
            window.removeEventListener('mousemove', handleMouseMove);
            window.removeEventListener('mouseup', handleMouseUp);
        };
    }, [dragState?.isDragging, dragState?.item]);

    const fetchWallpapers = async () => {
        setLoading(true);
        setError(null);
        setWallpapers([]);
        try {
            let resStr = "";
            if (activeTab === 'class' && groupId) {
                resStr = await invoke<string>('fetch_class_wallpapers', { groupId });
            } else if (activeTab === 'library') {
                resStr = await invoke<string>('fetch_wallpaper_library');
            }

            if (resStr) {
                const res: BackendResponse = JSON.parse(resStr);
                if (res.data && res.data.wallpapers) {
                    setWallpapers(res.data.wallpapers);
                }
            }
        } catch (e) {
            console.error(e);
            setError("加载壁纸失败");
        } finally {
            setLoading(false);
        }
    };

    const fetchWeeklySchedule = async () => {
        if (!groupId) return;
        try {
            const resStr = await invoke<string>('fetch_weekly_config', { groupId });
            console.log('[WallpaperModal] Weekly config raw response:', resStr);
            const res = JSON.parse(resStr);

            // Handle potentially different response structures
            // Log shows: {"data": {"code": 200, "is_enabled": true, "weekly_wallpapers": ...}}
            // Standard might be: {"code": 200, "data": { "is_enabled": true ... }}

            let data = null;
            if (res.data && res.data.weekly_wallpapers !== undefined) {
                // Case 1: Structure from log
                data = res.data;
            } else if (res.code === 200 && res.data) {
                // Case 2: Standard structure
                data = res.data;
            }

            if (data) {
                setWeeklyEnabled(data.is_enabled || false);
                console.log('[WallpaperModal] Weekly Enabled:', data.is_enabled);

                if (data.is_enabled) {
                    setShowWeeklyPanel(true);
                }

                if (data.weekly_wallpapers) {
                    const apiSchedule = data.weekly_wallpapers;
                    console.log('[WallpaperModal] API Schedule Keys:', Object.keys(apiSchedule));

                    const newSchedule: WeeklySchedule = {
                        monday: null, tuesday: null, wednesday: null, thursday: null,
                        friday: null, saturday: null, sunday: null
                    };

                    const dayMap: Record<string, DayOfWeek> = {
                        "0": "monday", "1": "tuesday", "2": "wednesday", "3": "thursday",
                        "4": "friday", "5": "saturday", "6": "sunday"
                    };

                    Object.keys(apiSchedule).forEach(key => {
                        const dayKey = dayMap[String(key)];
                        if (dayKey && apiSchedule[key]) {
                            newSchedule[dayKey] = {
                                id: -1,
                                name: "Scheduled Wallpaper",
                                image_url: apiSchedule[key],
                                is_current: 0
                            };
                        } else {
                            console.warn('[WallpaperModal] Unmapped key or empty url:', key, apiSchedule[key]);
                        }
                    });

                    console.log('[WallpaperModal] Constructed Schedule:', newSchedule);
                    setWeeklySchedule(newSchedule);
                } else {
                    console.log('[WallpaperModal] No weekly_wallpapers data found');
                }
            } else {
                console.warn('[WallpaperModal] Invalid response structure:', res);
            }
        } catch (e) {
            console.error('[WallpaperModal] Failed to fetch weekly config:', e);
        }
    };

    const saveWeeklySchedule = async () => {
        if (!groupId) return;

        const missingDays: string[] = [];
        for (const day of DAYS) {
            if (!weeklySchedule[day.key]) {
                missingDays.push(day.label);
            }
        }

        if (missingDays.length > 0) {
            alert(`请为所有日期设置壁纸，缺少: ${missingDays.join(', ')}`);
            return;
        }

        setSavingWeekly(true);
        try {
            const apiSchedule: Record<string, string> = {};
            const dayToApiMap: Record<DayOfWeek, string> = {
                "monday": "0", "tuesday": "1", "wednesday": "2", "thursday": "3",
                "friday": "4", "saturday": "5", "sunday": "6"
            };

            for (const day of DAYS) {
                const url = weeklySchedule[day.key]?.image_url;
                if (url) {
                    apiSchedule[dayToApiMap[day.key]] = url;
                }
            }

            console.log('[WallpaperModal] Applying weekly config:', apiSchedule);
            const resStr = await invoke<string>('apply_weekly_config', {
                groupId,
                weeklyWallpapers: apiSchedule
            });
            console.log('[WallpaperModal] Apply response:', resStr);

            const res = JSON.parse(resStr);
            if (res.code === 200 || (res.data && res.data.code === 200)) {
                alert("一周壁纸设置已应用");
                setWeeklyEnabled(true);
            } else {
                alert("应用失败: " + (res.message || res.data?.message || JSON.stringify(res)));
            }
        } catch (e) {
            console.error('[WallpaperModal] Failed to apply weekly config:', e);
            alert("应用失败: " + String(e));
        } finally {
            setSavingWeekly(false);
        }
    };

    const handleRemoveFromDay = (day: DayOfWeek) => {
        setWeeklySchedule(prev => ({ ...prev, [day]: null }));
    };

    const handleAction = async () => {
        if (!selectedId || !groupId) return;
        setActionLoading(true);

        try {
            if (activeTab === 'class') {
                if (weeklyEnabled) {
                    await invoke('disable_weekly_wallpaper', { groupId });
                    setWeeklyEnabled(false);
                    setShowWeeklyPanel(false);
                }

                const message = {
                    type: "set_wallpaper",
                    wallpaper_id: selectedId
                };
                const wsMessage = `to:${groupId}:${JSON.stringify(message)}`;
                console.log('[WallpaperModal] Sending via WebSocket:', wsMessage);
                sendMessageWS(wsMessage);

                await invoke('set_class_wallpaper', { groupId, wallpaperId: selectedId });
                alert("已设置壁纸成功，已通知班级端");
                fetchWallpapers();
            } else {
                console.log('[WallpaperModal] Downloading wallpaper from library:', { groupId, wallpaperId: selectedId });
                const result = await invoke<string>('download_wallpaper', { groupId, wallpaperId: selectedId });

                const parsed = JSON.parse(result);
                if (parsed.code === 200 || parsed.code === 201 || (parsed.data && (parsed.data.code === 200 || parsed.data.code === 201))) {
                    const serverMessage = parsed.message || parsed.data?.message || '';
                    if (serverMessage.includes('已存在')) {
                        alert("该壁纸已存在于班级壁纸中");
                    } else {
                        alert("已下载到班级壁纸");
                    }
                    setActiveTab('class');
                } else {
                    alert("下载失败: " + (parsed.message || parsed.data?.message || JSON.stringify(parsed)));
                }
            }
        } catch (e) {
            console.error('[WallpaperModal] Action error:', e);
            alert("操作失败: " + String(e));
        } finally {
            setActionLoading(false);
        }
    };

    const handleSelectFile = () => {
        if (fileInputRef.current) {
            fileInputRef.current.click();
        }
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        if (fileInputRef.current) fileInputRef.current.value = "";

        if (!["image/png", "image/jpeg", "image/gif", "image/bmp"].includes(file.type)) {
            alert("请选择图片文件 (PNG, JPG, BMP, GIF)");
            return;
        }

        setUploading(true);
        try {
            const buffer = await file.arrayBuffer();
            const bytes = Array.from(new Uint8Array(buffer));

            const res = await invoke<string>('upload_wallpaper', {
                groupId,
                imageData: bytes,
                mimeType: file.type
            });

            const parsed = JSON.parse(res);
            if (parsed.code === 200 || parsed.code === 201 || (parsed.data && (parsed.data.code === 200 || parsed.data.code === 201))) {
                fetchWallpapers();
            } else {
                alert("上传失败: " + (parsed.message || parsed.data?.message || JSON.stringify(parsed)));
            }
        } catch (e) {
            alert("上传失败: " + String(e));
        } finally {
            setUploading(false);
        }
    };

    const startDrag = (e: React.MouseEvent, wp: Wallpaper) => {
        // Only allow drag if weekly panel is open and class tab is active
        if (activeTab !== 'class' || !showWeeklyPanel) return;

        e.preventDefault(); // Prevent text selection
        setDragState({
            item: wp,
            x: e.clientX,
            y: e.clientY,
            isDragging: true
        });
    };

    // Render Ghost Element
    const renderDragGhost = () => {
        if (!dragState || !dragState.isDragging) return null;
        return (
            <div
                className="fixed z-[9999] pointer-events-none opacity-80 shadow-2xl rounded-xl overflow-hidden border-2 border-orange-500"
                style={{
                    left: dragState.x,
                    top: dragState.y,
                    width: '100px', // Approximate size
                    height: '133px',
                    transform: 'translate(-50%, -50%)'
                }}
            >
                <img src={dragState.item.image_url} className="w-full h-full object-cover" alt="" />
            </div>
        );
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            {renderDragGhost()}

            {/* Hidden file input for upload */}
            <input
                type="file"
                ref={fileInputRef}
                onChange={handleFileChange}
                accept="image/png,image/jpeg,image/gif,image/bmp"
                className="hidden"
            />
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl w-[900px] h-[650px] flex flex-col overflow-hidden border border-white/60 ring-1 ring-sage-100/50 animate-in zoom-in-95 duration-200"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="h-20 flex items-center justify-between px-8 border-b border-sage-100/50 bg-white/40 backdrop-blur-md cursor-move select-none"
                >
                    <div className="flex items-center gap-4">
                        <div className="w-12 h-12 rounded-2xl bg-gradient-to-br from-violet-400 to-violet-600 text-white flex items-center justify-center shadow-lg shadow-violet-500/20">
                            <ImageIcon size={24} />
                        </div>
                        <h3 className="font-bold text-ink-800 text-xl tracking-tight">班级壁纸</h3>
                    </div>
                    <div className="flex items-center gap-3">
                        {/* Weekly Wallpaper Toggle */}
                        {activeTab === 'class' && (
                            <button
                                onClick={() => setShowWeeklyPanel(!showWeeklyPanel)}
                                className={`px-4 py-2 rounded-xl text-sm font-bold transition-all flex items-center gap-2 mr-2 ${showWeeklyPanel
                                    ? 'bg-orange-500 text-white shadow-lg shadow-orange-500/20'
                                    : 'bg-orange-50 text-orange-600 border border-orange-100 hover:bg-orange-100'
                                    }`}
                            >
                                <Calendar size={16} />
                                一周壁纸
                                {weeklyEnabled && <span className="w-2 h-2 bg-emerald-400 rounded-full border border-white" />}
                            </button>
                        )}

                        {/* Tabs */}
                        <div className="flex bg-sage-100/50 rounded-xl p-1.5 gap-1 mr-2">
                            <button
                                onClick={() => setActiveTab('class')}
                                className={`px-4 py-2 rounded-lg text-sm font-bold transition-all ${activeTab === 'class'
                                    ? 'bg-white text-ink-800 shadow-sm'
                                    : 'text-sage-500 hover:text-sage-700 hover:bg-white/50'
                                    }`}
                            >
                                班级壁纸
                            </button>
                            <button
                                onClick={() => setActiveTab('library')}
                                className={`px-4 py-2 rounded-lg text-sm font-bold transition-all ${activeTab === 'library'
                                    ? 'bg-white text-ink-800 shadow-sm'
                                    : 'text-sage-500 hover:text-sage-700 hover:bg-white/50'
                                    }`}
                            >
                                壁纸库
                            </button>
                        </div>

                        {/* Action Button */}
                        {activeTab === 'class' ? (
                            <button
                                onClick={handleAction}
                                disabled={!selectedId || actionLoading}
                                className={`px-5 py-2.5 rounded-xl text-sm font-bold transition-all flex items-center gap-2 ${!selectedId
                                    ? 'bg-sage-100 text-sage-400 cursor-not-allowed'
                                    : 'bg-violet-500 text-white hover:bg-violet-600 shadow-lg shadow-violet-500/20 hover:-translate-y-0.5'
                                    }`}
                            >
                                {actionLoading ? <Loader2 size={16} className="animate-spin" /> : <Check size={16} />}
                                设为壁纸
                            </button>
                        ) : (
                            <button
                                onClick={handleAction}
                                disabled={!selectedId || actionLoading}
                                className={`px-5 py-2.5 rounded-xl text-sm font-bold transition-all flex items-center gap-2 ${!selectedId
                                    ? 'bg-sage-100 text-sage-400 cursor-not-allowed'
                                    : 'bg-blue-500 text-white hover:bg-blue-600 shadow-lg shadow-blue-500/20 hover:-translate-y-0.5'
                                    }`}
                            >
                                {actionLoading ? <Loader2 size={16} className="animate-spin" /> : <Download size={16} />}
                                下载
                            </button>
                        )}

                        <button
                            onClick={onClose}
                            className="ml-2 w-10 h-10 flex items-center justify-center rounded-full text-sage-400 hover:text-clay-600 hover:bg-clay-50 transition-all duration-300"
                        >
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {/* Weekly Wallpaper Panel */}
                {showWeeklyPanel && activeTab === 'class' && (
                    <div className="bg-violet-50/50 border-b border-violet-100/50 p-5 backdrop-blur-sm animate-in slide-in-from-top-2 duration-200">
                        <div className="flex items-center justify-between mb-4">
                            <span className="text-sm font-bold text-violet-800 flex items-center gap-2">
                                <span className="w-1.5 h-1.5 rounded-full bg-violet-400"></span>
                                拖拽下方壁纸到对应日期
                            </span>
                            <button
                                onClick={saveWeeklySchedule}
                                disabled={savingWeekly}
                                className="px-5 py-1.5 bg-violet-500 text-white rounded-xl text-xs font-bold hover:bg-violet-600 transition-all shadow-md shadow-violet-500/10 flex items-center gap-1.5"
                            >
                                {savingWeekly ? <Loader2 size={14} className="animate-spin" /> : <Check size={14} />}
                                应用设置
                            </button>
                        </div>
                        <div className="grid grid-cols-7 gap-3">
                            {DAYS.map(day => (
                                <div
                                    key={day.key}
                                    data-drop-zone={day.key}
                                    className="flex flex-col items-center cursor-default group/day relative z-10"
                                >
                                    <span className="text-xs font-bold text-violet-600 mb-2 pointer-events-none">{day.label}</span>
                                    <div
                                        className={`w-full aspect-[3/4] rounded-2xl border-2 border-dashed flex items-center justify-center overflow-hidden transition-all pointer-events-none ${weeklySchedule[day.key]
                                            ? 'border-violet-300 bg-white/80 shadow-sm'
                                            : 'border-violet-200/60 bg-white/40 group-hover/day:border-violet-400 group-hover/day:bg-white/60'
                                            }`}
                                    >
                                        {weeklySchedule[day.key] ? (
                                            <div className="relative w-full h-full group pointer-events-none">
                                                <img
                                                    src={weeklySchedule[day.key]!.image_url}
                                                    alt=""
                                                    className="w-full h-full object-cover"
                                                />
                                                <button
                                                    onClick={(e) => {
                                                        e.stopPropagation();
                                                        e.preventDefault();
                                                        handleRemoveFromDay(day.key);
                                                    }}
                                                    className="absolute top-1 right-1 w-5 h-5 bg-clay-500 text-white rounded-full flex items-center justify-center opacity-0 group-hover:opacity-100 transition-all hover:scale-110 pointer-events-auto cursor-pointer shadow-sm"
                                                >
                                                    <X size={12} strokeWidth={3} />
                                                </button>
                                            </div>
                                        ) : (
                                            <ImageIcon size={20} className="text-violet-200 pointer-events-none" />
                                        )}
                                    </div>
                                </div>
                            ))}
                        </div>
                    </div>
                )}

                {/* Content */}
                <div className="flex-1 overflow-y-auto p-6 bg-white/30 custom-scrollbar">
                    {loading ? (
                        <div className="flex flex-col items-center justify-center h-full gap-3">
                            <Loader2 className="animate-spin text-violet-500" size={32} />
                            <span className="text-sage-500 text-sm font-medium">加载中...</span>
                        </div>
                    ) : error ? (
                        <div className="flex flex-col items-center justify-center h-full gap-3">
                            <span className="text-clay-500 font-bold">{error}</span>
                            <button onClick={fetchWallpapers} className="flex items-center gap-2 text-violet-500 hover:text-violet-600 font-bold px-4 py-2 hover:bg-violet-50 rounded-xl transition-all">
                                <RefreshCw size={16} />
                                重试
                            </button>
                        </div>
                    ) : wallpapers.length === 0 ? (
                        <div className="flex flex-col items-center justify-center h-full gap-5">
                            <div className="w-24 h-24 bg-sage-50 rounded-[2rem] flex items-center justify-center border border-sage-100">
                                <ImageIcon size={40} className="text-sage-300" />
                            </div>
                            <p className="text-sage-400 font-bold text-lg">暂无壁纸</p>
                            {activeTab === 'class' && (
                                <button
                                    onClick={handleSelectFile}
                                    disabled={uploading}
                                    className="px-6 py-3 bg-violet-500 text-white rounded-2xl font-bold flex items-center gap-2 hover:bg-violet-600 transition-all shadow-lg shadow-violet-500/20 hover:-translate-y-0.5"
                                >
                                    {uploading ? <Loader2 size={20} className="animate-spin" /> : <Upload size={20} />}
                                    上传壁纸
                                </button>
                            )}
                        </div>
                    ) : (
                        <div className="grid grid-cols-5 gap-5">
                            {wallpapers.map(wp => {
                                const isSelected = selectedId === wp.id;
                                const isCurrent = activeTab === 'class' && wp.is_current === 1;
                                return (
                                    <div
                                        key={wp.id}
                                        onMouseDown={(e) => startDrag(e, wp)}
                                        onClick={() => setSelectedId(wp.id)}
                                        className={`group relative aspect-[3/4] bg-white rounded-2xl overflow-hidden cursor-pointer border-2 transition-all shadow-sm hover:shadow-lg ${isSelected
                                            ? 'border-violet-500 shadow-violet-200/50 scale-[1.02]'
                                            : 'border-white/60 hover:border-violet-200'
                                            } ${activeTab === 'class' && showWeeklyPanel ? 'cursor-grab active:cursor-grabbing' : ''}`}
                                    >
                                        <img src={wp.image_url} alt={wp.name} className="w-full h-full object-cover" />

                                        {/* Name Overlay */}
                                        <div className="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-black/80 via-black/40 to-transparent p-3 pt-6 text-center transform translate-y-full group-hover:translate-y-0 transition-transform duration-300">
                                            <span className="text-white text-xs font-bold truncate block">
                                                {wp.name || `壁纸 ${wp.id}`}
                                            </span>
                                        </div>

                                        {/* Current Badge */}
                                        {isCurrent && (
                                            <div className="absolute top-3 right-3 bg-emerald-100/90 backdrop-blur-sm text-emerald-700 text-[10px] px-2.5 py-1 rounded-lg border border-emerald-200 font-bold shadow-sm">
                                                使用中
                                            </div>
                                        )}

                                        {/* Selected Border/Indicator */}
                                        {isSelected && (
                                            <div className="absolute inset-0 border-[3px] border-violet-500 rounded-2xl pointer-events-none">
                                                <div className="absolute top-2 left-2 w-7 h-7 bg-violet-500 rounded-full flex items-center justify-center shadow-md">
                                                    <Check size={16} className="text-white" strokeWidth={3} />
                                                </div>
                                            </div>
                                        )}
                                    </div>
                                );
                            })}

                            {/* Upload Card (Class Tab Only) - at the end */}
                            {activeTab === 'class' && (
                                <div
                                    onClick={handleSelectFile}
                                    className="aspect-[3/4] bg-white/50 border-2 border-dashed border-sage-200 rounded-2xl flex flex-col items-center justify-center gap-3 cursor-pointer hover:border-violet-300 hover:bg-violet-50/50 transition-all group"
                                >
                                    {uploading ? (
                                        <>
                                            <Loader2 size={32} className="animate-spin text-violet-500" />
                                            <span className="text-xs font-bold text-violet-400">上传中...</span>
                                        </>
                                    ) : (
                                        <>
                                            <div className="w-14 h-14 bg-white/80 group-hover:bg-violet-100 rounded-2xl flex items-center justify-center transition-colors shadow-sm">
                                                <Upload size={24} className="text-sage-400 group-hover:text-violet-500 transition-colors" />
                                            </div>
                                            <span className="text-xs text-sage-500 font-bold group-hover:text-violet-600 transition-colors">添加壁纸</span>
                                        </>
                                    )}
                                </div>
                            )}
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default WallpaperModal;
