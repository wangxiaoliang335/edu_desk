
import { useRef, useState, useEffect, useMemo } from 'react';
import { getCurrentWindow, LogicalPosition, LogicalSize } from '@tauri-apps/api/window';
import { format, addDays, getMonth, isSameDay, addWeeks, startOfWeek } from 'date-fns';
import {
    SchoolEvent,
    loadEvents,
    saveEvents,
    formatDateKey
} from '../utils/schoolEvents';
import * as XLSX from 'xlsx';
import { Upload, X, Trash2, Plus, FileText, Check, AlertCircle, Info, Calendar as CalendarIcon, Tag, ZoomIn, ZoomOut, Minimize2, Star, Gift, GraduationCap, Heart } from 'lucide-react';
import '../styles/calendar.css';

interface SchoolCalendarProps {
    onClose?: () => void;
}

const SchoolCalendar = ({ onClose }: SchoolCalendarProps) => {
    const [events, setEvents] = useState<SchoolEvent[]>([]);
    const [holidayData, setHolidayData] = useState<Record<string, any>>({});
    const [selectedDate, setSelectedDate] = useState<Date | null>(null);
    const [isDialogOpen, setIsDialogOpen] = useState(false);
    const [eventInput, setEventInput] = useState('');
    const fileInputRef = useRef<HTMLInputElement>(null);
    const [scale, setScale] = useState<number>(() => {
        const saved = localStorage.getItem('school_calendar_scale');
        return saved ? parseFloat(saved) : 1.4; // 默认缩放进一步加大
    });
    const [specialIcons, setSpecialIcons] = useState<Record<string, string>>(() => {
        const saved = localStorage.getItem('school_calendar_special_icons');
        return saved ? JSON.parse(saved) : {};
    });

    const [semesterStartDate] = useState<Date>(() => {
        const now = new Date();
        const year = now.getFullYear();
        const month = now.getMonth() + 1;
        
        if (month >= 9 || month === 1) {
            // 第一学期（秋季）：9月1日左右开学
            const startYear = month === 1 ? year - 1 : year;
            return startOfWeek(new Date(startYear, 8, 1), { weekStartsOn: 1 });
        } else {
            // 第二学期（春季）：2月16日左右开学
            return startOfWeek(new Date(year, 1, 16), { weekStartsOn: 1 });
        }
    });

    const [semesterWeeks] = useState(24); // 增加到24周，确保覆盖寒暑假开始

    const monthColors: Record<number, string> = {
        2: 'bg-[#fce7f3]', // Feb - Light Pink
        3: 'bg-[#fef9c3]', // Mar - Light Yellow
        4: 'bg-[#ffedd5]', // Apr - Light Orange
        5: 'bg-[#f0fdf4]', // May - Light Green
        6: 'bg-[#f0f9ff]', // Jun - Light Blue
        7: 'bg-[#fef9c3]', // Jul - Light Yellow
    };

    const getMonthColor = (month: number) => {
        return monthColors[month] || 'bg-white';
    };

    const monthBgColors: Record<number, string> = {
        1: '#dbeafe', // Jan - Light Blue
        2: '#fce7f3', // Feb - Light Pink
        3: '#fef9c3', // Mar - Light Yellow
        4: '#e9d5ff', // Apr - Light Purple
        5: '#dcfce7', // May - Light Green
        6: '#ffe4e6', // Jun - Light Rose
        7: '#dbeafe', // Jul - Light Blue
        8: '#fef3c7', // Aug - Light Amber
        9: '#e0f2fe', // Sep - Light Sky
        10: '#fde68a', // Oct - Light Gold
        11: '#e5e7eb', // Nov - Light Gray
        12: '#f1f5f9', // Dec - Light Slate
    };

    const getMonthBg = (month: number) => {
        return monthBgColors[month] || '#ffffff';
    };

    // 中文数字转换
    const toChineseNum = (n: number) => {
        const chars = ['零', '一', '二', '三', '四', '五', '六', '七', '八', '九', '十', '十一', '十二', '十三', '十四', '十五', '十六', '十七', '十八', '十九', '二十'];
        return chars[n] || n;
    };

    const getAcademicYearLabel = (date: Date) => {
        const year = date.getFullYear();
        const month = date.getMonth() + 1;
        // Typically Sep–Jan belongs to the "first semester" of the academic year
        if (month >= 9 || month === 1) {
            const startYear = month === 1 ? year - 1 : year;
            return `${startYear}-${startYear + 1}学年度第一学期`;
        }
        // Feb–Jul belongs to the "second semester"
        return `${year - 1}-${year}学年度第二学期`;
    };

    const getSemesterSeason = (date: Date) => {
        const month = date.getMonth() + 1;
        if (month >= 2 && month <= 7) return '春季';
        return '秋季';
    };

    // 保存缩放比例
    useEffect(() => {
        localStorage.setItem('school_calendar_scale', scale.toString());
    }, [scale]);

    // 保存特殊图标
    useEffect(() => {
        localStorage.setItem('school_calendar_special_icons', JSON.stringify(specialIcons));
    }, [specialIcons]);

    // 恢复窗口位置和大小
    useEffect(() => {
        const restoreWindowState = async () => {
            try {
                const window = getCurrentWindow();
                const saved = localStorage.getItem('school_calendar_window_state');
                if (saved) {
                    const state = JSON.parse(saved);
                    if (state.x !== undefined && state.y !== undefined) {
                        await window.setPosition(new LogicalPosition(state.x, state.y));
                    }
                    if (state.width !== undefined && state.height !== undefined) {
                        await window.setSize(new LogicalSize(state.width, state.height));
                    }
                }

                // 监听窗口位置和大小变化，自动保存
                const unlistenPosition = await window.listen('tauri://move', async (event: any) => {
                    const pos = event.payload;
                    const size = await window.innerSize();
                    localStorage.setItem('school_calendar_window_state', JSON.stringify({
                        x: pos.x,
                        y: pos.y,
                        width: size.width,
                        height: size.height
                    }));
                });

                const unlistenSize = await window.listen('tauri://resize', async (event: any) => {
                    const size = event.payload;
                    const pos = await window.outerPosition();
                    localStorage.setItem('school_calendar_window_state', JSON.stringify({
                        x: pos.x,
                        y: pos.y,
                        width: size.width,
                        height: size.height
                    }));
                });

                return () => {
                    unlistenPosition();
                    unlistenSize();
                };
            } catch (err) {
                console.error('Failed to restore window state:', err);
            }
        };

        restoreWindowState();
    }, []);

    // 鼠标滚轮缩放
    useEffect(() => {
        const handleWheel = (e: WheelEvent) => {
            if (e.ctrlKey || e.metaKey) {
                e.preventDefault();
                const delta = e.deltaY > 0 ? -0.1 : 0.1;
                setScale(prev => Math.max(0.5, Math.min(2.0, prev + delta)));
            }
        };

        window.addEventListener('wheel', handleWheel, { passive: false });
        return () => window.removeEventListener('wheel', handleWheel);
    }, []);

    useEffect(() => {
        const loaded = loadEvents();
        if (loaded.length === 0) {
            // Add default remarks from the image
            const defaultRemarks = [
                "开学资料准备，学生报到",
                "开学啦！拒绝焦虑",
                "假期两天续命ing",
                "三月，请对我好一点！",
                "埋头苦干！",
                "渐入佳境~",
                "喝杯奶茶舒缓情绪！",
                "再坚持一下",
                "连休三天！",
                "漫漫长路，调整心态！",
                "期中复习~",
                "记得调休！",
                "欧耶！连休五天！",
                "埋头苦干！",
                "继续保持好心情！",
                "坚持就是胜利！",
                "期待端午！",
                "你好，6月！",
                "收拾收拾，准备复习！",
                "放弃助人情结",
                "尊重他人命运",
                "熬过周末~加油！",
                "快乐暑假来啦！！"
            ];
            
            const initialEvents: SchoolEvent[] = defaultRemarks.map((remark, index) => {
                const weekStartDate = addWeeks(semesterStartDate, index);
                const key = formatDateKey(weekStartDate);
                return {
                    id: `wr-default-${index}`,
                    date: key,
                    title: `WEEKLY_REMARK:${remark}`,
                    type: 'remark'
                };
            });
            setEvents(initialEvents);
            saveEvents(initialEvents);
        } else {
            // Simple deduplication by date + title
            const uniqueEvents = loaded.filter((event, index, self) =>
                index === self.findIndex((t) => (
                    t.date === event.date && t.title === event.title
                ))
            );
            setEvents(uniqueEvents);
        }
    }, [semesterStartDate]);

    useEffect(() => {
        const fetchHolidays = async () => {
            const year = semesterStartDate.getFullYear();
            const years = [year, year + 1];
            for (const y of years) {
                const cacheKey = `timor_holidays_${y}`;
                const cached = localStorage.getItem(cacheKey);
                if (cached) {
                    try {
                        const data = JSON.parse(cached);
                        if (Date.now() - data.timestamp < 7 * 24 * 3600 * 1000) {
                            setHolidayData(prev => ({ ...prev, ...data.holidays }));
                            continue;
                        }
                    } catch (e) { localStorage.removeItem(cacheKey); }
                }

                try {
                    const res = await fetch(`https://timor.tech/api/holiday/year/${y}`);
                    const json = await res.json();
                    if (json.code === 0 && json.type) {
                        setHolidayData(prev => ({ ...prev, ...json.type }));
                        localStorage.setItem(cacheKey, JSON.stringify({
                            timestamp: Date.now(),
                            holidays: json.type
                        }));
                    }
                } catch (err) { console.error("Failed to fetch holidays", err); }
            }
        };
        fetchHolidays();
    }, [semesterStartDate]);

    const handleImportClick = (e: React.MouseEvent) => {
        e.stopPropagation(); // Stop drag
        fileInputRef.current?.click();
    };

    const handleClose = (e: React.MouseEvent) => {
        e.stopPropagation(); // Stop drag
        if (onClose) onClose();
        else getCurrentWindow().close();
    };

    const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (evt) => {
            try {
                const bstr = evt.target?.result;
                const wb = XLSX.read(bstr, { type: 'binary' });
                const wsname = wb.SheetNames[0];
                const ws = wb.Sheets[wsname];
                const rows = XLSX.utils.sheet_to_json<any[]>(ws, { header: 1 });

                const importedEvents: SchoolEvent[] = [];
                let dateColIdx = -1;
                let contentColIdx = -1;

                if (rows.length > 1) {
                    // Simple heuristic: col 0 date, col 1 content if no header found
                    dateColIdx = 0; contentColIdx = 1;

                    // Try to find real header
                    for (let i = 0; i < Math.min(rows.length, 10); i++) {
                        const row = rows[i];
                        if (Array.isArray(row)) {
                            row.forEach((cell, idx) => {
                                if (typeof cell === 'string') {
                                    if (cell.includes('日期') || cell.includes('Date')) dateColIdx = idx;
                                    if (cell.includes('内容') || cell.includes('备注')) contentColIdx = idx;
                                }
                            });
                            if (dateColIdx !== -1 && contentColIdx !== -1) {
                                break;
                            }
                        }
                    }

                    rows.forEach(row => {
                        if (!Array.isArray(row)) return;
                        const rawDate = row[dateColIdx];
                        const content = row[contentColIdx];
                        if (rawDate && content) {
                            let finalDateStr = '';
                            if (typeof rawDate === 'number') {
                                finalDateStr = format(new Date(Math.round((rawDate - 25569) * 86400 * 1000)), 'yyyy-MM-dd');
                            } else if (typeof rawDate === 'string') {
                                const m = rawDate.match(/^(\d{1,2})[.-](\d{1,2})/);
                                if (m) {
                                    // Simplified fallback logic for immediate fix
                                    // In real app, we should restore the full year-rollover logic
                                    // For now, assuming current year is fine for simple display fix
                                    const now = new Date();
                                    finalDateStr = `${now.getFullYear()}-${m[1].padStart(2, '0')}-${m[2].padStart(2, '0')}`;
                                }
                            }

                            if (finalDateStr) {
                                importedEvents.push({
                                    id: `import-${Date.now()}-${Math.random()}`,
                                    date: finalDateStr,
                                    title: String(content).trim(),
                                    type: 'activity'
                                });
                            }
                        }
                    });
                }

                if (importedEvents.length > 0) {
                    setEvents(prev => {
                        // Deduplicate on import too
                        const combined = [...prev, ...importedEvents];
                        const unique = combined.filter((e, i, self) =>
                            i === self.findIndex(t => t.date === e.date && t.title === e.title)
                        );
                        saveEvents(unique);
                        return unique;
                    });
                    alert(`成功导入 ${importedEvents.length} 条记录`);
                }

            } catch (err) {
                alert("导入失败");
            }
        };
        reader.readAsBinaryString(file);
        e.target.value = '';
    };

    const calendarRows = useMemo(() => {
        const rows = [];
        let currentWeekStart = semesterStartDate;
        for (let i = 0; i < semesterWeeks; i++) {
            rows.push({
                weekNumber: i + 1,
                startDate: currentWeekStart,
                days: Array.from({ length: 7 }, (_, d) => addDays(currentWeekStart, d)),
                month: getMonth(currentWeekStart) + 1
            });
            currentWeekStart = addWeeks(currentWeekStart, 1);
        }
        return rows;
    }, [semesterStartDate, semesterWeeks]);

    const rowSpans = useMemo(() => {
        const spans: number[] = new Array(calendarRows.length).fill(0);
        let currentMonth = -1;
        let startIndex = 0;
        calendarRows.forEach((row, index) => {
            const m = row.month;
            if (m !== currentMonth) {
                if (index > 0) spans[startIndex] = index - startIndex;
                currentMonth = m;
                startIndex = index;
            }
        });
        spans[startIndex] = calendarRows.length - startIndex;
        return spans;
    }, [calendarRows]);

    const getCellContent = (date: Date) => {
        const key = formatDateKey(date);
        return { events: events.filter(e => e.date === key), holiday: holidayData[key] };
    };

    const handleRemarkChange = (weekIndex: number, text: string) => {
        const key = formatDateKey(calendarRows[weekIndex].startDate);
        const newEvents = events.filter(e => !(e.date === key && e.title.startsWith("WEEKLY_REMARK:")));
        if (text.trim()) newEvents.push({ id: `wr-${key}`, date: key, title: `WEEKLY_REMARK:${text}`, type: 'remark' });
        setEvents(newEvents);
        saveEvents(newEvents);
    };

    const getWeeklyRemark = (weekIndex: number) => {
        const key = formatDateKey(calendarRows[weekIndex].startDate);
        const evt = events.find(e => e.date === key && e.title.startsWith("WEEKLY_REMARK:"));
        return evt ? evt.title.replace("WEEKLY_REMARK:", "") : "";
    };

    const handleDateClick = (date: Date) => {
        setSelectedDate(date);
        setEventInput('');
        setIsDialogOpen(true);
    };

    const handleAddEvent = (date: string, title: string, type: any) => {
        if (!title.trim()) return;
        const newEvent: SchoolEvent = {
            id: `manual-${Date.now()}`,
            date,
            title: title.trim(),
            type
        };
        const updated = [...events, newEvent];
        setEvents(updated);
        saveEvents(updated);
        setEventInput('');
    };

    const handleComplete = () => {
        if (eventInput.trim() && selectedDate) {
            handleAddEvent(formatDateKey(selectedDate), eventInput, 'activity');
        }
        setIsDialogOpen(false);
    };

    const handleDeleteEvent = (id: string) => {
        const updated = events.filter(e => e.id !== id);
        setEvents(updated);
        saveEvents(updated);
    };

    const handleUpdateEvent = (id: string, updates: Partial<SchoolEvent>) => {
        const updated = events.map(e => e.id === id ? { ...e, ...updates } : e);
        setEvents(updated);
        saveEvents(updated);
    };

    const [isPinned, setIsPinned] = useState(() => {
        return localStorage.getItem('school_calendar_pinned') === 'true';
    });

    // Handle Pinning
    useEffect(() => {
        const win = getCurrentWindow();
        win.setAlwaysOnTop(isPinned);
        localStorage.setItem('school_calendar_pinned', isPinned.toString());
    }, [isPinned]);

    return (
        <div
            className="flex flex-col bg-transparent h-full w-full overflow-hidden text-zinc-800 font-sans select-none relative p-0 rounded-none border-none shadow-none"
            onContextMenu={(e) => e.preventDefault()}
        >
            {/* Background Image / Pattern - 模拟图二效果 */}
            <div className="absolute inset-0 z-0 opacity-20 pointer-events-none bg-[#7eb0d5]" style={{ 
                backgroundImage: `radial-gradient(#1e3a8a 1px, transparent 1px)`,
                backgroundSize: '30px 30px'
            }}></div>

            {/* 整个顶部作为拖拽区域 */}
            <div 
                data-tauri-drag-region
                className="pt-6 pb-4 flex flex-col items-center justify-center shrink-0 cursor-move relative z-10"
            >
                <h1 data-tauri-drag-region className="text-6xl font-black text-[#1e3a8a] mb-2 tracking-[0.5em] ml-[0.5em] drop-shadow-sm">校历</h1>
                <div className="text-[14px] font-black text-[#1e3a8a]/90 tracking-widest bg-white/30 px-4 py-1 rounded-full backdrop-blur-sm">
                    {getAcademicYearLabel(semesterStartDate)}（{getSemesterSeason(semesterStartDate)}）教师工作日历
                </div>
            </div>

            {/* 内容容器 - 移除边距和圆角以消除白色角 */}
            <div className="flex-1 overflow-hidden flex flex-col relative z-10">
                <div className="flex-1 overflow-auto custom-scrollbar p-4" style={{ transform: `scale(${scale})`, transformOrigin: 'top center' }}>
                    <table className="w-full border-collapse border-2 border-[#1e3a8a]/30 text-base bg-white/90 shadow-2xl">
                        <thead>
                            <tr className="bg-zinc-100">
                                <th className="border border-zinc-400 w-14 text-center py-3 leading-tight bg-zinc-200">
                                    <div className="text-[11px]">星 期</div>
                                    <div className="text-[11px]">周 月</div>
                                    <div className="text-[11px]">次 日</div>
                                </th>
                                {['一', '二', '三', '四', '五', '六', '日'].map(d => (
                                    <th key={d} className="border border-zinc-400 py-3 text-center font-bold text-base">{d}</th>
                                ))}
                                <th className="border border-zinc-400 py-4 text-center font-bold w-64 bg-zinc-200 text-[14px]">备 注</th>
                            </tr>
                        </thead>
                        <tbody>
                            {calendarRows.map((row, rowIndex) => (
                                <tr key={rowIndex} className="h-[70px]">
                                    <td className="border border-zinc-400 text-center font-bold bg-zinc-50 w-16 text-lg">
                                        {toChineseNum(row.weekNumber)}
                                    </td>
                                    {row.days.map((day, idx) => {
                                        const { events: dayEvents, holiday } = getCellContent(day);
                                        const displayEvents = dayEvents.filter(e => !e.title.startsWith("WEEKLY_REMARK:"));
                                        const isToday = isSameDay(day, new Date());
                                        const isMonthStart = day.getDate() === 1 || (rowIndex === 0 && idx === 0);
                                        // 强化节假日判断：API数据优先，其次判断固定节日
                                        const holidayName = holiday?.name;
                                        const isNationalHoliday = holiday?.type === 2 || 
                                                        (day.getMonth() === 9 && day.getDate() >= 1 && day.getDate() <= 7) || // 国庆
                                                        (day.getMonth() === 0 && day.getDate() === 1); // 元旦
                                        const isWeekend = idx >= 5;
                                        const isCompensatoryWorkday = holiday?.type === 1; // 调休补班

                                        const dateDisplay = isMonthStart 
                                            ? `${day.getMonth() + 1}/${day.getDate()}`
                                            : day.getDate();

                                        // 统一放假颜色逻辑：法定节假日或周末，且不是补班
                                        const cellBg = (isNationalHoliday || isWeekend) && !isCompensatoryWorkday
                                            ? '#fde68a' // 修改为淡黄色（与周六日一致）
                                            : getMonthBg(day.getMonth() + 1);

                                        const specialIconType = specialIcons[formatDateKey(day)];
                                        const SpecialIcon = specialIconType === 'star' ? Star :
                                                          specialIconType === 'gift' ? Gift :
                                                          specialIconType === 'graduation' ? GraduationCap :
                                                          specialIconType === 'heart' ? Heart : null;

                                        return (
                                            <td
                                                key={idx}
                                                className={`
                                                    border border-zinc-400 text-center relative p-0 group
                                                    ${isToday ? 'ring-2 ring-inset ring-blue-500 z-10' : ''}
                                                    hover:bg-zinc-50 transition-colors cursor-pointer
                                                `}
                                                style={{ backgroundColor: cellBg }}
                                                onClick={() => handleDateClick(day)}
                                            >
                                                <div className="flex flex-col items-center justify-center min-h-[70px] py-2">
                                                    <span className={`font-bold ${isMonthStart ? 'text-[14px]' : 'text-xl'} ${isToday ? 'text-blue-600' : ''}`}>
                                                        {dateDisplay}
                                                    </span>
                                                    {(holidayName || isNationalHoliday) && (
                                                        <span className="text-[11px] text-red-600 font-bold scale-90 leading-none mt-1">
                                                            {holidayName || (day.getMonth() === 9 ? '国庆节' : '元旦')}
                                                        </span>
                                                    )}
                                                    {SpecialIcon && (
                                                        <div className="mt-1">
                                                            <SpecialIcon size={16} className={
                                                                specialIconType === 'star' ? 'text-yellow-500' :
                                                                specialIconType === 'gift' ? 'text-pink-500' :
                                                                specialIconType === 'graduation' ? 'text-blue-500' :
                                                                'text-red-500'
                                                            } />
                                                        </div>
                                                    )}
                                                    {displayEvents.length > 0 && (
                                                        <div className="absolute top-2 right-2 w-2.5 h-2.5 rounded-full bg-blue-500 border border-white shadow-sm"></div>
                                                    )}
                                                </div>
                                                
                                                {/* Hover Tooltip - 字体加大 */}
                                                {displayEvents.length > 0 && (
                                                    <div className="absolute invisible group-hover:visible z-[100] bottom-full left-1/2 -translate-x-1/2 mb-3 pointer-events-none animate-in fade-in slide-in-from-bottom-1">
                                                        <div className="bg-zinc-900/95 backdrop-blur-md text-white text-[18px] font-bold py-3 px-5 rounded-2xl shadow-2xl whitespace-nowrap border border-white/20">
                                                            {displayEvents.map((e, i) => (
                                                                <div key={e.id} className="flex items-center gap-3 mb-2 last:mb-0">
                                                                    <div className="w-3 h-3 rounded-full bg-blue-400 shadow-[0_0_8px_rgba(96,165,250,0.6)]"></div>
                                                                    {e.title}
                                                                </div>
                                                            ))}
                                                        </div>
                                                        <div className="w-4 h-4 bg-zinc-900 rotate-45 absolute -bottom-2 left-1/2 -translate-x-1/2 border-r border-b border-white/20"></div>
                                                    </div>
                                                )}
                                            </td>
                                        );
                                    })}
                                    <td className="border border-zinc-400 p-0 relative bg-white">
                                        <textarea
                                            className={`w-full h-full min-h-[70px] bg-transparent resize-none border-none focus:ring-0 text-[14px] p-3 leading-relaxed font-bold ${
                                                /连休|休|假|暑假|寒假/.test(getWeeklyRemark(rowIndex)) ? 'text-red-500' : 'text-[#1e3a8a]'
                                            }`}
                                            value={getWeeklyRemark(rowIndex)}
                                            onChange={(e) => handleRemarkChange(rowIndex, e.target.value)}
                                            onMouseDown={e => e.stopPropagation()}
                                        />
                                    </td>
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </div>
            </div>

            {/* Event Management Dialog */}
            {isDialogOpen && selectedDate && (
                <div className="fixed inset-0 z-[100] flex items-center justify-center dialog-overlay animate-in fade-in duration-300 p-20">
                    <div className="w-[500px] rounded-[2.5rem] bg-white dialog-box overflow-hidden flex flex-col animate-in zoom-in-95 duration-300">
                        {/* Dialog Header */}
                        <div className="px-8 pt-8 pb-4 flex items-center justify-between">
                            <div className="flex items-center gap-4">
                                <div className="w-12 h-12 bg-stone-100 rounded-2xl flex items-center justify-center text-stone-600">
                                    <Tag size={24} />
                                </div>
                                <div>
                                    <h3 className="text-xl font-bold text-stone-800 tracking-tight">{format(selectedDate, 'yyyy年MM月dd日')}</h3>
                                    <p className="text-[10px] font-bold text-stone-400 uppercase tracking-widest mt-0.5">日程管理</p>
                                </div>
                            </div>
                            <button onClick={() => setIsDialogOpen(false)} className="w-10 h-10 rounded-full hover:bg-stone-100 flex items-center justify-center text-stone-400 transition-colors">
                                <X size={20} />
                            </button>
                        </div>

                        {/* Dialog Body */}
                        <div className="flex-1 overflow-auto p-8 pt-2 custom-scrollbar">
                            {/* Existing Events List */}
                            <div className="space-y-3 mb-8">
                                <label className="text-[10px] font-black text-stone-400 uppercase tracking-[0.2em] mb-4 block">当前事项</label>
                                {events.filter(e => e.date === formatDateKey(selectedDate) && !e.title.startsWith("WEEKLY_REMARK:")).length === 0 ? (
                                    <div className="py-10 flex flex-col items-center justify-center border-2 border-dashed border-stone-100 rounded-3xl text-stone-300 italic text-sm">
                                        当日无安排
                                    </div>
                                ) : (
                                    events.filter(e => e.date === formatDateKey(selectedDate) && !e.title.startsWith("WEEKLY_REMARK:")).map(evt => (
                                        <div key={evt.id} className="group flex items-center justify-between p-4 bg-stone-50 rounded-2xl border border-stone-100 hover:bg-white hover:shadow-md transition-all">
                                            <div className="flex items-center gap-3">
                                                <div className={`w-2 h-2 rounded-full ${evt.type === 'holiday' ? 'bg-red-400' : evt.type === 'exam' ? 'bg-purple-400' : 'bg-blue-400'}`}></div>
                                                <span className="font-bold text-stone-700">{evt.title}</span>
                                            </div>
                                            <button onClick={() => handleDeleteEvent(evt.id)} className="opacity-0 group-hover:opacity-100 w-8 h-8 rounded-lg hover:bg-red-50 text-red-400 flex items-center justify-center transition-all">
                                                <Trash2 size={16} />
                                            </button>
                                        </div>
                                    ))
                                )}
                            </div>

                            {/* Add New Event Form */}
                            <div className="bg-stone-50 rounded-[2rem] p-6 border border-stone-100">
                                <label className="text-[10px] font-black text-stone-400 uppercase tracking-[0.2em] mb-4 block">新增事项</label>
                                <div className="space-y-4">
                                    <input
                                        type="text"
                                        placeholder="输入日程内容..."
                                        value={eventInput}
                                        onChange={(e) => setEventInput(e.target.value)}
                                        className="w-full bg-white border border-stone-200 rounded-xl px-4 py-3 text-sm font-bold focus:ring-2 focus:ring-orange-100 outline-none transition-all"
                                        onKeyDown={(e) => {
                                            if (e.key === 'Enter' && selectedDate) {
                                                handleAddEvent(formatDateKey(selectedDate), eventInput, 'activity');
                                            }
                                        }}
                                    />
                                    <div className="flex gap-2 h-10">
                                        {[
                                            { type: 'activity', label: '活动' },
                                            { type: 'holiday', label: '放假' },
                                            { type: 'exam', label: '考试' },
                                            { type: 'meeting', label: '会议' }
                                        ].map(item => (
                                            <button
                                                key={item.type}
                                                onClick={() => {
                                                    if (selectedDate) {
                                                        handleAddEvent(formatDateKey(selectedDate), eventInput, item.type);
                                                    }
                                                }}
                                                className={`flex-1 rounded-xl text-[10px] font-black uppercase tracking-wider transition-all border
                                                    ${item.type === 'activity' ? 'bg-blue-50 text-blue-600 border-blue-100 hover:bg-blue-100' :
                                                        item.type === 'holiday' ? 'bg-red-50 text-red-600 border-red-100 hover:bg-red-100' :
                                                            item.type === 'exam' ? 'bg-purple-50 text-purple-600 border-purple-100 hover:bg-purple-100' :
                                                                'bg-stone-100 text-stone-600 border-stone-200 hover:bg-stone-200'}
                                                `}
                                            >
                                                {item.label}
                                            </button>
                                        ))}
                                    </div>
                                </div>
                            </div>

                            {/* Special Date Icon Selector */}
                            <div className="bg-stone-50 rounded-[2rem] p-6 border border-stone-100 mt-4">
                                <label className="text-[10px] font-black text-stone-400 uppercase tracking-[0.2em] mb-4 block">特殊日期图标</label>
                                <div className="flex gap-3">
                                    {[
                                        { type: 'star', label: '重要', icon: Star, color: 'text-yellow-500' },
                                        { type: 'gift', label: '礼物', icon: Gift, color: 'text-pink-500' },
                                        { type: 'graduation', label: '毕业', icon: GraduationCap, color: 'text-blue-500' },
                                        { type: 'heart', label: '纪念', icon: Heart, color: 'text-red-500' }
                                    ].map(item => {
                                        const IconComponent = item.icon;
                                        const dateKey = selectedDate ? formatDateKey(selectedDate) : '';
                                        const isSelected = specialIcons[dateKey] === item.type;
                                        return (
                                            <button
                                                key={item.type}
                                                onClick={() => {
                                                    if (selectedDate) {
                                                        const dateKey = formatDateKey(selectedDate);
                                                        if (isSelected) {
                                                            // 移除图标
                                                            const newIcons = { ...specialIcons };
                                                            delete newIcons[dateKey];
                                                            setSpecialIcons(newIcons);
                                                        } else {
                                                            // 设置图标
                                                            setSpecialIcons({ ...specialIcons, [dateKey]: item.type });
                                                        }
                                                    }
                                                }}
                                                className={`flex-1 flex flex-col items-center justify-center gap-2 p-4 rounded-xl border-2 transition-all
                                                    ${isSelected 
                                                        ? 'bg-white border-orange-300 shadow-md' 
                                                        : 'bg-white border-stone-200 hover:border-stone-300 hover:shadow-sm'}
                                                `}
                                            >
                                                <IconComponent size={24} className={isSelected ? item.color : 'text-stone-400'} />
                                                <span className={`text-[10px] font-bold ${isSelected ? 'text-stone-700' : 'text-stone-400'}`}>
                                                    {item.label}
                                                </span>
                                            </button>
                                        );
                                    })}
                                    <button
                                        onClick={() => {
                                            if (selectedDate) {
                                                const dateKey = formatDateKey(selectedDate);
                                                const newIcons = { ...specialIcons };
                                                delete newIcons[dateKey];
                                                setSpecialIcons(newIcons);
                                            }
                                        }}
                                        className="flex-1 flex flex-col items-center justify-center gap-2 p-4 rounded-xl border-2 border-stone-200 hover:border-stone-300 hover:shadow-sm bg-white transition-all"
                                        title="清除图标"
                                    >
                                        <X size={24} className="text-stone-400" />
                                        <span className="text-[10px] font-bold text-stone-400">清除</span>
                                    </button>
                                </div>
                            </div>
                        </div>

                        {/* Dialog Footer */}
                        <div className="p-8 pt-2 flex items-center justify-center">
                            <button onClick={handleComplete} className="w-full py-4 bg-stone-800 text-white rounded-2xl font-black text-sm uppercase tracking-[0.2em] shadow-lg shadow-stone-200 hover:bg-stone-900 transition-all active:scale-[0.98]">
                                完成
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default SchoolCalendar;
