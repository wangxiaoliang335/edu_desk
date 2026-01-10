import { useRef, useState, useEffect } from 'react';
import { X, Calendar, Save, FileSpreadsheet, Maximize2, Minimize2, Loader2, RefreshCw } from 'lucide-react';
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

const DutyRosterModal = ({ isOpen, onClose, groupId }: DutyRosterModalProps) => {
    const [loading, setLoading] = useState(false);
    const [saving, setSaving] = useState(false);
    const [rows, setRows] = useState<string[][]>([]);
    const [requirementRowIndex, setRequirementRowIndex] = useState<number>(-1);
    const [isMinimalist, setIsMinimalist] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const { style, handleMouseDown } = useDraggable();

    // Initial Load
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
            if (res.data) { // Check if data exists as per Qt logic usually wrapping result
                // Note: The Qt code shows specific structure. Let's assume standard response wrapper if backend follows it.
                // Qt: dataObj.value("rows"), dataObj.value("requirement_row_index")
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

            // Normalize to string[][] and ensure valid strings, handling sparse arrays
            const newRows: string[][] = jsonData.map((row: any) => {
                // Spread first to fill holes with undefined, then map to string
                const denseRow = Array.isArray(row) ? [...row] : [];
                return denseRow.map(cell => (cell === null || cell === undefined) ? "" : String(cell));
            });

            // Pad each row to match standard column count
            const maxCols = newRows.reduce((acc, row) => Math.max(acc, row.length), 0);
            const normalizedRows = newRows.map(row => {
                const arr = [...row];
                while (arr.length < maxCols) arr.push("");
                return arr;
            });

            if (confirm(`成功解析 ${normalizedRows.length} 行数据。是否覆盖当前值日表？`)) {
                setRows(normalizedRows);
                // Trigger auto-save
                setSaving(true);
                try {
                    await invoke('save_duty_roster', {
                        groupId,
                        rows: normalizedRows,
                        requirementRowIndex: requirementRowIndex
                    });
                    console.log("导入并保存成功");
                    alert("导入成功！");
                } catch (err) {
                    console.error(err);
                    alert("导入后保存失败: " + String(err));
                } finally {
                    setSaving(false);
                }
            }
        } catch (error) {
            console.error("Import failed", error);
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
            // Maybe show toast? For now just silent success or log
            console.log("保存成功");
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

    const isRequirementRow = (index: number) => index === requirementRowIndex;

    // Helper to get today's column index (Monday=1, ..., Friday=5) based on Date
    const getTodayColumnIndex = () => {
        const day = new Date().getDay(); // 0=Sun, 1=Mon, ..., 6=Sat
        if (day >= 1 && day <= 5) return day;
        return -1;
    };

    // Minimalist View Logic (Task | People for Today)
    const renderMinimalistView = () => {
        const todayCol = getTodayColumnIndex(); // 1-based index usually if 0 is Task Name? 
        // Logic from Qt: 
        // 1st col (index 0) = Task Name
        // Headers are Row 0? Qt says: Row 0 is Headers (Date).
        // Format: Row 0: [Empty, Mon, Tue, Wed, Thu, Fri, ...]
        // Data Rows: [TaskName, PersonMon, PersonTue, ...]

        if (todayCol === -1) {
            return <div className="flex items-center justify-center h-full text-gray-500">今天不是工作日</div>;
        }

        const taskList: { task: string; people: string[] }[] = [];

        // Skip header row (0)
        for (let i = 1; i < rows.length; i++) {
            if (isRequirementRow(i)) continue;
            const row = rows[i];
            if (!row || row.length <= todayCol) continue;

            const taskName = row[0]?.trim();
            const person = row[todayCol]?.trim();

            if (taskName && person) {
                // Check if we should merge with previous task
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

        const requirementRow = requirementRowIndex >= 0 ? rows[requirementRowIndex] : null;

        return (
            <div className="space-y-4 p-8 bg-[#1e1e1e] text-white h-full overflow-y-auto">
                <div className="flex justify-between items-center mb-6">
                    <h2 className="text-2xl font-bold">今日值日 ({['周日', '周一', '周二', '周三', '周四', '周五', '周六'][new Date().getDay()]})</h2>
                </div>

                <div className="grid gap-4">
                    {taskList.length === 0 ? (
                        <div className="text-gray-400">今日无值日安排</div>
                    ) : (
                        taskList.map((item, idx) => (
                            <div key={idx} className="flex bg-[#282a2b] border border-[#3e3e3e] rounded-lg overflow-hidden">
                                <div className="w-32 p-4 bg-[#333] font-bold flex items-center justify-center border-r border-[#3e3e3e]">
                                    {item.task}
                                </div>
                                <div className="flex-1 p-4 flex items-center flex-wrap gap-2 text-lg">
                                    {item.people.map((p, pi) => (
                                        <span key={pi} className="bg-blue-600/30 px-3 py-1 rounded">{p}</span>
                                    ))}
                                </div>
                            </div>
                        ))
                    )}
                </div>

                {requirementRow && (
                    <div className="mt-8 p-4 bg-[#2d2d2d] rounded border border-[#3e3e3e]">
                        <h3 className="font-bold mb-2 text-yellow-500">要求 & 备注</h3>
                        <div className="text-sm opacity-80 whitespace-pre-wrap">
                            {/* Logic: Join all columns except first if they have content, or handle first column splitting if comma separated */}
                            {(() => {
                                const content = [];
                                // Check first col for comma
                                if (requirementRow[0] && requirementRow[0].includes(',')) {
                                    const parts = requirementRow[0].split(',');
                                    content.push(...parts.slice(1).map(s => s.trim()).filter(s => s));
                                }
                                // Other cols
                                for (let c = 1; c < requirementRow.length; c++) {
                                    if (requirementRow[c]?.trim()) content.push(requirementRow[c].trim());
                                }
                                return content.join(' ');
                            })()}
                        </div>
                    </div>
                )}
            </div>
        );
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/60 backdrop-blur-sm animate-in fade-in duration-200">
            {/* Dark Theme Container to match Qt */}
            <div
                style={style}
                className="bg-[#282a2b] text-white rounded-lg shadow-2xl w-[1000px] h-[700px] flex flex-col overflow-hidden border border-[#444]"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="h-12 flex items-center justify-between px-4 bg-[#333] border-b border-[#444] cursor-move select-none"
                >
                    <div className="flex items-center gap-2 font-bold select-none">
                        <Calendar size={18} className="text-blue-400" />
                        <span>值日表</span>
                    </div>
                    <div className="flex items-center gap-2">
                        <button
                            className="px-3 py-1.5 bg-blue-600 hover:bg-blue-500 rounded text-sm font-bold flex items-center gap-1 transition-colors"
                            onClick={handleImport}
                        >
                            <FileSpreadsheet size={14} /> 导入
                        </button>
                        {/* Hidden Input */}
                        <input
                            type="file"
                            ref={fileInputRef}
                            onChange={handleFileChange}
                            accept=".xlsx, .xls, .csv"
                            className="hidden"
                        />
                        <button
                            className="px-3 py-1.5 bg-blue-600 hover:bg-blue-500 rounded text-sm font-bold flex items-center gap-1 transition-colors"
                            onClick={() => setIsMinimalist(!isMinimalist)}
                        >
                            {isMinimalist ? <Maximize2 size={14} /> : <Minimize2 size={14} />}
                            {isMinimalist ? "完整" : "极简"}
                        </button>
                        <button onClick={onClose} className="ml-2 p-1.5 hover:bg-red-500 rounded-full text-gray-400 hover:text-white transition-colors">
                            <X size={18} />
                        </button>
                    </div>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-hidden relative">
                    {loading ? (
                        <div className="absolute inset-0 flex items-center justify-center">
                            <Loader2 className="animate-spin text-blue-500" size={32} />
                        </div>
                    ) : error ? (
                        <div className="absolute inset-0 flex flex-col items-center justify-center gap-2">
                            <span className="text-red-400">{error}</span>
                            <button onClick={fetchRoster} className="flex items-center gap-1 text-blue-400 hover:underline">
                                <RefreshCw size={14} /> 重试
                            </button>
                        </div>
                    ) : isMinimalist ? (
                        renderMinimalistView()
                    ) : (
                        <div className="h-full flex flex-col">
                            <div className="flex-1 overflow-auto bg-[#1E1E1E] p-1">
                                <table className="w-full border-collapse text-sm">
                                    <thead>
                                        <tr>
                                            {rows[0]?.map((header, idx) => (
                                                <th key={idx} className="bg-[#282A2B] border border-[#3E3E3E] p-2 text-left sticky top-0 z-10 min-w-[100px]">
                                                    {header}
                                                </th>
                                            ))}
                                        </tr>
                                    </thead>
                                    <tbody>
                                        {rows.slice(1).map((row, rIdx) => {
                                            const actualRowIndex = rIdx + 1;
                                            const isReq = isRequirementRow(actualRowIndex);

                                            // Handle Requirement Row Spanning
                                            if (isReq) {
                                                return (
                                                    <tr key={actualRowIndex} className="bg-[#282a2b]/50">
                                                        {/* First Cell: "要求" etc */}
                                                        <td className="border border-[#3E3E3E] p-2 font-bold text-yellow-500">
                                                            {row[0]?.split(',')[0] || "要求"}
                                                        </td>
                                                        {/* Spanned Cell for content */}
                                                        <td colSpan={Math.max(1, (rows[0]?.length || 1) - 1)} className="border border-[#3E3E3E] p-2 italic text-gray-300">
                                                            {(() => {
                                                                const content = [];
                                                                if (row[0] && row[0].includes(',')) content.push(...row[0].split(',').slice(1));
                                                                for (let c = 1; c < row.length; c++) if (row[c]?.trim()) content.push(row[c].trim());
                                                                return content.join(' ');
                                                            })()}
                                                        </td>
                                                    </tr>
                                                );
                                            }

                                            return (
                                                <tr key={actualRowIndex} className="hover:bg-[#333]">
                                                    {row.map((cell, cIdx) => (
                                                        <td key={cIdx} className="border border-[#3E3E3E] p-0">
                                                            <input
                                                                className="w-full h-full bg-transparent border-none outline-none p-2 text-white focus:bg-[#4169E1] transition-colors"
                                                                value={cell}
                                                                onChange={(e) => handleCellChange(actualRowIndex, cIdx, e.target.value)}
                                                                onBlur={handleSave} // Auto-save on blur
                                                            />
                                                        </td>
                                                    ))}
                                                </tr>
                                            );
                                        })}
                                    </tbody>
                                </table>
                            </div>
                            <div className="h-8 bg-[#333] border-t border-[#444] flex items-center px-4 text-xs text-gray-400">
                                {saving ? <span className="flex items-center gap-1 text-blue-400"><Loader2 size={10} className="animate-spin" /> 保存中...</span> : "所有更改将自动保存"}
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default DutyRosterModal;
