import { useState, useRef } from 'react';
import { X, FileSpreadsheet, Plus, Trash2 } from 'lucide-react';
import * as XLSX from 'xlsx';
import MidtermGradeModal from './MidtermGradeModal';
import StudentPhysiqueModal from './StudentPhysiqueModal';

// Define the shape of the data rows we might parse (generic)
type SheetRow = (string | number | undefined)[];

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
}

const CustomListModal = ({ isOpen, onClose, classId }: CustomListModalProps) => {
    const [importedFiles, setImportedFiles] = useState<ImportedFile[]>([]);
    const [selectedFileId, setSelectedFileId] = useState<string | null>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);

    // Sub-modal states
    const [showMidtermModal, setShowMidtermModal] = useState(false);
    const [showPhysiqueModal, setShowPhysiqueModal] = useState(false);
    const [currentData, setCurrentData] = useState<ImportedFile | null>(null);


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
            const dataRows = rawData.slice(1); // Not used for type detection but stored

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
                headers: headers
            };

            setImportedFiles(prev => [...prev, newFile]);
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

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-[#808080] rounded shadow-2xl w-[600px] h-[400px] flex flex-col overflow-hidden text-sm">

                {/* Header */}
                <div className="flex items-center justify-between p-2">
                    <div className="bg-[#d3d3d3] text-black px-4 py-2 rounded font-bold text-base">
                        学生统计表导入
                    </div>
                    <button onClick={onClose} className="w-8 h-8 flex items-center justify-center bg-[#666666] text-white rounded hover:bg-[#777777] font-bold">
                        X
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
                {/* We need to pass data to these modals. Since they don't exist yet, we will just comment/placeholder them or implement props in them later. */}
                {currentData && (
                    <>
                        <MidtermGradeModal
                            isOpen={showMidtermModal}
                            onClose={() => setShowMidtermModal(false)}
                            fileName={currentData.name}
                            data={currentData.data}
                            classId={classId}
                        />
                        <StudentPhysiqueModal
                            isOpen={showPhysiqueModal}
                            onClose={() => setShowPhysiqueModal(false)}
                            fileName={currentData.name}
                            data={currentData.data}
                            classId={classId}
                        />
                    </>
                )}
            </div>
        </div>
    );
};

export default CustomListModal;
