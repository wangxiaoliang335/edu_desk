import { useState, useEffect, useRef } from 'react';
import { X, Shuffle, RotateCcw, Filter, ChevronDown, Check, Users } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface RandomCallModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
    students?: string[]; // Fallback simple list
}

interface AttributeTable {
    id: string;
    name: string;
    headers: string[]; // e.g. ["学号", "姓名", "性别", "小组"]
    rows: any[]; // e.g. [{ "学号": "1001", "姓名": "张三", "性别": "男"... }]
}

const RandomCallModal = ({ isOpen, onClose, classId, students: initialStudents = [] }: RandomCallModalProps) => {
    // Data State
    const [tables, setTables] = useState<AttributeTable[]>([]);
    const [loading, setLoading] = useState(false);

    // Filter State
    const [selectedTableId, setSelectedTableId] = useState<string>('');
    const [selectedAttribute, setSelectedAttribute] = useState<string>('');
    const [filterValue, setFilterValue] = useState<string>('ALL');

    // Derived State
    const [targetStudents, setTargetStudents] = useState<string[]>([]);
    const [availableAttributes, setAvailableAttributes] = useState<string[]>([]);
    const [availableValues, setAvailableValues] = useState<string[]>([]);

    // Animation State
    const [currentName, setCurrentName] = useState("---");
    const [isRunning, setIsRunning] = useState(false);
    const [history, setHistory] = useState<string[]>([]);

    // UI Refs
    const intervalRef = useRef<any>(null);
    const { style, handleMouseDown } = useDraggable();

    // Fetch Tables on Open
    useEffect(() => {
        if (isOpen && classId) {
            fetchAttributeTables();
        } else if (isOpen && initialStudents.length > 0) {
            // Fallback mode if no classId but students passed
            setTargetStudents(initialStudents);
        } else if (!isOpen) {
            // Reset state on close
            setIsRunning(false);
            if (intervalRef.current) clearInterval(intervalRef.current);
            setCurrentName("---");
        }
    }, [isOpen, classId]);

    const fetchAttributeTables = async () => {
        setLoading(true);
        try {
            let targetClassId = classId || '';
            if (targetClassId.length === 11) targetClassId = targetClassId.substring(0, 9);

            const newTables: AttributeTable[] = [];

            // 1. Fetch Student Scores (Individual Attributes)
            try {
                const res = await fetch(`/api/student-scores?class_id=${targetClassId}`);
                const json = await res.json();
                if (json.code === 200 && json.data && json.data.headers) {
                    const list = Array.isArray(json.data.headers) ? json.data.headers : [json.data.headers];
                    list.forEach((item: any) => {
                        const filesList = (item.excel_file_url && Array.isArray(item.excel_file_url))
                            ? item.excel_file_url
                            : ((item.excel_files && Array.isArray(item.excel_files)) ? item.excel_files : []);

                        if (filesList.length > 0) {
                            filesList.forEach((ef: any, idx: number) => {
                                const rawFields = ef.fields || [];
                                const fields = rawFields
                                    .map((f: any) => typeof f === 'string' ? f : f.field_name)
                                    .filter((f: string) => f !== '总分');

                                const rows = (item.scores || []).map((s: any) => {
                                    const row: any = { '学号': s.student_id, '姓名': s.student_name };
                                    fields.forEach((f: string) => {
                                        const specificKey = `${f}_${ef.filename}`;
                                        row[f] = s.scores_json_full?.[specificKey] ?? s.scores_json_full?.[f] ?? s[f] ?? "";
                                    });
                                    return row;
                                });

                                newTables.push({
                                    id: `score_${item.id}_${idx}`,
                                    name: ef.filename || item.exam_name || "未命名成绩单",
                                    headers: ['学号', '姓名', ...fields],
                                    rows: rows
                                });
                            });
                        } else {
                            const rawFields = item.fields || [];
                            const fields = rawFields
                                .map((f: any) => typeof f === 'string' ? f : f.field_name)
                                .filter((f: string) => f !== '总分');

                            const rows = (item.scores || []).map((s: any) => {
                                const row: any = { '学号': s.student_id, '姓名': s.student_name };
                                fields.forEach((f: string) => {
                                    row[f] = s.scores_json_full?.[f] ?? s[f] ?? "";
                                });
                                return row;
                            });

                            newTables.push({
                                id: `score_${item.id}`,
                                name: item.exam_name || "未命名成绩单",
                                headers: ['学号', '姓名', ...fields],
                                rows: rows
                            });
                        }
                    });
                }
            } catch (e) {
                console.error("Failed to fetch student scores:", e);
            }

            // 2. Fetch Group Scores (Group Attributes)
            try {
                const now = new Date();
                const year = now.getFullYear();
                const month = now.getMonth() + 1;
                const term = (month >= 8 || month <= 1) ? `${month >= 8 ? year : year - 1}-${month >= 8 ? year + 1 : year}-1` : `${year - 1}-${year}-2`;

                const res = await fetch(`/api/group-scores?class_id=${targetClassId}&term=${term}`);
                const json = await res.json();

                if (json.code === 200 && json.data) {
                    const groupList = Array.isArray(json.data) ? json.data : (json.data.header ? [json.data] : []);

                    groupList.forEach((tableData: any, gIdx: number) => {
                        const headerInfo = tableData.header || {};
                        const groupScores = tableData.group_scores || [];
                        const fileUrls = headerInfo.excel_file_url;

                        if (fileUrls && typeof fileUrls === 'object' && Object.keys(fileUrls).length > 0) {
                            Object.keys(fileUrls).forEach((filename, fIdx) => {
                                const fileInfo = fileUrls[filename];
                                const rawFields = fileInfo.fields || [];
                                const fields = rawFields.filter((f: string) => !['总分', '小组总分'].includes(f));

                                const rows: any[] = [];
                                groupScores.forEach((g: any) => {
                                    if (g.students) {
                                        g.students.forEach((s: any) => {
                                            const row: any = {
                                                '学号': s.student_id,
                                                '姓名': s.student_name,
                                                '小组': g.group_name
                                            };
                                            fields.forEach((f: string) => {
                                                const specificKey = `${f}_${filename}`;
                                                row[f] = s.scores_json_full?.[specificKey] ?? s.scores_json_full?.[f] ?? "";
                                            });
                                            rows.push(row);
                                        });
                                    }
                                });

                                newTables.push({
                                    id: `group_${headerInfo.id || gIdx}_${fIdx}`,
                                    name: filename || "未命名小组表",
                                    headers: ['小组', '学号', '姓名', ...fields],
                                    rows: rows
                                });
                            });
                        } else {
                            let inferredFields: string[] = [];
                            if (groupScores?.[0]?.students?.[0]?.scores_json_full) {
                                inferredFields = Object.keys(groupScores[0].students[0].scores_json_full).filter(k => !k.includes("_"));
                            }
                            const rawFields = (headerInfo.fields?.map((f: any) => typeof f === 'string' ? f : f.field_name) || inferredFields);
                            const fields = rawFields.filter((f: string) => !['总分', '小组总分'].includes(f));

                            const rows: any[] = [];
                            groupScores.forEach((g: any) => {
                                if (g.students) {
                                    g.students.forEach((s: any) => {
                                        const row: any = {
                                            '学号': s.student_id,
                                            '姓名': s.student_name,
                                            '小组': g.group_name
                                        };
                                        fields.forEach((f: string) => {
                                            row[f] = s.scores_json_full?.[f] ?? "";
                                        });
                                        rows.push(row);
                                    });
                                }
                            });

                            newTables.push({
                                id: `group_${headerInfo.id || gIdx}`,
                                name: headerInfo.exam_name || "未命名小组表",
                                headers: ['小组', '学号', '姓名', ...fields],
                                rows: rows
                            });
                        }
                    });
                }
            } catch (e) {
                console.error("Failed to fetch group scores:", e);
            }

            setTables(newTables);

            if (newTables.length > 0) {
                if (!newTables.find(t => t.id === selectedTableId)) {
                    setSelectedTableId(newTables[0].id);
                }
            } else {
                if (initialStudents.length > 0 && newTables.length === 0) {
                    setTargetStudents(initialStudents);
                } else {
                    setTargetStudents([]);
                }
            }

        } catch (e) {
            console.error(e);
            if (initialStudents.length > 0) setTargetStudents(initialStudents);
        } finally {
            setLoading(false);
        }
    };

    // Update Available Attributes when Table Changes
    useEffect(() => {
        if (!selectedTableId) return;
        const table = tables.find(t => t.id === selectedTableId);
        if (table) {
            // Exclude standard non-attribute fields if necessary, but user might want to filter by ID?
            // Usually attributes are columns like "Gender", "Group", "Score"
            const attrs = table.headers.filter(h => !['学号', '姓名', '总分'].includes(h));
            setAvailableAttributes(attrs);
            setSelectedAttribute(attrs.length > 0 ? attrs[0] : '');
        }
    }, [selectedTableId, tables]);

    // Update Available Values and Students when Attribute/Table Changes
    useEffect(() => {
        if (!selectedTableId) {
            // No table selected, maybe fallback students?
            return;
        }

        const table = tables.find(t => t.id === selectedTableId);
        if (!table) return;

        // If no attribute selected, all students in table
        if (!selectedAttribute) {
            setTargetStudents(table.rows.map(r => r['姓名']));
            return;
        }

        // Get unique values for the attribute
        const values = Array.from(new Set(table.rows.map(r => r[selectedAttribute]))).filter(v => v !== undefined && v !== "") as string[];
        setAvailableValues(['ALL', ...values]);

        // Reset filter value to ALL when attribute changes
        setFilterValue('ALL');

    }, [selectedAttribute, selectedTableId, tables]);


    // Filter Students when Value Changes
    useEffect(() => {
        if (!selectedTableId) return;
        const table = tables.find(t => t.id === selectedTableId);
        if (!table) return;

        let filtered = table.rows;
        if (selectedAttribute && filterValue !== 'ALL') {
            filtered = filtered.filter(r => String(r[selectedAttribute]) === filterValue);
        }

        setTargetStudents(filtered.map(r => r['姓名']));
    }, [filterValue, selectedAttribute, selectedTableId, tables]);


    const toggleRollCall = () => {
        if (targetStudents.length === 0) {
            alert("当前列表没有学生，请调整筛选条件");
            return;
        }

        if (isRunning) {
            // Stop
            if (intervalRef.current) clearInterval(intervalRef.current);
            setIsRunning(false);
            // Add to history
            if (currentName !== "---") {
                setHistory(prev => [currentName, ...prev].slice(0, 5));
            }
        } else {
            // Start
            setIsRunning(true);
            intervalRef.current = setInterval(() => {
                const randomName = targetStudents[Math.floor(Math.random() * targetStudents.length)];
                setCurrentName(randomName);
            }, 50); // Fast cycle
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl w-[500px] overflow-hidden border border-white/60 ring-1 ring-sage-100/50 flex flex-col animate-in zoom-in-95 duration-200"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="p-5 flex items-center justify-between border-b border-sage-100/50 bg-white/40 backdrop-blur-md cursor-move select-none"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-2xl bg-gradient-to-br from-indigo-400 to-indigo-600 text-white flex items-center justify-center shadow-lg shadow-indigo-500/20">
                            <Shuffle size={20} />
                        </div>
                        <span className="font-bold text-ink-800 text-lg tracking-tight">随机点名</span>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-9 h-9 flex items-center justify-center rounded-full text-sage-400 hover:text-clay-600 hover:bg-clay-50 transition-all duration-300"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Filter Section */}
                <div className="px-6 py-4 bg-white/30 border-b border-sage-100/50 flex gap-3 backdrop-blur-sm">
                    {/* Table Select */}
                    <div className="flex-1 group">
                        <label className="text-xs font-bold text-sage-500 mb-1.5 block uppercase tracking-wide pl-1">筛选表格</label>
                        <div className="relative">
                            <select
                                className="w-full appearance-none bg-white border border-sage-200 text-ink-700 text-sm rounded-xl py-2.5 px-3 pr-8 focus:outline-none focus:border-indigo-400 focus:ring-2 focus:ring-indigo-100 transition-all font-bold shadow-sm"
                                value={selectedTableId}
                                onChange={(e) => setSelectedTableId(e.target.value)}
                                disabled={loading || tables.length === 0}
                            >
                                {tables.length === 0 && <option>暂无属性表</option>}
                                {tables.map(t => <option key={t.id} value={t.id}>{t.name}</option>)}
                            </select>
                            <Filter className="absolute right-3 top-1/2 -translate-y-1/2 text-sage-400 pointer-events-none group-hover:text-indigo-500 transition-colors" size={14} />
                        </div>
                    </div>

                    {/* Attribute Select */}
                    <div className="w-1/4 group">
                        <label className="text-xs font-bold text-sage-500 mb-1.5 block uppercase tracking-wide pl-1">属性</label>
                        <div className="relative">
                            <select
                                className="w-full appearance-none bg-white border border-sage-200 text-ink-700 text-sm rounded-xl py-2.5 px-3 pr-8 focus:outline-none focus:border-indigo-400 focus:ring-2 focus:ring-indigo-100 transition-all font-bold shadow-sm"
                                value={selectedAttribute}
                                onChange={(e) => setSelectedAttribute(e.target.value)}
                                disabled={availableAttributes.length === 0}
                            >
                                {availableAttributes.length === 0 && <option>-</option>}
                                {availableAttributes.map(a => <option key={a} value={a}>{a}</option>)}
                            </select>
                            <ChevronDown className="absolute right-3 top-1/2 -translate-y-1/2 text-sage-400 pointer-events-none group-hover:text-indigo-500 transition-colors" size={14} />
                        </div>
                    </div>

                    {/* Value Select */}
                    <div className="w-1/4 group">
                        <label className="text-xs font-bold text-sage-500 mb-1.5 block uppercase tracking-wide pl-1">条件</label>
                        <div className="relative">
                            <select
                                className="w-full appearance-none bg-white border border-sage-200 text-ink-700 text-sm rounded-xl py-2.5 px-3 pr-8 focus:outline-none focus:border-indigo-400 focus:ring-2 focus:ring-indigo-100 transition-all font-bold shadow-sm"
                                value={filterValue}
                                onChange={(e) => setFilterValue(e.target.value)}
                                disabled={availableValues.length === 0}
                            >
                                <option value="ALL">全部</option>
                                {availableValues.filter(v => v !== 'ALL').map(v => <option key={v} value={v}>{v}</option>)}
                            </select>
                            <Check className="absolute right-3 top-1/2 -translate-y-1/2 text-sage-400 pointer-events-none group-hover:text-indigo-500 transition-colors" size={14} />
                        </div>
                    </div>
                </div>

                {/* Main Display */}
                <div className="p-8 flex flex-col items-center justify-center bg-white/40">
                    <div className="relative mb-8 group cursor-pointer" onClick={toggleRollCall}>
                        {/* Outer Ring */}
                        <div className={`w-56 h-56 rounded-full border-8 flex items-center justify-center transition-all duration-300 ${isRunning ? 'border-indigo-200 shadow-[0_0_40px_rgba(99,102,241,0.2)]' : 'border-white shadow-inner bg-sage-50/50'}`}>
                            {/* Spinner Ring */}
                            {isRunning && (
                                <div className="absolute inset-0 rounded-full border-8 border-t-indigo-500 border-r-transparent border-b-indigo-500 border-l-transparent animate-spin-slow opacity-60"></div>
                            )}

                            {/* Central Text */}
                            <div className="w-44 h-44 rounded-full bg-white shadow-xl flex items-center justify-center relative z-10 border border-white/50">
                                <span className={`text-4xl font-black bg-clip-text text-transparent bg-gradient-to-br from-ink-800 to-ink-600 transition-all duration-100 px-2 text-center leading-tight ${isRunning ? 'scale-110 blur-[0.5px] opacity-70' : 'scale-100 opacity-100'}`}>
                                    {currentName}
                                </span>
                            </div>
                        </div>

                        {/* Count Badge */}
                        <div className="absolute top-0 right-0 bg-indigo-50 text-indigo-600 text-xs font-bold px-3 py-1.5 rounded-full border border-indigo-100 flex items-center gap-1.5 shadow-sm hover:scale-105 transition-transform">
                            <Users size={12} strokeWidth={2.5} />
                            {targetStudents.length}人 Available
                        </div>
                    </div>

                    <button
                        onClick={toggleRollCall}
                        disabled={targetStudents.length === 0}
                        className={`w-full max-w-xs py-4 rounded-2xl font-black text-white text-lg shadow-xl transition-all active:scale-95 flex items-center justify-center gap-2 transform hover:-translate-y-1 ${isRunning
                            ? 'bg-gradient-to-r from-clay-500 to-red-500 shadow-clay-200 ring-4 ring-clay-100'
                            : 'bg-gradient-to-r from-indigo-500 to-indigo-600 shadow-indigo-200 ring-4 ring-indigo-100'}`}
                    >
                        {isRunning ? (
                            <>STOP IT!</>
                        ) : (
                            <>START RANDOM</>
                        )}
                    </button>
                </div>

                {/* History */}
                {history.length > 0 && (
                    <div className="bg-sage-50/50 border-t border-sage-100/50 p-5 backdrop-blur-md">
                        <div className="flex items-center gap-2 text-xs font-bold text-sage-400 mb-3 uppercase tracking-wider">
                            <RotateCcw size={12} />
                            最近点名历史
                        </div>
                        <div className="flex gap-2 flex-wrap">
                            {history.map((name, i) => (
                                <span key={i} className="px-3 py-1.5 bg-white border border-sage-200 text-ink-600 text-xs font-bold rounded-full shadow-sm animate-in fade-in zoom-in duration-300">
                                    {name}
                                </span>
                            ))}
                        </div>
                    </div>
                )}
            </div>
        </div>
    );
};

export default RandomCallModal;
