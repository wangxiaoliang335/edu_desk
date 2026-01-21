import { useRef, useState, useEffect } from 'react';
import { X, Calendar, FileSpreadsheet, Maximize2, Minimize2, Loader2, RefreshCw, Users, Crown, Brush, Eraser, Trash2, LayoutGrid, CheckCircle } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import * as XLSX from 'xlsx';
import { useDraggable } from '../../hooks/useDraggable';

interface DutyRosterModalProps {
    isOpen: boolean;
    onClose: () => void;
    groupId?: string;
}

interface DutyRosterResponse {
    code: number;
    message: string;
    data: {
        rows: string[][];
        requirement_row_index: number;
    };
}

const WEEKDAYS = ['周日', '周一', '周二', '周三', '周四', '周五', '周六'];

const getRequirementContent = (row: string[]) => {
    const content = [];
    const parts = row[0] ? row[0].split(/[,，]/) : [];
    if (parts.length > 1) content.push(...parts.slice(1));
    for (let c = 1; c < row.length; c++) if (row[c]?.trim()) content.push(row[c].trim());
    return content.join(' ');
};

const DutyRosterModal = ({ isOpen, onClose, groupId }: DutyRosterModalProps) => {
    const [loading, setLoading] = useState(false);
    const [saving, setSaving] = useState(false);
    const [rows, setRows] = useState<string[][]>([]);
    const [requirementRowIndex, setRequirementRowIndex] = useState<number>(-1);
    const [isMinimalist, setIsMinimalist] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const { style, handleMouseDown } = useDraggable();

    useEffect(() => {
        if (isOpen && groupId) {
            fetchRoster();
        }
    }, [isOpen, groupId]);

    const fetchRoster = async () => {
        if (!groupId) return;
        setLoading(true);
        setError(null);
        try {
            const resStr = await invoke<string>('fetch_duty_roster', { groupId });
            const res: DutyRosterResponse = JSON.parse(resStr);
            if (res.data) {
                setRows(res.data.rows || []);
                setRequirementRowIndex(res.data.requirement_row_index);
            }
        } catch (e) {
            console.error(e);
            setError("加载值日表失败");
        } finally {
            setLoading(false);
        }
    };

    const fileInputRef = useRef<HTMLInputElement>(null);

    const handleImport = () => {
        if (fileInputRef.current) {
            fileInputRef.current.click();
        }
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        try {
            setLoading(true);
            const data = await file.arrayBuffer();
            const workbook = XLSX.read(data);
            const worksheetName = workbook.SheetNames[0];
            const worksheet = workbook.Sheets[worksheetName];
            const jsonData = XLSX.utils.sheet_to_json(worksheet, { header: 1 }) as any[][];

            if (!jsonData || jsonData.length === 0) {
                alert("Excel 文件为空或无法解析");
                return;
            }

            const newRows: string[][] = jsonData.map((row: any) => {
                const denseRow = Array.isArray(row) ? [...row] : [];
                return denseRow.map(cell => (cell === null || cell === undefined) ? "" : String(cell));
            });

            const maxCols = newRows.reduce((acc, row) => Math.max(acc, row.length), 0);
            const normalizedRows = newRows.map(row => {
                const arr = [...row];
                while (arr.length < maxCols) arr.push("");
                return arr;
            });

            if (confirm(`成功解析 ${normalizedRows.length} 行数据。是否覆盖当前值日表？`)) {
                setRows(normalizedRows);
                setSaving(true);
                try {
                    await invoke('save_duty_roster', {
                        groupId,
                        rows: normalizedRows,
                        requirementRowIndex: requirementRowIndex
                    });
                    alert("导入成功！");
                } catch (err) {
                    alert("导入后保存失败: " + String(err));
                } finally {
                    setSaving(false);
                }
            }
        } catch (error) {
            alert("导入失败: " + String(error));
        } finally {
            setLoading(false);
            if (fileInputRef.current) fileInputRef.current.value = "";
        }
    };

    const handleSave = async () => {
        if (!groupId) return;
        setSaving(true);
        try {
            await invoke('save_duty_roster', {
                groupId,
                rows,
                requirementRowIndex: requirementRowIndex
            });
        } catch (e) {
            console.error(e);
            alert("保存失败: " + String(e));
        } finally {
            setSaving(false);
        }
    };

    const handleCellChange = (rowIndex: number, colIndex: number, value: string) => {
        const newRows = [...rows];
        newRows[rowIndex] = [...newRows[rowIndex]];
        newRows[rowIndex][colIndex] = value;
        setRows(newRows);
    };

    const handleRequirementChange = (rowIndex: number, newValue: string) => {
        const newRows = [...rows];
        const currentRow = [...newRows[rowIndex]];

        // Preserve the label (e.g., "要求") from the first cell
        const label = currentRow[0]?.split(/[,，]/)[0] || "要求";

        // Update first cell to just be the label (remove any attached content)
        currentRow[0] = label;

        // Put the entire new content in the second cell
        currentRow[1] = newValue;

        // Clear remaining cells to avoid duplication
        for (let i = 2; i < currentRow.length; i++) {
            currentRow[i] = "";
        }

        newRows[rowIndex] = currentRow;
        setRows(newRows);
    };

    const isRequirementRow = (index: number) => {
        if (requirementRowIndex >= 0) return index === requirementRowIndex;
        // Fallback: check if first cell contains "要求"
        const row = rows[index];
        return row && row[0] && (row[0].includes('要求') || row[0].includes('备注'));
    };

    const getTodayColumnIndex = () => {
        const day = new Date().getDay();
        if (day >= 1 && day <= 5) return day;
        return -1;
    };

    const getTaskIcon = (taskName: string) => {
        if (taskName.includes('组长')) return <Crown size={24} className="text-amber-500" />;
        if (taskName.includes('扫') || taskName.includes('拖')) return <Brush size={24} className="text-blue-500" />;
        if (taskName.includes('黑板')) return <Eraser size={24} className="text-purple-500" />;
        if (taskName.includes('垃圾') || taskName.includes('水池')) return <Trash2 size={24} className="text-rose-500" />;
        if (taskName.includes('桌') || taskName.includes('凳')) return <LayoutGrid size={24} className="text-emerald-500" />;
        return <CheckCircle size={24} className="text-sage-500" />;
    };

    const getTaskColor = (taskName: string) => {
        if (taskName.includes('组长')) return 'bg-amber-50 group-hover:bg-amber-100 hover:shadow-amber-100/50 border-amber-100';
        if (taskName.includes('扫') || taskName.includes('拖')) return 'bg-blue-50 group-hover:bg-blue-100 hover:shadow-blue-100/50 border-blue-100';
        if (taskName.includes('黑板')) return 'bg-purple-50 group-hover:bg-purple-100 hover:shadow-purple-100/50 border-purple-100';
        if (taskName.includes('垃圾') || taskName.includes('水池')) return 'bg-rose-50 group-hover:bg-rose-100 hover:shadow-rose-100/50 border-rose-100';
        if (taskName.includes('桌') || taskName.includes('凳')) return 'bg-emerald-50 group-hover:bg-emerald-100 hover:shadow-emerald-100/50 border-emerald-100';
        return 'bg-sage-50 group-hover:bg-sage-100 hover:shadow-sage-100/50 border-sage-100';
    };

    const renderMinimalistView = () => {
        const todayCol = getTodayColumnIndex();

        if (todayCol === -1) {
            return (
                <div className="flex flex-col items-center justify-center h-full text-gray-400 gap-4">
                    <div className="w-20 h-20 bg-gray-50 rounded-full flex items-center justify-center mb-2">
                        <Calendar size={40} className="text-gray-300" />
                    </div>
                    <span className="font-bold text-lg text-gray-500">今天不是工作日</span>
                    <p className="text-sm text-gray-400">请切换到完整视图查看或编辑其他日期</p>
                </div>
            );
        }

        const taskList: { task: string; people: string[] }[] = [];

        for (let i = 1; i < rows.length; i++) {
            if (isRequirementRow(i)) continue;
            const row = rows[i];
            if (!row || row.length <= todayCol) continue;

            const taskName = row[0]?.trim();
            const person = row[todayCol]?.trim();

            if (taskName && person) {
                const lastTask = taskList[taskList.length - 1];
                if (lastTask && lastTask.task === taskName) {
                    if (!lastTask.people.includes(person)) {
                        lastTask.people.push(person);
                    }
                } else {
                    taskList.push({ task: taskName, people: [person] });
                }
            }
        }

        const requirementRow = rows.find((_, i) => isRequirementRow(i)) || (requirementRowIndex >= 0 ? rows[requirementRowIndex] : null);

        return (
            <div className="p-8 h-full overflow-y-auto custom-scrollbar">
                {/* Header */}
                <div className="flex items-center justify-between mb-8">
                    <div className="flex items-center gap-4">
                        <div className="w-14 h-14 bg-blue-500 rounded-2xl flex items-center justify-center shadow-lg shadow-blue-500/30 text-white">
                            <Users size={28} />
                        </div>
                        <div>
                            <h2 className="text-2xl font-extrabold text-gray-800 tracking-tight">今日值日</h2>
                            <p className="text-base text-gray-500 font-medium flex items-center gap-2">
                                <span className="w-2 h-2 rounded-full bg-green-500"></span>
                                {WEEKDAYS[new Date().getDay()]}
                            </p>
                        </div>
                    </div>
                    {/* Optional: Add a subtle date display here properly if needed */}
                </div>

                {/* Task Grid */}
                <div className="grid grid-cols-1 gap-4 mb-8">
                    {taskList.length === 0 ? (
                        <div className="flex flex-col items-center justify-center py-16 text-gray-400 bg-gray-50/50 rounded-3xl border border-dashed border-gray-200">
                            <Calendar size={48} className="mb-4 text-gray-300" />
                            <p className="font-medium text-lg">今日无值日安排</p>
                        </div>
                    ) : (
                        taskList.map((item, idx) => {
                            const isLeader = item.task.includes('组长');
                            const cardColorClass = getTaskColor(item.task);

                            return (
                                <div
                                    key={idx}
                                    className={`
                                        relative overflow-hidden group transition-all duration-300
                                        rounded-2xl border p-5
                                        ${cardColorClass}
                                        hover:-translate-y-1 hover:shadow-lg
                                    `}
                                >
                                    <div className="flex items-start gap-5 relative z-10">
                                        {/* Icon Box */}
                                        <div className="bg-white/80 backdrop-blur-sm p-3 rounded-xl shadow-sm border border-white/50 shrink-0">
                                            {getTaskIcon(item.task)}
                                        </div>

                                        {/* Content */}
                                        <div className="flex-1">
                                            <h3 className={`text-lg font-bold mb-3 ${isLeader ? 'text-amber-800' : 'text-gray-700'}`}>
                                                {item.task}
                                            </h3>

                                            <div className="flex flex-wrap gap-2">
                                                {item.people.map((p, pi) => (
                                                    <span
                                                        key={pi}
                                                        className={`
                                                            px-3 py-1.5 rounded-lg text-sm font-bold flex items-center gap-1.5 shadow-sm
                                                            ${isLeader
                                                                ? 'bg-amber-500 text-white shadow-amber-200'
                                                                : 'bg-white text-gray-700 border border-transparent shadow-gray-200'
                                                            }
                                                        `}
                                                    >
                                                        {isLeader && <Crown size={12} fill="currentColor" />}
                                                        {p}
                                                    </span>
                                                ))}
                                            </div>
                                        </div>
                                    </div>

                                    {/* Decorative background element */}
                                    <div className="absolute -right-6 -bottom-6 w-24 h-24 rounded-full bg-white/20 blur-xl group-hover:scale-150 transition-transform duration-500"></div>
                                </div>
                            );
                        })
                    )}
                </div>

                {/* Requirement Section */}
                {requirementRow && (
                    <div className="relative overflow-hidden rounded-2xl bg-gradient-to-br from-amber-50 to-orange-50 border border-amber-100 p-6 shadow-sm">
                        <div className="flex items-start gap-4 relative z-10">
                            <div className="bg-amber-100 p-2.5 rounded-xl text-amber-600 shrink-0 mt-0.5">
                                <FileSpreadsheet size={20} />
                            </div>
                            <div className="flex-1">
                                <h3 className="font-bold text-amber-800 text-lg mb-2 flex items-center gap-2">
                                    要求 & 备注
                                </h3>
                                <div className="text-sm text-amber-900/80 leading-relaxed font-medium whitespace-pre-wrap">
                                    {getRequirementContent(requirementRow) || "暂无特别要求"}
                                </div>
                            </div>
                        </div>
                        <div className="absolute top-0 right-0 w-32 h-32 bg-amber-200/20 rounded-full blur-3xl -mr-16 -mt-16"></div>
                    </div>
                )}
            </div>
        );
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl w-[950px] h-[650px] flex flex-col overflow-hidden border border-white/60 ring-1 ring-sage-100/50 animate-in zoom-in-95 duration-200"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="h-16 flex items-center justify-between px-6 bg-white/40 border-b border-sage-100/50 cursor-move select-none backdrop-blur-md"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 bg-gradient-to-br from-blue-400 to-blue-600 rounded-2xl flex items-center justify-center shadow-lg shadow-blue-500/20 text-white">
                            <Calendar size={20} />
                        </div>
                        <h3 className="font-bold text-ink-800 text-xl tracking-tight">值日表</h3>
                    </div>
                    <div className="flex items-center gap-3">
                        <input
                            type="file"
                            ref={fileInputRef}
                            onChange={handleFileChange}
                            accept=".xlsx, .xls, .csv"
                            className="hidden"
                        />
                        <button
                            className="px-4 py-2 bg-white border border-sage-200 hover:border-sage-400 text-ink-700 hover:text-sage-700 rounded-xl text-sm font-bold flex items-center gap-2 transition-all shadow-sm hover:shadow-md hover:-translate-y-0.5"
                            onClick={handleImport}
                        >
                            <FileSpreadsheet size={16} className="text-sage-500" />
                            导入 Excel
                        </button>
                        <button
                            className={`px-4 py-2 rounded-xl text-sm font-bold flex items-center gap-2 transition-all shadow-sm hover:shadow-md hover:-translate-y-0.5 ${isMinimalist
                                ? 'bg-sage-100 text-sage-700 border border-sage-200'
                                : 'bg-white text-ink-600 border border-sage-200 hover:text-sage-600'
                                }`}
                            onClick={() => setIsMinimalist(!isMinimalist)}
                        >
                            {isMinimalist ? <Maximize2 size={16} /> : <Minimize2 size={16} />}
                            {isMinimalist ? "完整视图" : "今日视图"}
                        </button>
                        <button
                            onClick={onClose}
                            className="ml-2 w-9 h-9 flex items-center justify-center hover:bg-clay-50 rounded-full text-sage-400 hover:text-clay-600 transition-all duration-300"
                        >
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-hidden relative bg-white/30 custom-scrollbar">
                    {loading ? (
                        <div className="absolute inset-0 flex flex-col items-center justify-center gap-3">
                            <span className="loading loading-spinner text-sage-500 loading-lg"></span>
                            <span className="text-sage-400 text-sm font-medium">加载中...</span>
                        </div>
                    ) : error ? (
                        <div className="absolute inset-0 flex flex-col items-center justify-center gap-4">
                            <span className="text-clay-500 font-bold bg-clay-50 px-4 py-2 rounded-xl border border-clay-100">{error}</span>
                            <button onClick={fetchRoster} className="flex items-center gap-2 text-white bg-sage-500 hover:bg-sage-600 px-4 py-2 rounded-xl font-bold shadow-lg shadow-sage-500/20 transition-all">
                                <RefreshCw size={16} />
                                重试
                            </button>
                        </div>
                    ) : isMinimalist ? (
                        renderMinimalistView()
                    ) : (
                        <div className="h-full flex flex-col">
                            <div className="flex-1 overflow-auto p-5 custom-scrollbar">
                                <div className="bg-white/80 backdrop-blur-xl rounded-2xl border border-white/60 ring-1 ring-sage-50 overflow-hidden shadow-sm">
                                    <table className="w-full border-collapse text-sm">
                                        <thead>
                                            <tr>
                                                {rows[0]?.map((header, idx) => {
                                                    const isToday = idx === getTodayColumnIndex();
                                                    return (
                                                        <th
                                                            key={idx}
                                                            className={`
                                                                p-4 text-left font-extrabold sticky top-0 z-20 backdrop-blur-md border-b border-sage-200/50
                                                                ${idx === 0 ? 'min-w-[120px] bg-sage-50/90 text-sage-800' : 'min-w-[100px]'}
                                                                ${isToday ? 'bg-blue-50/90 text-blue-700' : 'bg-white/90 text-gray-500'}
                                                            `}
                                                        >
                                                            <div className="flex items-center gap-2">
                                                                {header}
                                                                {isToday && (
                                                                    <span className="px-1.5 py-0.5 rounded-md bg-blue-100 text-blue-600 text-[10px] font-bold">
                                                                        今天
                                                                    </span>
                                                                )}
                                                            </div>
                                                        </th>
                                                    );
                                                })}
                                            </tr>
                                        </thead>
                                        <tbody>
                                            {rows.slice(1).map((row, rIdx) => {
                                                const actualRowIndex = rIdx + 1;
                                                const isReq = isRequirementRow(actualRowIndex);

                                                if (isReq) {
                                                    return (
                                                        <tr key={actualRowIndex} className="bg-gradient-to-r from-amber-50/50 to-orange-50/50">
                                                            <td className="border-b border-amber-100 p-4 font-bold text-amber-700 flex items-center gap-2">
                                                                <FileSpreadsheet size={16} />
                                                                {row[0]?.split(/[,，]/)[0] || "要求"}
                                                            </td>
                                                            <td
                                                                colSpan={Math.max(1, (rows[0]?.length || 1) - 1)}
                                                                className="border-b border-amber-100 p-0 relative group"
                                                            >
                                                                <textarea
                                                                    className="w-full h-full min-h-[80px] bg-transparent border-none outline-none p-4 text-amber-900/80 font-medium whitespace-pre-wrap break-all resize-none focus:bg-white/60 focus:ring-2 focus:ring-inset focus:ring-amber-200/50 transition-all placeholder:text-amber-800/30 leading-relaxed"
                                                                    value={getRequirementContent(row)}
                                                                    onChange={(e) => handleRequirementChange(actualRowIndex, e.target.value)}
                                                                    onBlur={handleSave}
                                                                    placeholder="在此输入值日要求..."
                                                                />
                                                                <div className="absolute right-2 top-2 opacity-0 group-hover:opacity-100 pointer-events-none transition-opacity">
                                                                    <Brush size={14} className="text-amber-400" />
                                                                </div>
                                                            </td>
                                                        </tr>
                                                    );
                                                }

                                                return (
                                                    <tr key={actualRowIndex} className="hover:bg-sage-50/30 transition-colors group">
                                                        {row.map((cell, cIdx) => {
                                                            const isToday = cIdx === getTodayColumnIndex();

                                                            // First Column: Task Name + Icon
                                                            if (cIdx === 0) {
                                                                return (
                                                                    <td key={cIdx} className="border-b border-sage-100/50 p-4 bg-sage-50/10 font-bold text-gray-700">
                                                                        <div className="flex items-center gap-3">
                                                                            <div className={`p-1.5 rounded-lg bg-white shadow-sm border border-gray-100 text-gray-400`}>
                                                                                {getTaskIcon(cell)}
                                                                            </div>
                                                                            {cell}
                                                                        </div>
                                                                    </td>
                                                                );
                                                            }

                                                            // Other Columns: Inputs
                                                            return (
                                                                <td
                                                                    key={cIdx}
                                                                    className={`
                                                                        border-b border-sage-100/50 p-0 relative transition-colors
                                                                        ${isToday ? 'bg-blue-50/30' : ''}
                                                                    `}
                                                                >
                                                                    <input
                                                                        className={`
                                                                            w-full h-full py-4 px-3 bg-transparent border-none outline-none 
                                                                            text-ink-600 font-medium text-center
                                                                            focus:bg-white focus:ring-2 focus:ring-inset focus:ring-blue-200 focus:text-blue-700 
                                                                            transition-all rounded-sm placeholder-gray-300
                                                                            ${cell ? 'font-semibold' : ''}
                                                                        `}
                                                                        value={cell}
                                                                        onChange={(e) => handleCellChange(actualRowIndex, cIdx, e.target.value)}
                                                                        onBlur={handleSave}
                                                                    />
                                                                    {/* Hover visual cue */}
                                                                    <div className="absolute inset-0 border-2 border-transparent group-hover/td:border-sage-200 pointer-events-none rounded-sm transition-colors"></div>
                                                                </td>
                                                            );
                                                        })}
                                                    </tr>
                                                );
                                            })}
                                        </tbody>
                                    </table>
                                </div>
                            </div>
                            <div className="h-12 bg-white/60 border-t border-sage-100/50 flex items-center px-6 text-xs text-sage-500 font-medium backdrop-blur-md gap-4">
                                {saving ? (
                                    <span className="flex items-center gap-2 text-sage-600 bg-sage-50 px-3 py-1 rounded-full animate-pulse border border-sage-100">
                                        <Loader2 size={12} className="animate-spin" />
                                        正在保存更改...
                                    </span>
                                ) : (
                                    <span className="flex items-center gap-2 opacity-70">
                                        <CheckCircle size={14} />
                                        所有更改将自动保存
                                    </span>
                                )}
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default DutyRosterModal;
