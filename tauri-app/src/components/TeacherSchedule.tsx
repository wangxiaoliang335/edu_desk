import { useState, useEffect, useMemo } from 'react';
import { getCurrentTerm } from '../utils/term';
import { invoke } from '@tauri-apps/api/core';
import { useDraggable } from '../hooks/useDraggable';

// API Interfaces
interface ScheduleCell {
    row_index: number;
    col_index: number;
    course_name: string;
    is_highlight: number;
}

interface ScheduleResponse {
    message: string;
    code: number;
    data: {
        schedule: {
            id: number;
            class_id: string;
            term: string;
            days: string[];
            times: string[];
        } | null;
        cells: ScheduleCell[];
    } | ScheduleResponseDataArray; // Handle both single object and array return based on doc
}

type ScheduleResponseDataArray = Array<{
    schedule: any;
    cells: ScheduleCell[];
}>;


// Mock Data Type
interface Course {
    id: string;
    subject: string;
    class: string;
    room?: string;
    color: string;
    highlight?: boolean;
}

const WEEKDAYS = ['周一', '周二', '周三', '周四', '周五'];
// Map API row index to Period ID
// Assuming API row 0 = Period 1, ...
const PERIODS = [
    { id: 1, time: '08:00 - 08:45', name: '第1节', rowIndex: 0 },
    { id: 2, time: '08:55 - 09:40', name: '第2节', rowIndex: 1 },
    { id: 3, time: '10:00 - 10:45', name: '第3节', rowIndex: 2 },
    { id: 4, time: '10:55 - 11:40', name: '第4节', rowIndex: 3 },
    { id: 'noon', time: '12:00 - 14:00', name: '午休', rowIndex: -1 },
    { id: 5, time: '14:00 - 14:45', name: '第5节', rowIndex: 4 },
    { id: 6, time: '14:55 - 15:40', name: '第6节', rowIndex: 5 },
    { id: 7, time: '16:00 - 16:45', name: '第7节', rowIndex: 6 },
    { id: 8, time: '16:55 - 17:40', name: '第8节', rowIndex: 7 },
];

// Softer, warmer colors for subjects
const SUBJECT_COLORS: Record<string, string> = {
    '语文': 'bg-red-50 text-red-700 border-red-100',
    '数学': 'bg-blue-50 text-blue-700 border-blue-100',
    '英语': 'bg-violet-50 text-violet-700 border-violet-100',
    '物理': 'bg-indigo-50 text-indigo-700 border-indigo-100',
    '化学': 'bg-cyan-50 text-cyan-700 border-cyan-100',
    '历史': 'bg-orange-50 text-orange-700 border-orange-100',
    '地理': 'bg-amber-50 text-amber-700 border-amber-100',
    '生物': 'bg-lime-50 text-lime-700 border-lime-100',
    '政治': 'bg-rose-50 text-rose-700 border-rose-100',
    '体育': 'bg-emerald-50 text-emerald-700 border-emerald-100',
    '音乐': 'bg-pink-50 text-pink-700 border-pink-100',
    '美术': 'bg-fuchsia-50 text-fuchsia-700 border-fuchsia-100',
};

