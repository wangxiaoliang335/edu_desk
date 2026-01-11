import { useState, useRef, useEffect } from 'react';
import { X, FileSpreadsheet, Plus, Trash2 } from 'lucide-react';
import * as XLSX from 'xlsx';
import MidtermGradeModal from './MidtermGradeModal';
import StudentPhysiqueModal from './StudentPhysiqueModal';

// Define the shape of the data rows we might parse (generic)
// type SheetRow = (string | number | undefined)[];

interface CustomListModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
}

interface ImportedFile {
    id: string;
    name: string; // Filename
    type: 'midterm_grade' | 'student_physique' | 'unknown';
    data: any[]; // Raw JSON data from xlsx
    headers: string[];
    file?: File; // Raw file object for upload
}

const CustomListModal = ({ isOpen, onClose, classId }: CustomListModalProps) => {
    const [importedFiles, setImportedFiles] = useState<ImportedFile[]>([]);
    // const [selectedFileId, setSelectedFileId] = useState<string | null>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);

    // Sub-modal states
    const [showMidtermModal, setShowMidtermModal] = useState(false);
    const [showPhysiqueModal, setShowPhysiqueModal] = useState(false);
    const [currentData, setCurrentData] = useState<ImportedFile | null>(null);

    useEffect(() => {
        if (isOpen && classId) {
            fetchServerFiles();
        }
    }, [isOpen, classId]);

    const fetchServerFiles = async () => {
        if (!classId) return;

        // QT compatibility: remove '01' suffix for fetching as well
        let targetClassId = classId;
        if (targetClassId.length === 11) {
            targetClassId = targetClassId.substring(0, 9);
        }

        try {
            // 1. Fetch Student Scores (Midterm Grade)
            // The API doc says getting without term returns a list of headers (tables).
            const resScore = await fetch(`/api/student-scores?class_id=${targetClassId}`);
            const jsonScore = await resScore.json();
            console.log("Student Scores Response:", jsonScore); // Log response

            let serverFiles: ImportedFile[] = [];

            if (jsonScore.code === 200 && jsonScore.data && jsonScore.data.headers) {
                // Determine if it is a list or single object
                const list = Array.isArray(jsonScore.data.headers) ? jsonScore.data.headers : [jsonScore.data.headers];

                list.forEach((item: any) => {
                    // Check if 'excel_files' or 'excel_file_url' exists (QT / Backend format difference?)
                    // Log shows 'excel_file_url'
                    const filesList = (item.excel_file_url && Array.isArray(item.excel_file_url))
                        ? item.excel_file_url
                        : ((item.excel_files && Array.isArray(item.excel_files)) ? item.excel_files : []);

                    if (filesList.length > 0) {
                        // Iterate over each excel file in the list
                        filesList.forEach((ef: any, idx: number) => {
                            // ef.fields -> ["总分", "数学"...]
                            // ef.fields -> ["总分", "数学"...]
                            const rawFileFields = ef.fields || [];
                            // Filter out "总分" to avoid duplication (as we append it manually)
                            const fileFields = rawFileFields.filter((f: any) => {
                                const fname = typeof f === 'string' ? f : f.field_name;
                                return fname !== '总分';
                            });
                            const tableHeaders = ['学号', '姓名', ...fileFields, '总分'];

                            // Construct Rows
                            const tableRows = item.scores.map((s: any) => {
                                const row: any[] = [];
                                row.push(s.student_id);
                                row.push(s.student_name);
                                fileFields.forEach((f: any) => {
                                    const fname = typeof f === 'string' ? f : f.field_name;
                                    // Try specific key first: "Field_Filename.xlsx"
                                    // Log shows: "数学_期中成绩单.xlsx"
                                    const specificKey = `${fname}_${ef.filename}`;

                                    const val = s.scores_json_full?.[specificKey] ??
                                        s.scores_json_full?.[fname] ??
                                        s[fname] ?? "";
                                    row.push(val);
                                });
                                // Total might be specific too? 
                                // "总分_期中成绩单.xlsx"
                                const totalKey = `总分_${ef.filename}`;
                                const totalVal = s.scores_json_full?.[totalKey] ?? s.total_score ?? "";
                                row.push(totalVal);
                                // Parsing comments
                                // s.comments -> {"数学_期中成绩单2": "...", ...}
                                if (s.comments && Object.keys(s.comments).length > 0) {

                                    const fileBaseName = ef.filename ? ef.filename.replace(/\.[^/.]+$/, "") : "";

                                    Object.keys(s.comments).forEach(commentKey => {
                                        fileFields.forEach((f: any) => {
                                            const fname = typeof f === 'string' ? f : f.field_name;

                                            // Construct expected key for this specific file
                                            // e.g. "数学_期中成绩单2" (Basename)
                                            const expectedKeyWithFile = `${fname}_${fileBaseName}`;
                                            // e.g. "数学_期中成绩单.xlsx" (Fullname)
                                            const expectedKeyWithFullFilename = `${fname}_${ef.filename}`;

                                            // Conditions:
                                            // 1. Exact match with filename suffix (Preferred)
                                            // 2. Exact match with full filename suffix
                                            // 3. Exact match with field name (Legacy/Global)

                                            const isMatch = (commentKey === expectedKeyWithFile) ||
                                                (commentKey === expectedKeyWithFullFilename) ||
                                                (commentKey === fname);

                                            if (isMatch) {
                                                (row as any)[`_comment_${fname}`] = s.comments[commentKey];
                                            } else {
                                                // Optional: Log misses if needed
                                                // if (commentKey.startsWith(fname)) console.log(`[Debug] Partial match ignored: ${commentKey} vs ${expectedKeyWithFile}`);
                                            }
                                        });
                                    });
                                }

                                return row;
                            });

                            const fullData = [tableHeaders, ...tableRows];
                            serverFiles.push({
                                id: `server_score_${item.id}_${idx}`,
                                name: ef.filename || (item.exam_name || "未命名"),
                                type: 'midterm_grade',
                                data: fullData,
                                headers: tableHeaders
                            });
                        });
                    } else {
                        // Fallback for legacy data or if excel_files is empty (use main item fields)
                        // Fallback for legacy data or if excel_files is empty (use main item fields)
                        const rawItemFields = item.fields || [];
                        const itemFields = rawItemFields.filter((f: any) => {
                            const fname = typeof f === 'string' ? f : f.field_name;
                            return fname !== '总分';
                        });
                        const tableHeaders = ['学号', '姓名', ...itemFields.map((f: any) => typeof f === 'string' ? f : f.field_name), '总分'];

                        const tableRows = item.scores.map((s: any) => {
                            const row: any[] = [];
                            row.push(s.student_id);
                            row.push(s.student_name);
                            itemFields.forEach((f: any) => {
                                const fname = typeof f === 'string' ? f : f.field_name;
                                const val = s.scores_json_full?.[fname] ?? s[fname] ?? "";
                                row.push(val);
                            });
                            row.push(s.total_score);

                            // Parse comments for legacy/single path
                            if (s.comments) {
                                Object.keys(s.comments).forEach(commentKey => {
                                    itemFields.forEach((f: any) => {
                                        const fname = typeof f === 'string' ? f : f.field_name;
                                        // Same improved heuristic
                                        if (commentKey === fname || commentKey.startsWith(`${fname}_`)) {
                                            (row as any)[`_comment_${fname}`] = s.comments[commentKey];
                                        }
                                    });
                                });
                            }
                            return row;
                        });

                        const fullData = [tableHeaders, ...tableRows];
                        serverFiles.push({
                            id: `server_score_${item.id}`,
                            name: item.exam_name || item.excel_file_name || "未命名成绩表",
                            type: 'midterm_grade',
                            data: fullData,
                            headers: tableHeaders
                        });
                    }
                });
            }

            const calculateTerm = () => {
                const now = new Date();
                const month = now.getMonth() + 1;
                const year = now.getFullYear();
                // Jan is part of previous year's Term 1
                if (month >= 8 || month <= 1) {
                    const startYear = month >= 8 ? year : year - 1;
                    return `${startYear}-${startYear + 1}-1`;
                } else {
                    return `${year - 1}-${year}-2`;
                }
            };

            // 2. Fetch Group Scores
            // Try to use the term from the student scores if available, otherwise calculate default
            let termParam = "";
            if (jsonScore.code === 200 && jsonScore.data && jsonScore.data.headers) {
                const list = Array.isArray(jsonScore.data.headers) ? jsonScore.data.headers : [jsonScore.data.headers];
                if (list.length > 0 && list[0].term) {
                    termParam = `&term=${list[0].term}`;
                }
            }

            if (!termParam) {
                termParam = `&term=${calculateTerm()}`;
            }

            console.log(`Fetching group scores with class_id=${targetClassId}${termParam}`);
            const resGroup = await fetch(`/api/group-scores?class_id=${targetClassId}${termParam}`);
            const jsonGroup = await resGroup.json();
            console.log("Group Scores Response Full:", JSON.stringify(jsonGroup, null, 2));

            if (jsonGroup.code === 200 && jsonGroup.data) {
                // The Doc 2.2 says response data has "header" (singular) and "group_scores".
                // This implies it returns ONE table if term is inferred (NULL).
                // Does it return a list if multiple? "message: 查询成功"
                // If it returns a list, data might be array?
                // Let's handle both.
                const groupList = Array.isArray(jsonGroup.data) ? jsonGroup.data : (jsonGroup.data.header ? [jsonGroup.data] : []);
                console.log("Processed Group List:", groupList);

                groupList.forEach((tableData: any) => {
                    const headerInfo = tableData.header || {};
                    const groupScores = tableData.group_scores || [];

                    // Check if we have multiple files merged in this record
                    const fileUrls = headerInfo.excel_file_url;

                    if (fileUrls && typeof fileUrls === 'object' && Object.keys(fileUrls).length > 0) {
                        // Iterate over each file (e.g. "小组管理表.xlsx", "学生小组体质统计表.xlsx")
                        Object.keys(fileUrls).forEach((filename, fIdx) => {
                            const fileInfo = fileUrls[filename];
                            const rawFields = fileInfo.fields || []; // ["语文", "数学"...]

                            // Filter fields
                            const displayFields = rawFields.filter((f: string) => !['总分', '小组总分'].includes(f));
                            const tableHeaders = ['小组', '学号', '姓名', ...displayFields, '总分', '小组总分'];

                            // Build Rows using specific file fields
                            let tableRows: any[] = [];
                            groupScores.forEach((g: any) => {
                                if (g.students) {
                                    g.students.forEach((s: any) => {
                                        const row = [];
                                        row.push(g.group_name);
                                        row.push(s.student_id);
                                        row.push(s.student_name);

                                        // Dynamic scores
                                        displayFields.forEach((field: string) => {
                                            // Construct keys: "Field_Filename.xlsx"
                                            const specificKey = `${field}_${filename}`;
                                            const val = s.scores_json_full?.[specificKey] ??
                                                s.scores_json_full?.[field] ?? "";

                                            row.push(val);
                                        });

                                        // Totals
                                        // Try to find specific total if exists, else use global
                                        // But usually global total is sum of all? 
                                        // Wait, the LOG shows specific fields include "总分".
                                        // "scores_json_full": { "数学_小组管理表.xlsx": 89.4 ... }
                                        // Does it have "总分_小组管理表.xlsx"?
                                        // If not, we might have to sum it ourselves or just use global s.total_score (which is sum of ALL files).
                                        // For clarity, let's use global total for now, or re-calculate?
                                        // Re-calculation in Modal will happen anyway.
                                        // Let's passed s.total_score but user might confuse it if it includes other tables.
                                        // Ideally we sum the visible fields.
                                        // For now, let's just pass 0 or existing total. The Modal recalculates totals based on columns.
                                        row.push(s.total_score);
                                        row.push(s.group_total_score);

                                        // Comments
                                        if (s.comments_json_full) {
                                            const fileBaseName = filename ? filename.replace(/\.[^/.]+$/, "") : "";
                                            Object.keys(s.comments_json_full).forEach(commentKey => {
                                                displayFields.forEach((fname: string) => {
                                                    const expectedKeyWithFile = `${fname}_${fileBaseName}`;
                                                    const expectedKeyWithFullFilename = `${fname}_${filename}`;

                                                    const isMatch = (commentKey === expectedKeyWithFile) ||
                                                        (commentKey === expectedKeyWithFullFilename) ||
                                                        (commentKey === fname);

                                                    if (isMatch) {
                                                        (row as any)[`_comment_${fname}`] = s.comments_json_full[commentKey];
                                                    }
                                                });
                                            });
                                        }

                                        tableRows.push(row);
                                    });
                                }
                            });

                            const fullData = [tableHeaders, ...tableRows];
                            serverFiles.push({
                                id: `server_group_${headerInfo.id}_${fIdx}`,
                                name: filename || "未命名小组表", // Display name with extension to match server requirement
                                type: 'student_physique',
                                data: fullData,
                                headers: tableHeaders
                            });
                        });

                    } else {
                        // Legacy / Single file fallback
                        let inferredFields: string[] = [];
                        if (groupScores?.[0]?.students?.[0]?.scores_json_full) {
                            inferredFields = Object.keys(groupScores[0].students[0].scores_json_full).filter(k => !k.includes("_"));
                        }

                        const rawDynamicFields = (headerInfo.fields?.map((f: any) => typeof f === 'string' ? f : f.field_name) || inferredFields);
                        const dynamicFields = rawDynamicFields.filter((f: string) => !['总分', '小组总分'].includes(f));

                        const tableHeaders = ['小组', '学号', '姓名', ...dynamicFields, '总分', '小组总分'];
                        let tableRows: any[] = [];
                        if (groupScores) {
                            groupScores.forEach((g: any) => {
                                if (g.students) {
                                    g.students.forEach((s: any) => {
                                        const row = [];
                                        row.push(g.group_name);
                                        row.push(s.student_id);
                                        row.push(s.student_name);
                                        dynamicFields.forEach((fname: string) => {
                                            const val = s.scores_json_full?.[fname] ?? "";
                                            row.push(val);
                                        });
                                        row.push(s.total_score);
                                        row.push(s.group_total_score);
                                        tableRows.push(row);
                                    });
                                }
                            });
                        }

                        const fullData = [tableHeaders, ...tableRows];
                        serverFiles.push({
                            id: `server_group_${headerInfo.id || Date.now()}`,
                            name: headerInfo.exam_name || "未命名小组表",
                            type: 'student_physique',
                            data: fullData,
                            headers: tableHeaders
                        });
                    }
                });
            }

            // Merge with existing local files (if any), avoiding duplicates by name? 
            // For now, just replace or append? User might have just imported something locally.
            // Let's filter out server files from current state (if we re-fetch) and append.
            // Actually, simplest is to just append unique ones or overwrite.
            // Given "login again", we assume empty start.

            // To be safe, we just set them.
            setImportedFiles(prev => {
                const newFiles = [...prev];
                serverFiles.forEach(sf => {
                    if (!newFiles.find(f => f.name === sf.name && f.type === sf.type)) {
                        newFiles.push(sf);
                    }
                });
                return newFiles;
            });

        } catch (e) {
            console.error("Failed to fetch server files", e);
        }
    };


    // State to track if we should prompt for upload (because it was just imported)
    const [autoPromptFileId, setAutoPromptFileId] = useState<string | null>(null);

    const handleFileSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (evt) => {
            const bstr = evt.target?.result;
            const wb = XLSX.read(bstr, { type: 'binary' });

            // Assume first sheet
            const wsname = wb.SheetNames[0];
            const ws = wb.Sheets[wsname];

            // Get raw data as array of arrays to check headers
            const rawData = XLSX.utils.sheet_to_json<any>(ws, { header: 1 });
            if (rawData.length === 0) return;

            const headers = (rawData[0] as string[]).map(h => String(h).trim());
            // const dataRows = rawData.slice(1); // Not used for type detection but stored

            // Determine type
            let fileType: ImportedFile['type'] = 'unknown';

            const hasId = headers.includes('学号');
            const hasName = headers.includes('姓名');
            const hasGroup = headers.includes('小组');

            if (hasGroup) {
                // Student Group Attribute Table -> StudentPhysiqueModal
                fileType = 'student_physique';
            } else if (hasId && hasName) {
                // Student Attribute Table (No Group) -> MidtermGradeModal
                fileType = 'midterm_grade';
            }

            const newFile: ImportedFile = {
                id: Date.now().toString(),
                name: file.name.replace(/\.[^/.]+$/, ""), // Remove extension
                type: fileType,
                data: rawData, // Store all raw data including header for now, or just dataRows? 
                // Let's store rawData (AoA) to be flexible
                headers: headers,
                file: file
            };

            setImportedFiles(prev => [...prev, newFile]);

            // Auto-open and prompt
            setCurrentData(newFile);
            setAutoPromptFileId(newFile.id);
            if (fileType === 'midterm_grade') {
                setShowMidtermModal(true);
            } else if (fileType === 'student_physique') {
                setShowPhysiqueModal(true);
            } else {
                alert("导入成功，但无法识别表格类型，请手动查看。");
            }
        };
        reader.readAsBinaryString(file);

        // Reset input
        if (fileInputRef.current) fileInputRef.current.value = "";
    };

    const handleFileClick = (file: ImportedFile) => {
        setCurrentData(file);
        if (file.type === 'midterm_grade') {
            setShowMidtermModal(true);
        } else if (file.type === 'student_physique') {
            setShowPhysiqueModal(true);
        } else {
            alert("无法识别的表格类型，或者是未知格式");
        }
    };

    const deleteFile = (e: React.MouseEvent, id: string) => {
        e.stopPropagation();
        setImportedFiles(prev => prev.filter(f => f.id !== id));
    };

    if (!isOpen) return null;

    const handleFileUpdate = (fileId: string, newData: any[]) => {
        setImportedFiles(prev => prev.map(f => {
            if (f.id === fileId) {
                return { ...f, data: newData };
            }
            return f;
        }));
        // Also update currentData if it's the one being edited, to keep them in sync immediately
        if (currentData && currentData.id === fileId) {
            setCurrentData(prev => prev ? { ...prev, data: newData } : null);
        }
    };

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-[#808080] rounded shadow-2xl w-[600px] h-[400px] flex flex-col overflow-hidden text-sm">

                {/* Header */}
                <div className="flex items-center justify-between p-2">
                    <div className="bg-[#d3d3d3] text-black px-4 py-2 rounded font-bold text-base">
                        学生统计表导入
                    </div>
                    <button onClick={onClose} className="w-8 h-8 flex items-center justify-center bg-[#666666] text-white rounded hover:bg-[#777777] font-bold">
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 p-4 flex flex-wrap content-start gap-4 overflow-y-auto">
                    {importedFiles.map(file => (
                        <div
                            key={file.id}
                            onClick={() => handleFileClick(file)}
                            className="relative group w-24 h-24 bg-green-600 rounded flex flex-col items-center justify-center cursor-pointer hover:bg-green-700 transition-colors shadow-md border-2 border-transparent hover:border-yellow-300"
                        >
                            <FileSpreadsheet className="text-white mb-2" size={32} />
                            <span className="text-white text-xs px-1 text-center line-clamp-2 break-all font-bold">
                                {file.name}
                            </span>

                            {/* Delete Button (Hover) */}
                            <button
                                onClick={(e) => deleteFile(e, file.id)}
                                className="absolute -top-2 -right-2 bg-red-500 text-white rounded-full p-1 opacity-0 group-hover:opacity-100 transition-opacity shadow-sm"
                            >
                                <Trash2 size={12} />
                            </button>
                        </div>
                    ))}

                    {/* Add Button */}
                    <button
                        onClick={() => fileInputRef.current?.click()}
                        className="w-24 h-24 bg-[#a9a9a9] rounded flex flex-col items-center justify-center cursor-pointer hover:bg-[#b0b0b0] transition-colors border-2 border-dashed border-[#666666] text-[#444444]"
                    >
                        <Plus size={32} />
                        <span className="text-xs font-bold mt-1">导入表格</span>
                    </button>
                </div>

                {/* Hidden Input */}
                <input
                    type="file"
                    ref={fileInputRef}
                    className="hidden"
                    accept=".xlsx, .xls, .csv"
                    onChange={handleFileSelect}
                />

                {/* Sub Modals */}
                {currentData && (
                    <>
                        <MidtermGradeModal
                            isOpen={showMidtermModal}
                            onClose={() => setShowMidtermModal(false)}
                            fileName={currentData.name}
                            data={currentData.data}
                            classId={classId}
                            file={currentData.file}
                            autoPrompt={currentData.id === autoPromptFileId}
                            onAutoPromptComplete={() => setAutoPromptFileId(null)}
                            onDataChange={(newData) => handleFileUpdate(currentData.id, newData)}
                        />
                        <StudentPhysiqueModal
                            isOpen={showPhysiqueModal}
                            onClose={() => setShowPhysiqueModal(false)}
                            fileName={currentData.name}
                            data={currentData.data}
                            classId={classId}
                            file={currentData.file}
                            autoPrompt={currentData.id === autoPromptFileId}
                            onAutoPromptComplete={() => setAutoPromptFileId(null)}
                            onDataChange={(newData) => handleFileUpdate(currentData.id, newData)}
                        />
                    </>
                )}
            </div>
        </div>
    );
};

export default CustomListModal;
