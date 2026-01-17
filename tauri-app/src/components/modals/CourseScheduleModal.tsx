import { useRef, useState, useEffect, Fragment } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { X, Calendar, Download, RefreshCw, FileSpreadsheet } from 'lucide-react';
import * as XLSX from 'xlsx';
import { useDraggable } from '../../hooks/useDraggable';
import { getCurrentTerm } from '../../utils/term';

interface CourseScheduleModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
}

// Mock Schedule Data removed

// Removed static CURRENT_TERM

const CourseScheduleModal = ({ isOpen, onClose, classId }: CourseScheduleModalProps) => {
    const { style, handleMouseDown } = useDraggable();
    const [scheduleData, setScheduleData] = useState<any[]>([]); // Initialize empty
    const [loading, setLoading] = useState(false);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const notifyTodayScheduleRefresh = (classId: string) => {
        window.dispatchEvent(new CustomEvent('refresh-daily-schedule', { detail: { classId } }));
    };

    useEffect(() => {
        if (isOpen && classId) {
            fetchSchedule();
        }
    }, [isOpen, classId]);

    const fetchSchedule = async () => {
        if (!classId) return;
        setLoading(true);
        try {
            // Get token
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                } catch (e) {
                    console.error("Failed to parse user info", e);
                }
            }

            const term = getCurrentTerm();
            console.log('[CourseScheduleModal] Fetching get_course_schedule', { classId, term });
            const res = await invoke<string>('get_course_schedule', {
                classId,
                term,
                token
            });
            console.log("Course Schedule Response:", res);
            try {
                const parsed = JSON.parse(res);
                if (parsed.code === 200 && parsed.data) {
                    // Check for new structure { schedule: ..., cells: ... }
                    if (parsed.data.schedule && parsed.data.cells) {
                        const { times, days } = parsed.data.schedule;
                        const cells = parsed.data.cells;

                        // Reconstruct rows
                        // Reconstruct rows
                        // times is ["08:00..", ..]
                        const rows = times.map((t: string, i: number) => {
                            const row: any = { time: t };

                            // cells have row_index, col_index
                            // days array is ["mon", "tue", "wed", "thu", "fri", "sat", "sun"]

                            days.forEach((d: string, j: number) => {
                                // Find cell by indices matching row i and col j
                                const cell = cells.find((c: any) => c.row_index === i && c.col_index === j);
                                row[d] = cell ? (cell.course_name || cell.subject || "") : "";
                            });

                            // Label handling (lunch)
                            // Robust check for "lunch" or "午"
                            if (t && (t.includes("午") || t.toLowerCase().includes("lunch"))) {
                                row.label = t;
                                row.time = "lunch";
                            }

                            return row;
                        });
                        setScheduleData(rows);
                    } else {
                        // Old fallback
                        const list = Array.isArray(parsed.data) ? parsed.data : (parsed.data.courses || []);
                        if (Array.isArray(list)) {
                            setScheduleData(list);
                        }
                    }
                }
            } catch (e) {
                console.error("Parse error", e);
            }
        } catch (e) {
            console.error("Fetch schedule error:", e);
        } finally {
            setLoading(false);
        }
    };

    const handleImportClick = () => {
        fileInputRef.current?.click();
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !classId) return;

        try {
            setLoading(true);
            const data = await file.arrayBuffer();
            const workbook = XLSX.read(data);
            const worksheet = workbook.Sheets[workbook.SheetNames[0]];
            // Use header:1 to get array of arrays (index based)
            // This is robust against mismatched header strings
            const jsonData = XLSX.utils.sheet_to_json(worksheet, { header: 1 }) as any[][];

            // Filter logic:
            // 1. Skip completely empty rows
            // 2. Skip header row (heuristic: contains "周一" or "Mon")
            const mappedData = jsonData
                .filter(row => row && row.length > 0)
                .filter(row => {
                    const str = JSON.stringify(row);
                    return !str.includes("周一") && !str.includes("Mon");
                })
                .map((row) => ({
                    time: row[0] ? String(row[0]) : '', // Col 0 is Time
                    label: (row[0] && String(row[0]).includes('午')) ? String(row[0]) : '', // Heuristic for lunch
                    mon: row[1] ? String(row[1]) : '',
                    tue: row[2] ? String(row[2]) : '',
                    wed: row[3] ? String(row[3]) : '',
                    thu: row[4] ? String(row[4]) : '',
                    fri: row[5] ? String(row[5]) : '',
                    sat: row[6] ? String(row[6]) : '',
                    sun: row[7] ? String(row[7]) : ''
                }));

            // Save to backend
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                } catch (e) { }
            }

            // Derive metadata
            const times = mappedData.map((d: any) => d.time || "");
            const days = ['mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'];

            // Convert rows to flat cells
            const cells: any[] = [];
            mappedData.forEach((row: any, rowIndex: number) => {
                days.forEach((day, colIndex) => {
                    if (row[day]) {
                        cells.push({
                            row_index: rowIndex, // Send index
                            col_index: colIndex,
                            course_name: row[day], // specific field name
                            day: day, // keep for debug/robustness?
                            time: row.time
                        });
                    }
                });
            });

            const saveRes = await invoke<string>('save_course_schedule', {
                classId,
                term: getCurrentTerm(),
                days,
                times,
                cells: cells,
                token
            });
            console.log("Save Course Schedule Response:", saveRes);

            setScheduleData(mappedData);
            fetchSchedule();
            if (classId) notifyTodayScheduleRefresh(classId);

        } catch (e) {
            console.error("Import failed", e);
            alert("导入失败");
        } finally {
            setLoading(false);
            if (fileInputRef.current) fileInputRef.current.value = '';
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl w-[900px] h-[650px] flex flex-col overflow-hidden border border-white/50 ring-1 ring-sage-100/50"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-white/30 px-6 py-4 flex items-center justify-between text-ink-800 flex-shrink-0 cursor-move select-none border-b border-sage-100/50 backdrop-blur-md"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 bg-gradient-to-br from-sage-400 to-sage-600 rounded-2xl flex items-center justify-center shadow-lg shadow-sage-500/20 text-white">
                            <Calendar size={20} />
                        </div>
                        <div>
                            <div className="flex items-center gap-2">
                                <span className="font-bold text-lg tracking-tight">班级课程表</span>
                                <span className="text-xs font-bold text-sage-600 bg-sage-50 px-2 py-0.5 rounded-lg border border-sage-100 shadow-sm">{getCurrentTerm()}</span>
                            </div>
                        </div>
                    </div>
                    <div className="flex items-center gap-2">
                        <input
                            type="file"
                            ref={fileInputRef}
                            className="hidden"
                            accept=".xlsx, .xls"
                            onChange={handleFileChange}
                        />
                        <button onClick={handleImportClick} className="p-2 hover:bg-white hover:text-sage-600 rounded-xl transition-all text-sage-400 hover:shadow-sm" title="导入Excel">
                            <FileSpreadsheet size={20} />
                        </button>
                        <button onClick={fetchSchedule} className="p-2 hover:bg-white hover:text-sage-600 rounded-xl transition-all text-sage-400 hover:shadow-sm">
                            <RefreshCw size={20} className={loading ? "animate-spin" : ""} />
                        </button>
                        <button className="p-2 hover:bg-white hover:text-sage-600 rounded-xl transition-all text-sage-400 hover:shadow-sm">
                            <Download size={20} />
                        </button>
                        <div className="w-[1px] h-6 bg-sage-200/50 mx-1"></div>
                        <button onClick={onClose} className="w-9 h-9 flex items-center justify-center hover:bg-clay-50 hover:text-clay-600 rounded-full transition-all text-sage-400">
                            <X size={22} />
                        </button>
                    </div>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-auto p-6 bg-white/40 custom-scrollbar">
                    {scheduleData.length === 0 ? (
                        /* Empty State */
                        <div className="h-full flex flex-col items-center justify-center text-center gap-4">
                            <div className="w-24 h-24 bg-sage-50 rounded-full flex items-center justify-center border border-sage-100 shadow-inner">
                                <Calendar size={48} className="text-sage-300" />
                            </div>
                            <div>
                                <h3 className="text-lg font-bold text-ink-600 mb-1">暂无课程表数据</h3>
                                <p className="text-ink-400 text-sm font-medium">请导入Excel课程表文件以开始使用</p>
                            </div>
                            <button
                                onClick={handleImportClick}
                                className="mt-4 px-6 py-3 bg-sage-600 hover:bg-sage-700 text-white font-bold rounded-2xl flex items-center gap-2 transition-all shadow-lg shadow-sage-500/20 active:scale-95"
                            >
                                <FileSpreadsheet size={20} />
                                导入课程表
                            </button>
                        </div>
                    ) : (
                        <div className="bg-white/80 rounded-2xl border border-white/60 shadow-sm overflow-hidden ring-1 ring-sage-50 backdrop-blur-sm">
                            <div className="grid grid-cols-6 text-sm">
                                {/* Header Row */}
                                <div className="bg-sage-50/80 p-3 font-bold text-sage-500 text-center border-b border-r border-sage-100">时间</div>
                                {['周一', '周二', '周三', '周四', '周五'].map(day => (
                                    <div key={day} className="bg-sage-50/80 p-3 font-bold text-ink-700 text-center border-b border-sage-100 border-r last:border-r-0">
                                        {day}
                                    </div>
                                ))}

                                {/* Schedule Rows */}
                                {scheduleData.map((row, index) => {
                                    if (row.time === 'lunch') {
                                        return (
                                            <div key={index} className="col-span-6 bg-orange-50/30 p-2 text-center text-orange-400 font-bold text-xs border-b border-sage-100 tracking-wider">
                                                {row.label}
                                            </div>
                                        );
                                    }
                                    return (
                                        <Fragment key={index}>
                                            {/* Time Column */}
                                            <div className="p-4 flex items-center justify-center text-sage-500 font-bold border-b border-r border-sage-100 text-xs bg-sage-50/30">
                                                {row.time}
                                            </div>
                                            {/* Course Columns */}
                                            {[row.mon, row.tue, row.wed, row.thu, row.fri].map((course, i) => (
                                                <div key={i} className="p-2 border-b border-r border-sage-100 last:border-r-0 h-24 relative group hover:bg-white transition-colors">
                                                    <div className={`
                                                    h-full w-full rounded-xl flex flex-col items-center justify-center gap-1 cursor-pointer transition-all hover:shadow-md border border-transparent hover:border-white/50
                                                    ${course === '数学' ? 'bg-blue-50 text-blue-700' :
                                                            course === '语文' ? 'bg-red-50 text-red-700' :
                                                                course === '英语' ? 'bg-orange-50 text-orange-700' :
                                                                    course === '物理' ? 'bg-indigo-50 text-indigo-700' :
                                                                        course === '化学' ? 'bg-purple-50 text-purple-700' :
                                                                            course === '体育' ? 'bg-green-50 text-green-700' :
                                                                                course ? 'bg-sage-50 text-sage-700' :
                                                                                    'hover:bg-sage-50/30'}
                                                `}>
                                                        <span className="font-bold text-base">{course || ''}</span>
                                                        {course && <span className="text-[10px] opacity-70 font-medium">Classroom</span>}
                                                    </div>
                                                </div>
                                            ))}
                                        </Fragment>
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

export default CourseScheduleModal;
