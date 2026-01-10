import { useState, useEffect } from 'react';

interface StudentPhysiqueModalProps {
    isOpen: boolean;
    onClose: () => void;
    fileName: string;
    data: any[]; // Array of Arrays
    classId?: string;
}

interface TableRow {
    _id: number;
    [key: string]: any;
}

const StudentPhysiqueModal = ({ isOpen, onClose, fileName, data, classId }: StudentPhysiqueModalProps) => {
    const [headers, setHeaders] = useState<string[]>([]);
    const [rows, setRows] = useState<TableRow[]>([]);

    useEffect(() => {
        if (data && data.length > 0) {
            const rawHeaders = (data[0] as any[]).map(h => String(h).trim());
            console.log("Physique Raw Headers:", rawHeaders);

            let processedHeaders = [...rawHeaders];
            if (!processedHeaders.includes('总分')) processedHeaders.push('总分');
            if (!processedHeaders.includes('小组总分')) processedHeaders.push('小组总分');

            // Process rows
            // Logic to fill empty group names with previous row's group if empty (common in excel)
            let lastGroup = "";
            const groupColIdx = rawHeaders.indexOf('小组');

            const processedRows = data.slice(1).map((row, idx) => {
                const rowObj: TableRow = { _id: idx };

                // Handle Group Fill
                let groupVal = groupColIdx >= 0 ? row[groupColIdx] : "";
                if (groupVal) {
                    lastGroup = groupVal;
                } else if (lastGroup && (!groupVal || groupVal.trim() === '')) {
                    // If current group is empty but we have a last group, fill it? 
                    // QT logic: "if group exists... if empty use currentGroup".
                    rowObj['小组'] = lastGroup;
                }

                rawHeaders.forEach((h, i) => {
                    if (h === '小组' && rowObj['小组']) return; // Already set
                    rowObj[h] = row[i];
                });

                // Ensure '小组' is set in rowObj if it wasn't in loop
                if (groupColIdx >= 0 && !rowObj['小组'] && lastGroup) {
                    rowObj['小组'] = lastGroup;
                }

                return rowObj;
            });

            setHeaders(processedHeaders);
            setRows(processedRows);
        }
    }, [data]);

    // Calculate Row Spans for Group Column
    const getGroupRowSpan = (rowIndex: number): number => {
        const currentRow = rows[rowIndex];
        const currentGroup = currentRow['小组'];
        if (!currentGroup) return 1;

        // Check if previous row has same group
        if (rowIndex > 0 && rows[rowIndex - 1]['小组'] === currentGroup) {
            return 0; // Already spanned
        }

        // Count following rows
        let span = 1;
        for (let i = rowIndex + 1; i < rows.length; i++) {
            if (rows[i]['小组'] === currentGroup) {
                span++;
            } else {
                break;
            }
        }
        return span;
    };

    const handleValueClick = (rowId: number, colKey: string, currentValue: any) => {
        // Prevent editing Name/ID/Total/Group directly via this simple prompt
        if (['总分', '小组总分'].includes(colKey)) return;

        const newValue = prompt(`请输入 ${colKey} 的新值:`, currentValue || '');
        if (newValue !== null) {
            setRows(prev => prev.map(r => {
                if (r._id === rowId) {
                    return { ...r, [colKey]: newValue };
                }
                return r;
            }));
            // TODO: Recalculate totals
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[70] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-[#808080] rounded shadow-2xl w-[1200px] h-[800px] flex flex-col overflow-hidden text-sm select-none relative">

                {/* Close */}
                <button
                    onClick={onClose}
                    className="absolute top-2 right-2 w-8 h-8 flex items-center justify-center bg-[#666666] text-white rounded hover:bg-[#777777] font-bold z-20 border border-gray-500"
                >
                    X
                </button>

                {/* Header */}
                <div className="p-4 flex flex-col items-center justify-center gap-2 pt-10">
                    <div className="bg-[#add8e6] text-black px-8 py-2 rounded font-bold text-lg shadow-sm">
                        {fileName} (小组表)
                    </div>
                </div>

                {/* Controls (Green Buttons) */}
                <div className="px-4 py-2 flex gap-2 overflow-x-auto justify-start border-b border-gray-600 pb-4">
                    {['添加行', '删除行', '删除列', '添加列', '导出', '上传服务器'].map(btn => (
                        <button key={btn} className={`px-4 py-1.5 rounded text-white text-xs font-bold whitespace-nowrap shadow-sm ${btn === '上传服务器' ? 'bg-blue-600 hover:bg-blue-700' : 'bg-green-700 hover:bg-green-800'}`}>
                            {btn}
                        </button>
                    ))}
                </div>

                {/* Description */}
                <div className="px-4 py-2">
                    <div className="font-bold text-black mb-1">说明:</div>
                    <div className="bg-[#ffa500] text-black p-2 border border-gray-500 text-xs shadow-inner">
                        说明:该表为小组积分表。包含以下评分项: {headers.filter(h => !['小组', '学号', '姓名', '总分', '小组总分'].includes(h)).join('、')}
                    </div>
                </div>

                {/* Table Area */}
                <div className="flex-1 overflow-auto bg-white mx-4 mb-4 border border-gray-600 relative shadow-inner">
                    <table className="w-full border-collapse">
                        <thead className="bg-[#f0f0f0] sticky top-0 z-10 shadow-sm">
                            <tr>
                                <th className="w-10 border border-gray-400 p-1 bg-[#e0e0e0]"></th>
                                {headers.map(h => (
                                    <th key={h} className="border border-gray-400 p-2 text-gray-800 min-w-[80px] font-bold bg-[#e0e0e0]">
                                        {h}
                                    </th>
                                ))}
                            </tr>
                        </thead>
                        <tbody>
                            {rows.map((row, idx) => {
                                const groupSpan = getGroupRowSpan(idx);

                                return (
                                    <tr key={row._id} className="hover:bg-blue-50">
                                        <td className="border border-gray-300 bg-[#f8f8f8] text-center text-gray-500 text-xs font-mono select-none">{idx + 1}</td>

                                        {headers.map(col => {
                                            const val = row[col];

                                            // Handle Group Column Merge
                                            if (col === '小组') {
                                                if (groupSpan === 0) return null; // Skip rendered cell
                                                return (
                                                    <td key={col} rowSpan={groupSpan} className="border border-gray-300 p-1 text-center bg-white align-middle font-bold text-gray-800">
                                                        {val}
                                                    </td>
                                                );
                                            }

                                            const isFixed = ['学号', '姓名', '小组'].includes(col);
                                            const isTotal = ['总分', '小组总分'].includes(col);
                                            const isScore = !isFixed && !isTotal;

                                            return (
                                                <td key={col} className="border border-gray-300 p-1 text-center">
                                                    {isScore ? (
                                                        <div className="flex items-center justify-center gap-1">
                                                            <button
                                                                onClick={() => handleValueClick(row._id, col, val)}
                                                                className="min-w-[40px] px-2 py-1 bg-green-600 text-white rounded hover:bg-green-500 transition-colors font-medium cursor-pointer border-2 border-yellow-300 shadow-sm"
                                                            >
                                                                {val || '0'}
                                                            </button>
                                                            <button className="px-1.5 py-1 bg-green-600 text-white rounded hover:bg-green-500 text-[10px] border-none shadow-sm">
                                                                注
                                                            </button>
                                                        </div>
                                                    ) : isTotal ? (
                                                        <div className="min-w-[40px] px-2 py-1 bg-[#4a4a4a] text-[#dddddd] rounded border border-yellow-400 font-medium inline-block shadow-sm">
                                                            {val || '0'}
                                                        </div>
                                                    ) : (
                                                        <span className="text-gray-800 font-medium">{val}</span>
                                                    )}
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
        </div>
    );
};
export default StudentPhysiqueModal;
