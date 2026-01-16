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

const SUBJECT_COLORS: Record<string, string> = {
    '语文': 'bg-blue-100 text-blue-700 border-blue-200',
    '数学': 'bg-emerald-100 text-emerald-700 border-emerald-200',
    '英语': 'bg-purple-100 text-purple-700 border-purple-200',
    '物理': 'bg-indigo-100 text-indigo-700 border-indigo-200',
    '化学': 'bg-cyan-100 text-cyan-700 border-cyan-200',
    '历史': 'bg-orange-100 text-orange-700 border-orange-200',
    '地理': 'bg-amber-100 text-amber-700 border-amber-200',
    '生物': 'bg-lime-100 text-lime-700 border-lime-200',
    '政治': 'bg-rose-100 text-rose-700 border-rose-200',
    '体育': 'bg-green-100 text-green-700 border-green-200',
    '音乐': 'bg-pink-100 text-pink-700 border-pink-200',
    '美术': 'bg-fuchsia-100 text-fuchsia-700 border-fuchsia-200',
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
                            color: SUBJECT_COLORS[cell.course_name] || 'bg-gray-100 text-gray-700 border-gray-200',
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
                    color: SUBJECT_COLORS[newName.trim()] || 'bg-gray-100 text-gray-700 border-gray-200',
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
            className="flex flex-col bg-white/60 backdrop-blur-sm rounded-xl border border-gray-200 shadow-sm resize both overflow-auto min-w-[900px] min-h-[600px]"
            style={draggable.style}
        >
            {/* Header Actions */}
            <div className="flex flex-col gap-2 mb-3 px-3 pt-2 cursor-move select-none" onMouseDown={draggable.handleMouseDown}>
                <div className="flex items-center justify-between">
                    <div className="flex items-center gap-3">
                        <h2 className="text-lg font-bold text-gray-800">教师个人课表</h2>
                        <div className="text-xs px-2 py-1 rounded-full bg-blue-50 text-blue-600 border border-blue-100">{weekLabel}</div>
                        {weekRangeLabel && (
                            <div className="text-xs px-2 py-1 rounded-full bg-gray-50 text-gray-600 border border-gray-200">
                                {weekRangeLabel}
                            </div>
                        )}
                        {isEditing && (
                            <div className="text-xs px-2 py-1 rounded-full bg-orange-50 text-orange-600 border border-orange-100">
                                临时调课模式
                            </div>
                        )}
                    </div>
                    <div className="flex items-center gap-2">
                        {loading && <span className="text-sm text-gray-500 flex items-center animate-pulse">加载中...</span>}
                        {error && <span className="text-sm text-red-500 flex items-center">错误: {error}</span>}
                    </div>
                </div>

                <div className="flex items-start justify-between w-full gap-4">
                    <div className="flex flex-wrap items-center gap-2">
                        <label className="text-xs text-gray-500">学期</label>
                        <input
                            type="text"
                            value={currentTerm}
                            onChange={(e) => setCurrentTerm(e.target.value)}
                            className="text-xs border rounded px-2 py-1 w-28 bg-white font-medium text-gray-700"
                            placeholder="2025-2026-1"
                        />
                        <label className="text-xs text-gray-500 ml-1">第1周起始日</label>
                        <input
                            type="date"
                            value={termStartDate}
                            onChange={(e) => setTermStartDate(e.target.value)}
                            className="text-xs border rounded px-2 py-1 bg-white text-gray-700"
                        />
                        <label className="text-xs text-gray-500 ml-1">教师ID</label>
                        <input
                            type="text"
                            value={teacherId || '未绑定'}
                            disabled
                            className="text-xs border rounded px-2 py-1 w-24 bg-gray-100 text-gray-400"
                            placeholder="Teacher ID"
                        />
                        <button onClick={fetchSchedule} className="text-xs bg-blue-600 text-white px-3 py-1 rounded hover:bg-blue-700">
                            查询
                        </button>
                    </div>

                    <div className="flex items-start gap-2">
                        <button
                            onClick={() => setIsEditing((v) => !v)}
                            className={`text-xs px-3 py-1 rounded border ${isEditing ? 'bg-orange-500 text-white border-orange-500' : 'bg-white text-gray-600 border-gray-200 hover:bg-gray-50'}`}
                        >
                            临时调课
                        </button>
                        {isEditing && (
                            <button onClick={handleSave} className="text-xs bg-green-600 text-white px-3 py-1 rounded hover:bg-green-700">
                                保存
                            </button>
                        )}

                        <span className="mx-1 text-gray-300">|</span>

                        <button
                            onClick={() => setCurrentWeek((w) => w - 1)}
                            className="px-3 py-1.5 text-xs bg-white border border-gray-200 rounded-lg hover:bg-gray-50 text-gray-600 shadow-sm transition-all"
                        >
                            上周
                        </button>
                        <button
                            onClick={() => setCurrentWeek(0)}
                            className="px-3 py-1.5 text-xs bg-blue-600 text-white rounded-lg hover:bg-blue-700 shadow-sm shadow-blue-500/20 transition-all"
                        >
                            本周
                        </button>
                        <button
                            onClick={() => setCurrentWeek((w) => w + 1)}
                            className="px-3 py-1.5 text-xs bg-white border border-gray-200 rounded-lg hover:bg-gray-50 text-gray-600 shadow-sm transition-all"
                        >
                            下周
                        </button>
                        <div className="flex flex-col items-start gap-1 ml-1">
                            <div className="flex items-center gap-1">
                                <input
                                    type="number"
                                    min={0}
                                    value={weekInput}
                                    onChange={(e) => setWeekInput(e.target.value)}
                                    className="w-16 text-xs border rounded px-2 py-1 bg-white text-gray-700"
                                    placeholder="周次"
                                />
                                <button
                                    onClick={() => {
                                        const n = parseInt(weekInput, 10);
                                        if (!Number.isNaN(n)) setCurrentWeek(n);
                                    }}
                                    className="text-xs px-2 py-1 rounded border bg-white text-gray-600 border-gray-200 hover:bg-gray-50"
                                >
                                    跳转
                                </button>
                            </div>
                            {weekInputRangeLabel ? (
                                <div className="text-[11px] text-gray-500 mt-2">
                                    {weekInputRangeLabel}
                                </div>
                            ) : (
                                <div className="text-[11px] text-gray-400 mt-2">
                                    请输入周次查看日期
                                </div>
                            )}
                        </div>
                    </div>
                </div>

                <div className="text-xs text-gray-500">
                    说明：双击格子可修改课程；空值表示清空。临时调课将自动高亮。
                </div>
            </div>

            {/* Schedule Grid */}
            <div className="flex-1 overflow-auto rounded-xl border border-gray-200 bg-white shadow-sm">
                <div className="grid grid-cols-[80px_1fr_1fr_1fr_1fr_1fr] divide-x divide-gray-100 h-full min-w-[800px]">
                    {/* Time Column */}
                    <div className="bg-gray-50/50 flex flex-col divide-y divide-gray-100 text-xs text-gray-500 font-medium">
                        <div className="h-10 flex items-center justify-center bg-gray-100/50">时间</div>
                        {PERIODS.map((period) => (
                            <div key={period.id} className={`flex flex-col items-center justify-center p-2 ${period.id === 'noon' ? 'h-12 bg-gray-100/30' : 'h-24'}`}>
                                <span>{period.name}</span>
                                <span className="scale-90 text-gray-400 mt-1">{period.time}</span>
                            </div>
                        ))}
                    </div>

                    {/* Weekday Columns */}
                    {WEEKDAYS.map((day, dayIndex) => (
                        <div key={day} className="flex flex-col divide-y divide-gray-100">
                            {/* Header */}
                            <div className="h-10 flex items-center justify-center bg-gray-100/50 font-semibold text-gray-700 text-sm">
                                {day}
                            </div>

                            {/* Cells */}
                            {PERIODS.map((period) => {
                                if (period.id === 'noon') {
                                    return <div key="noon" className="h-12 bg-gray-50/20"></div>;
                                }

                                const cellKey = `${dayIndex + 1}-${period.id}`;
                                const courses = scheduleMap[cellKey];
                                const isSelected = selectedCell === cellKey;

                                return (
                                    <div
                                        key={period.id}
                                        onClick={() => setSelectedCell(cellKey)}
                                        onDoubleClick={() => handleCellEdit(cellKey)}
                                        className={`h-24 p-1 transition-colors relative group ${isSelected ? 'bg-blue-50/30' : 'hover:bg-gray-50/30'} ${isEditing ? 'cursor-pointer' : ''}`}
                                    >
                                        {courses ? (
                                            courses.map(course => (
                                                <div key={course.id} className={`h-full w-full rounded-lg p-2 border ${course.color} ${course.highlight ? 'ring-2 ring-orange-400 ring-offset-1' : ''} shadow-sm cursor-pointer hover:shadow-md transition-all flex flex-col justify-center gap-1`}>
                                                    <div className="font-bold text-sm">{course.subject}</div>
                                                    <div className="text-xs opacity-90">{course.class}</div>
                                                </div>
                                            ))
                                        ) : (
                                            <div className="h-full w-full rounded-lg border border-transparent group-hover:border-dashed group-hover:border-gray-300 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-all">
                                                <span className="text-gray-300 text-2xl font-light">+</span>
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
    );
};

export default TeacherSchedule;