const TeacherSchedule = () => {
    const [selectedCell, setSelectedCell] = useState<string | null>(null);
    const [scheduleMap, setScheduleMap] = useState<Record<string, Course[]>>({});
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [currentTerm, setCurrentTerm] = useState(getCurrentTerm()); // Default term
    const [currentWeek, setCurrentWeek] = useState(0); // 0 = 本周
    const [weekInput, setWeekInput] = useState('0');
    const [termStartDate, setTermStartDate] = useState(() => {
        return localStorage.getItem('teacher_schedule_term_start') || '';
    });
    const [isEditing, setIsEditing] = useState(false);
    const [scheduleMeta, setScheduleMeta] = useState<{ days: string[]; times: string[] }>({
        days: WEEKDAYS,
        times: PERIODS.filter((p) => p.id !== 'noon').map((p) => p.time),
    });
    const draggable = useDraggable();

    const teacherId = useMemo(() => {
        return localStorage.getItem('teacher_unique_id') || '';
    }, []);

    useEffect(() => {
        fetchSchedule();
    }, [teacherId, currentTerm, currentWeek]);

    useEffect(() => {
        setWeekInput(String(currentWeek));
    }, [currentWeek]);

    useEffect(() => {
        if (termStartDate) {
            localStorage.setItem('teacher_schedule_term_start', termStartDate);
        }
    }, [termStartDate]);

    const fetchSchedule = async () => {
        setLoading(true);
        setError(null);
        try {
            if (!currentTerm.trim()) {
                setError('学期不能为空');
                return;
            }
            if (!teacherId) {
                setError('未获取到教师ID，无法查询个人课表');
                return;
            }
            // Retrieve token (mocking for now if not in localStorage, usually passed via login)
            // In a real app, use a context or store
            const token = localStorage.getItem('token') || '';

            const responseStr = await invoke<string>('get_teacher_schedule', {
                teacherId,
                term: currentTerm,
                week: currentWeek,
                token
            });

            const response = JSON.parse(responseStr) as ScheduleResponse;

            if (response.code === 200) {
                const newScheduleMap: Record<string, Course[]> = {};

                // Handle Data Structure (might be object or array)
                // Assuming 'message: 查询成功' and single object for specific term
                let cells: ScheduleCell[] = [];
                let scheduleDays: string[] | null = null;
                let scheduleTimes: string[] | null = null;
                if (Array.isArray(response.data)) {
                    // If array, grab first one or flatten? For specific term query it should be object
                    if (response.data.length > 0) {
                        cells = response.data[0].cells;
                        scheduleDays = response.data[0].schedule?.days || null;
                        scheduleTimes = response.data[0].schedule?.times || null;
                    }
                } else if (response.data && response.data.cells) {
                    cells = response.data.cells;
                    scheduleDays = response.data.schedule?.days || null;
                    scheduleTimes = response.data.schedule?.times || null;
                }

                cells.forEach(cell => {
                    // Mapping: 
                    // API col_index 0 = Mon ?? Need to verify. Typically 0-based.
                    // My Grid keys: "dayIndex-periodId" (e.g. "1-1", "5-2")
                    // dayIndex: Mon=1, Tue=2...
                    // periodId: 1..8

                    // Assumption: API col_index 0 -> Mon, row_index 0 -> Period 1
                    const dayKey = cell.col_index + 1;
                    const periodObj = PERIODS.find(p => p.rowIndex === cell.row_index);

                    if (periodObj && periodObj.id !== 'noon') {
                        const cellKey = `${dayKey}-${periodObj.id}`;
                        const course: Course = {
                            id: `srv-${cell.row_index}-${cell.col_index}`,
                            subject: cell.course_name,
                            class: '个人课表',
                            color: SUBJECT_COLORS[cell.course_name] || 'bg-sage-50 text-ink-600 border-sage-100',
                            highlight: cell.is_highlight === 1
                        };

                        if (!newScheduleMap[cellKey]) newScheduleMap[cellKey] = [];
                        newScheduleMap[cellKey].push(course);
                    }
                });

                setScheduleMap(newScheduleMap);
                setScheduleMeta({
                    days: scheduleDays && scheduleDays.length ? scheduleDays : WEEKDAYS,
                    times: scheduleTimes && scheduleTimes.length ? scheduleTimes : PERIODS.filter((p) => p.id !== 'noon').map((p) => p.time),
                });
            } else {
                setError(response.message || '获取失败');
            }

        } catch (err: any) {
            console.error('Fetch error:', err);
            setError(err.toString());
            // Fallback to mock data for demo if API fails entirely
            // setScheduleMap(MOCK_SCHEDULE); 
        } finally {
            setLoading(false);
        }
    };

    const handleCellEdit = (cellKey: string) => {
        if (!isEditing) return;
        const existing = scheduleMap[cellKey]?.[0]?.subject || '';
        const newName = prompt('请输入课程名称（留空表示清空）', existing);
        if (newName === null) return;
        setScheduleMap((prev) => {
            const next = { ...prev };
            if (!newName.trim()) {
                delete next[cellKey];
                return next;
            }
            next[cellKey] = [
                {
                    id: `tmp-${cellKey}-${Date.now()}`,
                    subject: newName.trim(),
                    class: '个人课表',
                    color: SUBJECT_COLORS[newName.trim()] || 'bg-sage-50 text-ink-600 border-sage-100',
                    highlight: true,
                },
            ];
            return next;
        });
    };

    const handleSave = async () => {
        try {
            if (!currentTerm.trim()) {
                alert('学期不能为空');
                return;
            }
            if (!teacherId) {
                alert('未获取到教师ID，无法保存个人课表');
                return;
            }
            const token = localStorage.getItem('token') || '';
            const cellsPayload: ScheduleCell[] = [];

            Object.entries(scheduleMap).forEach(([key, courses]) => {
                const [dayIndexStr, periodIdStr] = key.split('-');
                const dayIndex = parseInt(dayIndexStr, 10) - 1;
                const periodId = parseInt(periodIdStr, 10);
                const periodObj = PERIODS.find((p) => p.id === periodId);
                if (!periodObj || !courses || courses.length === 0) return;
                const course = courses[0];
                cellsPayload.push({
                    row_index: periodObj.rowIndex,
                    col_index: dayIndex,
                    course_name: course.subject,
                    is_highlight: course.highlight ? 1 : 0,
                });
            });

            const resp = await invoke<string>('save_teacher_schedule', {
                teacherId,
                term: currentTerm,
                week: currentWeek,
                days: scheduleMeta.days,
                times: scheduleMeta.times,
                cells: cellsPayload,
                token,
            });
            const parsed = JSON.parse(resp);
            if (parsed?.code === 200) {
                alert('保存成功');
                setIsEditing(false);
            } else {
                alert(`保存失败：${parsed?.message || resp}`);
            }
        } catch (e) {
            console.error('Save schedule failed:', e);
            alert('保存失败，请稍后再试');
        }
    };

    const weekLabel = currentWeek === 0 ? '本周' : currentWeek > 0 ? `第${currentWeek}周` : `第${currentWeek}周`;

    const getWeekDateRange = (week: number) => {
        if (!termStartDate || week <= 0) return '';
        const start = new Date(termStartDate);
        if (Number.isNaN(start.getTime())) return '';
        const startMs = start.getTime() + (week - 1) * 7 * 24 * 60 * 60 * 1000;
        const endMs = startMs + 6 * 24 * 60 * 60 * 1000;
        const format = (d: Date) => {
            const y = d.getFullYear();
            const m = String(d.getMonth() + 1).padStart(2, '0');
            const day = String(d.getDate()).padStart(2, '0');
            return `${y}-${m}-${day}`;
        };
        return `${format(new Date(startMs))} ~ ${format(new Date(endMs))}`;
    };
    const weekRangeLabel = getWeekDateRange(currentWeek);
    const weekInputNumber = Number.parseInt(weekInput, 10);
    const weekInputRangeLabel = Number.isNaN(weekInputNumber) ? '' : getWeekDateRange(weekInputNumber);

    return (
        <div
            className="flex flex-col bg-paper rounded-[2rem] border border-sage-200 shadow-xl resize both overflow-auto min-w-[900px] min-h-[600px] font-sans text-ink-600"
            style={draggable.style}
        >
            {/* Header Actions */}
            <div className="flex flex-col gap-3 mb-4 px-6 pt-5 cursor-move select-none bg-white/50 border-b border-sage-50 pb-4" onMouseDown={draggable.handleMouseDown}>
                <div className="flex items-center justify-between">
                    <div className="flex items-center gap-3">
                        <span className="text-2xl">👨‍🏫</span>
                        <h2 className="text-xl font-bold text-ink-800 tracking-tight">教师个人课表</h2>
                        <div className="text-xs px-3 py-1 rounded-full bg-sage-50 text-sage-600 border border-sage-100 font-bold">{weekLabel}</div>
                        {weekRangeLabel && (
                            <div className="text-xs px-3 py-1 rounded-full bg-paper text-ink-400 border border-sage-100 font-medium">
                                {weekRangeLabel}
                            </div>
                        )}
                        {isEditing && (
                            <div className="text-xs px-3 py-1 rounded-full bg-clay-50 text-clay-600 border border-clay-100 font-bold animate-pulse">
                                临时调课模式
                            </div>
                        )}
                    </div>
                    <div className="flex items-center gap-2">
                        {loading && <span className="text-sm text-sage-500 flex items-center animate-pulse">加载中...</span>}
                        {error && <span className="text-sm text-clay-500 flex items-center">错误: {error}</span>}
                    </div>
                </div>

                <div className="flex items-start justify-between w-full gap-4">
                    <div className="flex flex-wrap items-center gap-2">
                        <label className="text-xs text-ink-400 font-medium">学期</label>
                        <input
                            type="text"
                            value={currentTerm}
                            onChange={(e) => setCurrentTerm(e.target.value)}
                            className="text-xs border border-sage-200 rounded-lg px-3 py-1.5 w-28 bg-white focus:border-sage-400 outline-none transition-colors font-medium text-ink-700"
                            placeholder="2025-2026-1"
                        />
                        <label className="text-xs text-ink-400 ml-2 font-medium">第1周起始日</label>
                        <input
                            type="date"
                            value={termStartDate}
                            onChange={(e) => setTermStartDate(e.target.value)}
                            className="text-xs border border-sage-200 rounded-lg px-2 py-1.5 bg-white text-ink-700 focus:border-sage-400 outline-none"
                        />
                        <label className="text-xs text-ink-400 ml-2 font-medium">教师ID</label>
                        <input
                            type="text"
                            value={teacherId || '未绑定'}
                            disabled
                            className="text-xs border border-sage-100 rounded-lg px-3 py-1.5 w-24 bg-sage-50/50 text-ink-300"
                            placeholder="Teacher ID"
                        />
                        <button onClick={fetchSchedule} className="text-xs bg-sage-600 text-white px-4 py-1.5 rounded-lg hover:bg-sage-700 transition-colors font-bold shadow-sm shadow-sage-200">
                            查询
                        </button>
                    </div>

                    <div className="flex items-start gap-2">
                        <button
                            onClick={() => setIsEditing((v) => !v)}
                            className={`text-xs px-4 py-1.5 rounded-lg border font-medium transition-all ${isEditing ? 'bg-clay-500 text-white border-clay-500 shadow-md shadow-clay-200' : 'bg-white text-ink-500 border-sage-200 hover:bg-sage-50'}`}
                        >
                            临时调课
                        </button>
                        {isEditing && (
                            <button onClick={handleSave} className="text-xs bg-sage-600 text-white px-4 py-1.5 rounded-lg hover:bg-sage-700 font-bold shadow-sm">
                                保存
                            </button>
                        )}

                        <span className="mx-2 text-sage-200">|</span>

                        <div className="flex bg-white rounded-lg border border-sage-200 p-0.5">
                            <button
                                onClick={() => setCurrentWeek((w) => w - 1)}
                                className="px-3 py-1 text-xs rounded-md hover:bg-sage-50 text-ink-500 transition-colors"
                            >
                                上周
                            </button>
                            <button
                                onClick={() => setCurrentWeek(0)}
                                className="px-3 py-1 text-xs bg-sage-100 text-sage-700 rounded-md font-bold"
                            >
                                本周
                            </button>
                            <button
                                onClick={() => setCurrentWeek((w) => w + 1)}
                                className="px-3 py-1 text-xs rounded-md hover:bg-sage-50 text-ink-500 transition-colors"
                            >
                                下周
                            </button>
                        </div>

                        <div className="flex items-center gap-1 ml-2">
                            <input
                                type="number"
                                min={0}
                                value={weekInput}
                                onChange={(e) => setWeekInput(e.target.value)}
                                className="w-16 text-xs border border-sage-200 rounded-lg px-2 py-1.5 bg-white text-ink-700 focus:border-sage-400 outline-none text-center"
                                placeholder="周次"
                            />
                            <button
                                onClick={() => {
                                    const n = parseInt(weekInput, 10);
                                    if (!Number.isNaN(n)) setCurrentWeek(n);
                                }}
                                className="text-xs px-3 py-1.5 rounded-lg border border-sage-200 bg-white text-ink-500 hover:bg-sage-50 font-medium"
                            >
                                跳转
                            </button>
                        </div>
                    </div>
                </div>
            </div>

            {/* Schedule Grid */}
            <div className="flex-1 overflow-auto px-6 pb-6">
                <div className="rounded-2xl border border-sage-100 bg-white shadow-sm overflow-hidden">
                    <div className="grid grid-cols-[80px_1fr_1fr_1fr_1fr_1fr] divide-x divide-sage-50 h-full min-w-[800px]">
                        {/* Time Column */}
                        <div className="bg-sage-50/30 flex flex-col divide-y divide-sage-50 text-xs text-ink-400 font-medium tracking-wide">
                            <div className="h-10 flex items-center justify-center bg-sage-50/50 text-sage-600 font-bold">时间</div>
                            {PERIODS.map((period) => (
                                <div key={period.id} className={`flex flex-col items-center justify-center p-2 ${period.id === 'noon' ? 'h-14 bg-sage-50/50' : 'h-28'}`}>
                                    <span className="font-bold text-ink-500">{period.name}</span>
                                    <span className="scale-90 opacity-70 mt-1 font-mono">{period.time}</span>
                                </div>
                            ))}
                        </div>

                        {/* Weekday Columns */}
                        {WEEKDAYS.map((day, dayIndex) => (
                            <div key={day} className="flex flex-col divide-y divide-sage-50 hover:bg-sage-50/10 transition-colors">
                                {/* Header */}
                                <div className="h-10 flex items-center justify-center bg-sage-50/30 font-bold text-ink-600 text-sm">
                                    {day}
                                </div>

                                {/* Cells */}
                                {PERIODS.map((period) => {
                                    if (period.id === 'noon') {
                                        return <div key="noon" className="h-14 bg-sage-50/20 flex items-center justify-center text-xs text-sage-300 tracking-widest">午休</div>;
                                    }

                                    const cellKey = `${dayIndex + 1}-${period.id}`;
                                    const courses = scheduleMap[cellKey];
                                    const isSelected = selectedCell === cellKey;

                                    return (
                                        <div
                                            key={period.id}
                                            onClick={() => setSelectedCell(cellKey)}
                                            onDoubleClick={() => handleCellEdit(cellKey)}
                                            className={`h-28 p-1.5 transition-all relative group ${isSelected ? 'bg-sage-50/50 ring-2 ring-inset ring-sage-200' : ''} ${isEditing ? 'cursor-pointer hover:bg-sage-50' : ''}`}
                                        >
                                            {courses ? (
                                                courses.map(course => (
                                                    <div key={course.id} className={`h-full w-full rounded-xl p-3 border ${course.color} ${course.highlight ? 'ring-2 ring-clay-400 ring-offset-1' : ''} shadow-sm cursor-pointer hover:shadow-md hover:-translate-y-0.5 transition-all flex flex-col justify-center gap-1.5`}>
                                                        <div className="font-bold text-base tracking-tight">{course.subject}</div>
                                                        <div className="text-xs opacity-75 font-medium bg-white/30 w-fit px-1.5 py-0.5 rounded-md">{course.class}</div>
                                                    </div>
                                                ))
                                            ) : (
                                                <div className="h-full w-full rounded-xl border border-transparent group-hover:border-dashed group-hover:border-sage-200 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-all">
                                                    <span className="text-sage-300 text-2xl font-light hover:scale-110 transition-transform cursor-pointer">+</span>
                                                </div>
                                            )}
                                        </div>
                                    );
                                })}
                            </div>
                        ))}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default TeacherSchedule;
