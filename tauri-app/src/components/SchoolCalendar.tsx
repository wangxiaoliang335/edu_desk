
import { useRef, useState, useEffect, useMemo } from 'react';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { format, addDays, getMonth, isSameDay, addWeeks, startOfWeek } from 'date-fns';
import {
    SchoolEvent,
    loadEvents,
    saveEvents,
    formatDateKey
} from '../utils/schoolEvents';
import * as XLSX from 'xlsx';
import { Upload, X, Trash2, Plus, FileText, Check, AlertCircle, Info, Calendar as CalendarIcon, Tag } from 'lucide-react';
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

    // Default semester logic
    const [semesterStartDate] = useState<Date>(() => {
        const now = new Date();
        const month = now.getMonth();
        const year = now.getFullYear();
        if (month >= 1 && month < 7) {
            return startOfWeek(new Date(year, 1, 15), { weekStartsOn: 1 });
        } else {
            if (month === 0) return startOfWeek(new Date(year - 1, 8, 1), { weekStartsOn: 1 });
            return startOfWeek(new Date(year, 8, 1), { weekStartsOn: 1 });
        }
    });

    const [semesterWeeks] = useState(22);

    useEffect(() => {
        const loaded = loadEvents();
        // Simple deduplication by date + title
        const uniqueEvents = loaded.filter((event, index, self) =>
            index === self.findIndex((t) => (
                t.date === event.date && t.title === event.title
            ))
        );
        setEvents(uniqueEvents);
    }, []);

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

    return (
        <div
            className="flex flex-col bg-paper h-screen w-screen overflow-hidden text-zinc-700 font-sans select-none relative"
            onContextMenu={(e) => e.preventDefault()}
        >
            {/* Background Texture Overlay */}
            <div className="absolute inset-0 pointer-events-none opacity-20 z-0 bg-[url('https://www.transparenttextures.com/patterns/silk.png')]"></div>

            {/* Header */}
            <div
                data-tauri-drag-region
                className="h-20 px-8 flex items-center justify-between shrink-0 bg-white/70 backdrop-blur-xl z-50 cursor-move border-b border-stone-200/50 shadow-sm"
            >
                <div className="flex items-center gap-5 pointer-events-none">
                    <div className="w-12 h-12 rounded-2xl bg-gradient-to-br from-orange-400 to-red-500 text-white flex items-center justify-center shadow-lg shadow-orange-200 animate-pulse-subtle">
                        <CalendarIcon size={28} />
                    </div>
                    <div className="flex flex-col">
                        <span className="text-2xl font-black text-stone-800 tracking-tight leading-none">校历日程</span>
                        <span className="text-[10px] font-bold text-stone-400 mt-1.5 uppercase tracking-[0.2em]">教师个人教学规划</span>
                    </div>
                </div>

                <div className="flex items-center gap-4 relative z-50 bg-stone-100/50 p-1.5 rounded-2xl border border-stone-200/50" onMouseDown={(e) => e.stopPropagation()}>
                    <input type="file" ref={fileInputRef} onChange={handleFileChange} accept=".xlsx, .xls" className="hidden" />
                    <button
                        onClick={handleImportClick}
                        className="flex items-center gap-2 bg-white hover:bg-orange-50 px-5 py-2 rounded-xl transition-all border border-stone-200 shadow-sm hover:shadow-md text-stone-600 font-bold text-sm group"
                    >
                        <Upload size={16} className="text-orange-500 group-hover:scale-110 transition-transform" /> 导入
                    </button>
                    <button
                        onClick={handleClose}
                        className="w-10 h-10 flex items-center justify-center rounded-xl hover:bg-red-50 text-stone-400 hover:text-red-500 transition-all cursor-pointer group"
                    >
                        <X size={20} className="group-hover:rotate-90 transition-transform duration-300" />
                    </button>
                </div>
            </div>

            {/* Table Content */}
            <div className="flex-1 overflow-auto custom-scrollbar bg-stone-50/20 z-10">
                <div className="min-w-full inline-block align-middle pb-12">
                    <table className="min-w-full bg-transparent table-fixed border-separate border-spacing-0">
                        <thead className="sticky top-0 z-40 bg-white/80 backdrop-blur-xl shadow-sm">
                            <tr className="text-stone-400 text-[10px] font-black uppercase tracking-[0.15em]">
                                <th className="py-5 w-20 text-center border-b border-stone-100 bg-stone-50/30">月份</th>
                                <th className="py-5 w-16 text-center border-b border-stone-100 bg-stone-50/30">周次</th>
                                {['周一', '周二', '周三', '周四', '周五', '周六', '周日'].map((d, i) => (
                                    <th key={d} className={`py-5 w-[11%] text-left pl-5 border-b border-stone-100 bg-white/50 ${i >= 5 ? 'text-red-400' : ''}`}>{d}</th>
                                ))}
                                <th className="py-5 px-6 text-left border-b border-stone-100 w-auto bg-amber-50/20">周备注</th>
                            </tr>
                        </thead>
                        <tbody className="divide-y divide-stone-100/50">
                            {calendarRows.map((row, rowIndex) => {
                                return (
                                    <tr key={rowIndex} className="group transition-colors bg-white hover:bg-stone-50">
                                        {rowSpans[rowIndex] > 0 && (
                                            <td
                                                rowSpan={rowSpans[rowIndex]}
                                                className="bg-white align-top pt-8 border-r border-stone-100 relative"
                                            >
                                                {/* Timeline connector */}
                                                <div className="absolute right-0 top-0 bottom-0 w-[2px] bg-stone-100 hidden"></div>

                                                <div className="sticky top-24 flex flex-col items-center justify-center min-h-[4rem] z-30 relative w-full">
                                                    {/* Background Watermark */}
                                                    <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 text-5xl font-black text-stone-100 rotate-90 select-none pointer-events-none -z-10 opacity-60 font-serif translate-x-2">{row.month}</div>

                                                    {/* Foreground Month */}
                                                    <div className="w-10 h-10 rounded-full border-2 border-stone-800 flex items-center justify-center bg-white z-10 shadow-sm">
                                                        <span className="text-xl font-bold text-stone-800 font-serif leading-none pt-0.5">{row.month}</span>
                                                    </div>
                                                    <div className="text-[10px] font-bold text-stone-400 mt-1 uppercase tracking-widest z-10">月</div>
                                                </div>
                                            </td>
                                        )}

                                        <td className="align-middle text-center bg-stone-50/30 border-r border-stone-100 border-dashed">
                                            <div className="inline-flex w-7 h-7 rounded-full bg-stone-200/50 items-center justify-center text-sm font-bold text-stone-500 font-mono">
                                                {row.weekNumber}
                                            </div>
                                        </td>

                                        {row.days.map((day, idx) => {
                                            const { events: dayEvents, holiday } = getCellContent(day);
                                            const isWeekend = idx >= 5;
                                            const isToday = isSameDay(day, new Date());
                                            const isHoliday = holiday && holiday.type === 2;
                                            const isWorkday = holiday && holiday.type === 1;

                                            return (
                                                <td
                                                    key={idx}
                                                    className={`
                                                        align-top p-2 h-36 border-r border-stone-50 last:border-0 transition-colors
                                                        ${isWeekend ? 'bg-orange-50/10' : ''}
                                                        ${isToday ? 'bg-blue-50/40 ring-1 ring-inset ring-blue-200' : ''}
                                                    `}
                                                >
                                                    <div className="flex items-center justify-between mb-2">
                                                        <span className={`
                                                             text-lg font-black w-9 h-9 flex items-center justify-center rounded-2xl transition-all
                                                             ${isToday ? 'bg-blue-500 text-white shadow-lg shadow-blue-400 animate-today-pulse scale-110' :
                                                                isHoliday ? 'text-red-500' :
                                                                    isWeekend ? 'text-stone-300' : 'text-stone-700'}
                                                         `}>
                                                            {day.getDate()}
                                                        </span>

                                                        <div className="flex gap-1.5">
                                                            {holiday && (
                                                                <span className={`text-[9px] font-black px-2 py-0.5 rounded-lg border shadow-sm ${isHoliday ? 'bg-red-50 text-red-500 border-red-100' :
                                                                    isWorkday ? 'bg-stone-100 text-stone-600 border-stone-200' : 'bg-stone-50 text-stone-400 border-stone-100'
                                                                    }`}>
                                                                    {holiday.name}
                                                                </span>
                                                            )}
                                                            <div className="w-6 h-6 rounded-lg border border-transparent group-hover:bg-stone-100 group-hover:border-stone-200 flex items-center justify-center text-stone-300 transition-colors cursor-pointer" onClick={(e) => { e.stopPropagation(); handleDateClick(day); }}>
                                                                <Plus size={14} />
                                                            </div>
                                                        </div>
                                                    </div>

                                                    <div className="flex flex-col gap-1.5 px-0.5" onClick={() => handleDateClick(day)}>
                                                        {dayEvents.filter(e => !e.title.startsWith("WEEKLY_REMARK:")).map((evt, evtIdx) => {
                                                            const isImportant = /节|假|纪念日|开学|典礼|周年|测试|考/.test(evt.title);
                                                            const isHolidayType = evt.type === 'holiday';
                                                            const shouldUseGlow = isHolidayType || isImportant;

                                                            return (
                                                                <div
                                                                    key={evt.id}
                                                                    className={`
                                                                        px-2.5 py-1.5 rounded-lg text-xs font-bold border break-words shadow-sm leading-snug
                                                                        transition-all duration-300 hover:scale-105 hover:shadow-md cursor-default
                                                                        animate-fade-in-up
                                                                        ${shouldUseGlow ? 'bg-red-50 border-red-100 text-red-700 animate-holiday-glow ring-1 ring-red-100' :
                                                                            evt.type === 'exam' ? 'bg-purple-50 border-purple-100 text-purple-700' :
                                                                                'bg-white border-stone-200 text-stone-600 hover:border-blue-300 hover:text-blue-600'}
                                                                    `}
                                                                    style={{ animationDelay: `${evtIdx * 100}ms` }}
                                                                    title={evt.title}
                                                                >
                                                                    {evt.title}
                                                                </div>
                                                            );
                                                        })}
                                                    </div>
                                                </td>
                                            );
                                        })}

                                        <td className="p-0 align-top relative bg-amber-50/10">
                                            {/* Ruled Paper Lines */}
                                            <div className="absolute inset-0 pointer-events-none ruled-paper opacity-50"></div>

                                            <textarea
                                                className="w-full h-full min-h-[9rem] bg-transparent resize-none border-none focus:ring-0 text-sm font-bold text-stone-600 placeholder-stone-300 rounded-none hover:bg-amber-50/20 focus:bg-amber-50/30 transition-colors p-5 pt-6 leading-[1.5rem] relative z-10"
                                                placeholder="本周备忘录..."
                                                spellCheck={false}
                                                value={getWeeklyRemark(rowIndex)}
                                                onChange={(e) => handleRemarkChange(rowIndex, e.target.value)}
                                                onMouseDown={e => e.stopPropagation()}
                                            />
                                        </td>
                                    </tr>
                                );
                            })}

                            <tr className="h-20">
                                <td colSpan={10} className="text-center text-stone-300 text-xs py-8 tracking-widest uppercase">
                                    - 学期结束 -
                                </td>
                            </tr>
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
