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
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl w-[1280px] h-[800px] flex flex-col overflow-hidden border border-white/50 ring-1 ring-sage-100/50"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-white/30 px-8 py-5 flex items-center justify-between flex-shrink-0 cursor-move border-b border-sage-100/50 backdrop-blur-md relative z-20"
                >
                    <div className="flex items-center gap-6">
                        {/* Title Section */}
                        <div className="flex items-center gap-4">
                            <div className="bg-gradient-to-br from-sage-400 to-sage-600 text-white w-12 h-12 rounded-2xl flex items-center justify-center shadow-lg shadow-sage-500/20">
                                <Calendar size={24} strokeWidth={2.5} />
                            </div>
                            <div>
                                <h2 className="text-2xl font-bold text-ink-800 tracking-tight">年级总课表</h2>
                                <p className="text-sm font-medium text-ink-400 mt-0.5 tracking-wide">Academic Schedule</p>
                            </div>
                        </div>

                        <div className="h-8 w-px bg-sage-200/50 mx-2"></div>

                        {/* Grade Selector - Pill Style */}
                        <div className="relative group">
                            <select
                                value={selectedGrade}
                                onChange={(e) => setSelectedGrade(e.target.value)}
                                className="appearance-none bg-white/60 hover:bg-white border border-sage-200 hover:border-sage-400 rounded-full px-6 py-2.5 pr-12 text-ink-700 font-bold text-sm focus:outline-none focus:ring-4 focus:ring-sage-100 transition-all min-w-[180px] cursor-pointer shadow-sm hover:shadow-md"
                            >
                                {grades.map(g => (
                                    <option key={g} value={g}>{g}</option>
                                ))}
                            </select>
                            <ChevronDown size={18} className="absolute right-4 top-1/2 -translate-y-1/2 text-sage-400 pointer-events-none group-hover:text-sage-600 transition-colors" />
                        </div>
                    </div>

                    <div className="flex items-center gap-4">
                        {/* Display Controls */}
                        <div className="flex items-center bg-white/50 rounded-xl p-1.5 border border-white/50 shadow-inner ring-1 ring-sage-50">
                            <button
                                onClick={() => setFontScale(s => Math.min(s + 0.1, 2))}
                                className="w-9 h-9 flex items-center justify-center hover:bg-white hover:text-sage-600 rounded-lg text-ink-400 transition-all font-bold text-sm shadow-none hover:shadow-sm active:scale-95"
                                title="放大字体"
                            >
                                A+
                            </button>
                            <div className="w-px h-5 bg-sage-200/50 mx-1"></div>
                            <button
                                onClick={() => setFontScale(s => Math.max(s - 0.1, 0.5))}
                                className="w-9 h-9 flex items-center justify-center hover:bg-white hover:text-sage-600 rounded-lg text-ink-400 transition-all font-bold text-xs shadow-none hover:shadow-sm active:scale-95"
                                title="缩小字体"
                            >
                                A-
                            </button>
                        </div>

                        {/* Actions */}
                        <div className="flex items-center gap-3 ml-2">
                            <button
                                onClick={fetchSchoolSchedule}
                                className="flex items-center gap-2 px-5 py-2.5 bg-sage-600 hover:bg-sage-700 text-white rounded-xl shadow-lg shadow-sage-500/20 hover:-translate-y-0.5 transition-all active:scale-95 group"
                            >
                                <RefreshCw size={18} className={`group-hover:rotate-180 transition-transform duration-500 ${loading ? "animate-spin" : ""}`} />
                                <span className="text-sm font-bold">刷新</span>
                            </button>

                            <button
                                onClick={onClose}
                                className="w-10 h-10 flex items-center justify-center hover:bg-clay-50 text-sage-400 hover:text-clay-600 rounded-full transition-all duration-300"
                            >
                                <X size={24} />
                            </button>
                        </div>
                    </div>
                </div>

                {/* Main Content Area */}
                <div className="flex-1 overflow-auto bg-white/40 p-8 relative custom-scrollbar">
                    {/* Background Pattern */}
                    <div className="absolute inset-0 opacity-[0.03] pointer-events-none"
                        style={{ backgroundImage: 'radial-gradient(#577b66 1px, transparent 1px)', backgroundSize: '24px 24px' }}>
                    </div>

                    {loading && scheduleData.length === 0 ? (
                        <div className="absolute inset-0 flex flex-col items-center justify-center z-50 bg-white/60 backdrop-blur-sm">
                            <div className="bg-white/80 p-8 rounded-3xl shadow-xl flex flex-col items-center animate-bounce-subtle border border-white/50 ring-1 ring-sage-100">
                                <div className="p-4 bg-sage-50 rounded-full mb-4">
                                    <RefreshCw size={40} className="animate-spin text-sage-500" />
                                </div>
                                <p className="text-ink-600 font-bold text-lg">正在同步课程数据...</p>
                            </div>
                        </div>
                    ) : !currentViewData ? (
                        <div className="flex flex-col items-center justify-center h-full text-sage-400 gap-6">
                            <div className="w-48 h-48 bg-white/60 rounded-full shadow-lg flex items-center justify-center border-4 border-white ring-1 ring-sage-50">
                                <Calendar size={80} className="text-sage-200" />
                            </div>
                            <div className="text-center">
                                <h3 className="text-2xl font-bold text-ink-600 mb-2">准备就绪</h3>
                                <p className="text-ink-400 max-w-xs mx-auto font-medium">请从左上角选择一个年级以查看详细课程安排</p>
                            </div>
                        </div>
                    ) : (
                        <div className="inline-block min-w-full bg-white/80 rounded-2xl shadow-sm border border-white/60 overflow-hidden relative z-10 ring-1 ring-sage-100/50 backdrop-blur-sm">
                            {/* The Grid Table */}
                            <div className="grid bg-sage-200/50 gap-[1px]" // Using gap for crisp borders
                                style={{
                                    gridTemplateColumns: `100px repeat(${currentViewData.days.length * currentViewData.classes.length}, minmax(100px, 1fr))`
                                }}
                            >
                                {/* --- Header Row 1: Days --- */}
                                <div className="bg-sage-50/80 backdrop-blur-sm sticky top-0 z-30 h-16 border-b border-sage-200 flex flex-col items-center justify-center">
                                    <span className="text-xs font-bold text-sage-400 uppercase tracking-wider">Time</span>
                                </div>

                                {currentViewData.days.map((day, dayIdx) => (
                                    <div
                                        key={day}
                                        className="bg-white/90 backdrop-blur-sm sticky top-0 z-30 flex items-center justify-center border-b border-sage-100 shadow-sm"
                                        style={{
                                            gridColumn: `span ${currentViewData.classes.length}`,
                                            fontSize: `${15 * fontScale}px`
                                        }}
                                    >
                                        <div className={`px-5 py-2 rounded-xl font-bold text-sm tracking-wide shadow-sm ${day === '周一' ? 'bg-clay-50 text-clay-600 ring-1 ring-clay-100' :
                                            day === '周二' ? 'bg-orange-50 text-orange-600 ring-1 ring-orange-100' :
                                                day === '周三' ? 'bg-amber-50 text-amber-600 ring-1 ring-amber-100' :
                                                    day === '周四' ? 'bg-sage-50 text-sage-600 ring-1 ring-sage-100' :
                                                        day === '周五' ? 'bg-blue-50 text-blue-600 ring-1 ring-blue-100' :
                                                            'bg-slate-100 text-slate-600'
                                            }`}>
                                            {day}
                                        </div>
                                    </div>
                                ))}

                                {/* --- Header Row 2: Classes --- */}
                                <div className="bg-sage-50/80 backdrop-blur-sm sticky top-16 z-20 h-12 flex items-center justify-center border-b border-sage-200">
                                    <div className="w-1.5 h-1.5 rounded-full bg-sage-300"></div>
                                </div>
                                {currentViewData.days.flatMap((day) =>
                                    currentViewData.classes.map((cls) => (
                                        <div
                                            key={`${day}-${cls.class_id}`}
                                            className="bg-sage-50/50 backdrop-blur-sm sticky top-16 z-20 p-2 flex items-center justify-center text-ink-600 font-bold border-b border-sage-200 hover:bg-sage-100 transition-colors"
                                            style={{ fontSize: `${12 * fontScale}px` }}
                                        >
                                            <span className="truncate max-w-full">{cls.class_name || cls.class_id}</span>
                                        </div>
                                    ))
                                )}

                                {/* --- Data Rows: Times --- */}
                                {currentViewData.times.map((time, timeIdx) => {
                                    // Calculate if it's morning or afternoon for subtle background distinction
                                    const isAfternoon = timeIdx > 3; // Rough heuristic

                                    return (
                                        <div key={`row-${timeIdx}`} className="contents group/row">
                                            {/* Time Column */}
                                            <div
                                                className="bg-white/80 backdrop-blur-sm p-3 flex flex-col items-center justify-center text-xs font-semibold text-sage-500 group-hover/row:bg-sage-50 relative border-r border-sage-100"
                                                style={{ fontSize: `${12 * fontScale}px` }}
                                            >
                                                <span className="text-ink-700 font-bold mb-0.5">第{timeIdx + 1}节</span>
                                                <span className="text-[10px] opacity-70 scale-90 font-mono">{time}</span>
                                            </div>

                                            {/* Course Cells */}
                                            {currentViewData.days.flatMap((day, dayIdx) =>
                                                currentViewData.classes.map((cls) => {
                                                    const content = getCellContent(cls, dayIdx, timeIdx);
                                                    const hasContent = !!content;

                                                    // Dynamic cell styling
                                                    const cellBg = hasContent
                                                        ? 'bg-white hover:bg-sage-50 hover:shadow-inner'
                                                        : 'bg-sage-50/20 hover:bg-sage-50/60';

                                                    return (
                                                        <div
                                                            key={`${timeIdx}-${dayIdx}-${cls.class_id}`}
                                                            className={`${cellBg} p-1.5 flex items-center justify-center min-h-[64px] transition-all duration-200 relative group/cell cursor-default`}
                                                            style={{ fontSize: `${13 * fontScale}px` }}
                                                        >
                                                            {content && (
                                                                <div className="w-full h-full flex flex-col items-center justify-center text-center rounded-lg border border-transparent group-hover/cell:border-sage-200 group-hover/cell:bg-white group-hover/cell:shadow-sm px-1 py-1 transition-all">
                                                                    <span className="font-bold text-ink-700 line-clamp-2 leading-tight group-hover/cell:text-sage-700">
                                                                        {content}
                                                                    </span>
                                                                </div>
                                                            )}

                                                            {/* Empty State Plus Icon (Invisible mostly) */}
                                                            {!content && (
                                                                <div className="opacity-0 group-hover/cell:opacity-20 text-sage-400 font-2xl">+</div>
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
