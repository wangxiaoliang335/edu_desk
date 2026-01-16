import { useState, useEffect, useMemo } from 'react';
import { getCurrentTerm } from '../../utils/term';
import { invoke } from '@tauri-apps/api/core';
import { X, Calendar, RefreshCw, ChevronDown } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface SchoolCourseScheduleModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo: any;
}

interface ClassSchedule {
    class_id: string;
    class_name: string;
    grade: string;
    school_stage: string;
    schedules?: {
        schedule?: {
            days: string[];
            times: string[];
        };
        cells?: Array<{
            row_index: number;
            col_index: number;
            course_name: string;
            is_highlight?: number;
        }>;
    };
}

// Removed static CURRENT_TERM


const SchoolCourseScheduleModal = ({ isOpen, onClose, userInfo }: SchoolCourseScheduleModalProps) => {
    const { style, handleMouseDown } = useDraggable();
    const [loading, setLoading] = useState(false);
    const [scheduleData, setScheduleData] = useState<ClassSchedule[]>([]);
    const [selectedGrade, setSelectedGrade] = useState<string>('');
    const [fontScale, setFontScale] = useState(1);

    // Derived state for grades
    const grades = useMemo(() => {
        const gradeSet = new Set<string>();
        scheduleData.forEach(s => {
            // Use grade or school_stage as fallback
            const g = s.grade || s.school_stage;
            if (g) gradeSet.add(g);
        });
        return Array.from(gradeSet).sort();
    }, [scheduleData]);

    // Derived state for current view
    const currentViewData = useMemo(() => {
        if (!selectedGrade) return null;
        const classes = scheduleData.filter(s => (s.grade || s.school_stage) === selectedGrade);

        if (classes.length === 0) return null;

        // Assume all classes in the same grade have the same schedule structure (days/times)
        // Taking the first one as reference for structure
        const reference = classes.find(c => c.schedules?.schedule?.days?.length) || classes[0];
        const days = reference.schedules?.schedule?.days || ["周一", "周二", "周三", "周四", "周五"];
        const times = reference.schedules?.schedule?.times || [];

        return {
            classes,
            days,
            times
        };
    }, [scheduleData, selectedGrade]);

    useEffect(() => {
        if (isOpen) {
            fetchSchoolSchedule();
        }
    }, [isOpen]);

    useEffect(() => {
        if (grades.length > 0 && !selectedGrade) {
            setSelectedGrade(grades[0]);
        }
    }, [grades]);

    const fetchSchoolSchedule = async () => {
        setLoading(true);
        try {
            // Use props userInfo instead of localStorage for better reliability
            let token = localStorage.getItem('token') || "";
            let schoolId = userInfo?.school_id || "";

            // Fallback to localStorage if userInfo prop is missing (though it should be passed)
            if (!schoolId) {
                const userInfoStr = localStorage.getItem('user_info');
                if (userInfoStr) {
                    try {
                        const u = JSON.parse(userInfoStr);
                        schoolId = u.school_id || "";
                    } catch (e) { }
                }
            }

            // Fallback: If schoolId is still missing but we have school_name, try to fetch school info
            if (!schoolId) {
                const schoolName = userInfo?.school_name || userInfo?.schoolName; // Try both casings
                if (schoolName) {
                    console.log("School ID missing, fetching by name:", schoolName);
                    try {
                        const schoolRes = await invoke<string>('get_school_by_name', { name: schoolName });
                        const schoolParsed = JSON.parse(schoolRes);
                        const schoolData = schoolParsed.data || schoolParsed;
                        if (schoolData.code === 200 && schoolData.schools?.length > 0) {
                            schoolId = schoolData.schools[0].id;
                            console.log("Fetched School ID:", schoolId);
                        }
                    } catch (e) {
                        console.error("Failed to fetch school by name", e);
                    }
                }
            }

            // Final check
            if (!schoolId) {
                console.warn("School ID not found in userInfo or via fetch", userInfo);
                // Can't proceed without school ID
                setLoading(false);
                return;
            }

            const res = await invoke<string>('get_school_course_schedule', {
                schoolId: String(schoolId), // Ensure it's a string
                term: getCurrentTerm(),
                token
            });
            console.log("School Course Schedule Response:", res);
            try {
                const parsed = JSON.parse(res);
                if (parsed.code === 200 && Array.isArray(parsed.data)) {
                    setScheduleData(parsed.data);
                }
            } catch (e) {
                console.error("Parse error", e);
            }

        } catch (e) {
            console.error("Fetch failed", e);
        } finally {
            setLoading(false);
        }
    };

    if (!isOpen) return null;

    // Helper to get cell content
    const getCellContent = (classInfo: ClassSchedule, dayIndex: number, timeIndex: number) => {
        if (!classInfo.schedules?.cells) return "";
        const cell = classInfo.schedules.cells.find(c => c.col_index === dayIndex && c.row_index === timeIndex);
        return cell ? cell.course_name : "";
    };

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-slate-900/50 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-[#f8f9fa] rounded-2xl shadow-2xl w-[1280px] h-[800px] flex flex-col overflow-hidden border border-white/60 ring-1 ring-black/5"
            >
                {/* Header - Fresh & Clean */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-white px-6 py-4 flex items-center justify-between flex-shrink-0 cursor-move border-b border-gray-200 shadow-sm relative z-20"
                >
                    <div className="flex items-center gap-6">
                        {/* Title Section */}
                        <div className="flex items-center gap-3">
                            <div className="bg-blue-50 text-blue-600 p-2.5 rounded-xl border border-blue-100 shadow-sm">
                                <Calendar size={22} strokeWidth={2.5} />
                            </div>
                            <div>
                                <h2 className="text-xl font-bold text-slate-800 tracking-tight">年级总课表</h2>
                                <p className="text-xs text-slate-500 font-medium mt-0.5">Academic Schedule</p>
                            </div>
                        </div>

                        <div className="h-8 w-px bg-gray-200 mx-2"></div>

                        {/* Grade Selector - Pill Style */}
                        <div className="relative group">
                            <select
                                value={selectedGrade}
                                onChange={(e) => setSelectedGrade(e.target.value)}
                                className="appearance-none bg-slate-50 hover:bg-white border border-slate-200 hover:border-blue-300 rounded-full px-5 py-2 pr-10 text-slate-700 font-bold text-sm focus:outline-none focus:ring-4 focus:ring-blue-500/10 transition-all min-w-[160px] cursor-pointer shadow-sm"
                            >
                                {grades.map(g => (
                                    <option key={g} value={g}>{g}</option>
                                ))}
                            </select>
                            <ChevronDown size={16} className="absolute right-4 top-1/2 -translate-y-1/2 text-slate-400 pointer-events-none group-hover:text-blue-500 transition-colors" />
                            <div className="absolute inset-0 rounded-full ring-1 ring-inset ring-black/5 pointer-events-none"></div>
                        </div>
                    </div>

                    <div className="flex items-center gap-3">
                        {/* Display Controls */}
                        <div className="flex items-center bg-slate-100/80 rounded-lg p-1 border border-slate-200/60 shadow-inner">
                            <button
                                onClick={() => setFontScale(s => Math.min(s + 0.1, 2))}
                                className="w-8 h-8 flex items-center justify-center hover:bg-white hover:text-blue-600 rounded-md text-slate-500 transition-all font-bold text-sm shadow-sm hover:shadow"
                                title="放大字体"
                            >
                                A+
                            </button>
                            <div className="w-px h-4 bg-slate-300/50 mx-1"></div>
                            <button
                                onClick={() => setFontScale(s => Math.max(s - 0.1, 0.5))}
                                className="w-8 h-8 flex items-center justify-center hover:bg-white hover:text-blue-600 rounded-md text-slate-500 transition-all font-bold text-xs shadow-sm hover:shadow"
                                title="缩小字体"
                            >
                                A-
                            </button>
                        </div>

                        {/* Actions */}
                        <div className="flex items-center gap-2 ml-2">
                            <button
                                onClick={fetchSchoolSchedule}
                                className="flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg shadow-sm hover:shadow-md hover:-translate-y-0.5 transition-all active:scale-95 group"
                            >
                                <RefreshCw size={16} className={`group-hover:rotate-180 transition-transform duration-500 ${loading ? "animate-spin" : ""}`} />
                                <span className="text-sm font-bold">刷新</span>
                            </button>

                            <button
                                onClick={onClose}
                                className="p-2 hover:bg-red-50 text-slate-400 hover:text-red-500 rounded-lg transition-colors"
                            >
                                <X size={24} />
                            </button>
                        </div>
                    </div>
                </div>

                {/* Main Content Area */}
                <div className="flex-1 overflow-auto bg-[#f0f2f5] p-6 relative custom-scrollbar">
                    {/* Background Pattern */}
                    <div className="absolute inset-0 opacity-[0.015] pointer-events-none"
                        style={{ backgroundImage: 'radial-gradient(#444 1px, transparent 1px)', backgroundSize: '16px 16px' }}>
                    </div>

                    {loading && scheduleData.length === 0 ? (
                        <div className="absolute inset-0 flex flex-col items-center justify-center z-50 bg-white/60 backdrop-blur-sm">
                            <div className="bg-white p-6 rounded-2xl shadow-xl flex flex-col items-center animate-bounce-subtle">
                                <div className="p-3 bg-blue-50 rounded-full mb-4">
                                    <RefreshCw size={32} className="animate-spin text-blue-500" />
                                </div>
                                <p className="text-slate-600 font-bold">正在同步课程数据...</p>
                            </div>
                        </div>
                    ) : !currentViewData ? (
                        <div className="flex flex-col items-center justify-center h-full text-slate-400 gap-6">
                            <div className="w-48 h-48 bg-white rounded-full shadow-lg flex items-center justify-center border-4 border-slate-50">
                                <img src="/empty-state.svg" className="w-24 h-24 opacity-20" alt="" onError={(e) => e.currentTarget.style.display = 'none'} />
                                <Calendar size={64} className="text-slate-200" />
                            </div>
                            <div className="text-center">
                                <h3 className="text-xl font-bold text-slate-600 mb-2">准备就绪</h3>
                                <p className="text-slate-400 max-w-xs mx-auto">请从左上角选择一个年级以查看详细课程安排</p>
                            </div>
                        </div>
                    ) : (
                        <div className="inline-block min-w-full bg-white rounded-xl shadow-sm border border-slate-200 overflow-hidden relative z-10">
                            {/* The Grid Table */}
                            <div className="grid bg-slate-200 gap-[1px]" // Using gap for crisp borders
                                style={{
                                    gridTemplateColumns: `100px repeat(${currentViewData.days.length * currentViewData.classes.length}, minmax(100px, 1fr))`
                                }}
                            >
                                {/* --- Header Row 1: Days --- */}
                                <div className="bg-slate-50 sticky top-0 z-30 h-14 border-b border-slate-200 flex flex-col items-center justify-center">
                                    <span className="text-xs font-bold text-slate-400 uppercase tracking-wider">Time</span>
                                </div>

                                {currentViewData.days.map((day, dayIdx) => (
                                    <div
                                        key={day}
                                        className="bg-white sticky top-0 z-30 flex items-center justify-center border-b border-slate-200 shadow-sm"
                                        style={{
                                            gridColumn: `span ${currentViewData.classes.length}`,
                                            fontSize: `${15 * fontScale}px`
                                        }}
                                    >
                                        <div className={`px-4 py-1.5 rounded-full font-bold text-sm tracking-wide ${day === '周一' ? 'bg-red-50 text-red-600' :
                                                day === '周二' ? 'bg-orange-50 text-orange-600' :
                                                    day === '周三' ? 'bg-amber-50 text-amber-600' :
                                                        day === '周四' ? 'bg-emerald-50 text-emerald-600' :
                                                            day === '周五' ? 'bg-blue-50 text-blue-600' :
                                                                'bg-slate-100 text-slate-600'
                                            }`}>
                                            {day}
                                        </div>
                                    </div>
                                ))}

                                {/* --- Header Row 2: Classes --- */}
                                <div className="bg-slate-50 sticky top-14 z-20 h-10 flex items-center justify-center border-b border-slate-200">
                                    <div className="w-1.5 h-1.5 rounded-full bg-slate-300"></div>
                                </div>
                                {currentViewData.days.flatMap((day) =>
                                    currentViewData.classes.map((cls) => (
                                        <div
                                            key={`${day}-${cls.class_id}`}
                                            className="bg-slate-50 sticky top-14 z-20 p-2 flex items-center justify-center text-slate-600 font-bold border-b border-slate-200 hover:bg-slate-100 transition-colors"
                                            style={{ fontSize: `${12 * fontScale}px` }}
                                        >
                                            <span className="truncate max-w-full">{cls.class_name || cls.class_id}</span>
                                        </div>
                                    ))
                                )}

                                {/* --- Data Rows: Times --- */}
                                {currentViewData.times.map((time, timeIdx) => {
                                    // Calculate if it's morning or afternoon for subtle background distinction
                                    const isAfternoon = timeIdx > 3; // Rough heuristic, usually 4 classes in morning

                                    return (
                                        <div key={`row-${timeIdx}`} className="contents group/row">
                                            {/* Time Column */}
                                            <div
                                                className="bg-white p-3 flex flex-col items-center justify-center text-xs font-semibold text-slate-500 group-hover/row:bg-slate-50 relative"
                                                style={{ fontSize: `${12 * fontScale}px` }}
                                            >
                                                {/* Visual connector line segment */}
                                                <div className={`absolute left-0 w-1 inset-y-0 ${timeIdx < 4 ? 'bg-amber-400/50' :
                                                        timeIdx === 4 ? 'bg-slate-300' : 'bg-indigo-400/50'
                                                    }`}></div>
                                                <span className="text-slate-800 font-bold mb-0.5">第{timeIdx + 1}节</span>
                                                <span className="text-[10px] opacity-70 scale-90">{time}</span>
                                            </div>

                                            {/* Course Cells */}
                                            {currentViewData.days.flatMap((day, dayIdx) =>
                                                currentViewData.classes.map((cls) => {
                                                    const content = getCellContent(cls, dayIdx, timeIdx);
                                                    const hasContent = !!content;

                                                    // Dynamic cell styling
                                                    const cellBg = hasContent
                                                        ? 'bg-white hover:bg-blue-50/50'
                                                        : 'bg-slate-50/30 hover:bg-slate-50/80';

                                                    return (
                                                        <div
                                                            key={`${timeIdx}-${dayIdx}-${cls.class_id}`}
                                                            className={`${cellBg} p-1.5 flex items-center justify-center min-h-[64px] transition-all duration-200 relative group/cell cursor-default`}
                                                            style={{ fontSize: `${13 * fontScale}px` }}
                                                        >
                                                            {content && (
                                                                <div className="w-full h-full flex flex-col items-center justify-center text-center rounded-lg border border-transparent group-hover/cell:border-blue-100 group-hover/cell:shadow-sm px-1 py-1 transition-all">
                                                                    <span className="font-bold text-slate-700 line-clamp-2 leading-tight group-hover/cell:text-blue-700">
                                                                        {content}
                                                                    </span>
                                                                </div>
                                                            )}

                                                            {/* Empty State Plus Icon (Invisible mostly, appears on hover maybe for future editing) */}
                                                            {!content && (
                                                                <div className="opacity-0 group-hover/cell:opacity-20 text-slate-400 font-2xl">+</div>
                                                            )}
                                                        </div>
                                                    );
                                                })
                                            )}
                                        </div>
                                    );
                                })}
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default SchoolCourseScheduleModal;
