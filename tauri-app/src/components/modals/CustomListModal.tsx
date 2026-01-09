import { useState, useRef } from 'react';
import { X, FileSpreadsheet, Upload, AlertCircle, CheckCircle, Info } from 'lucide-react';
// import { invoke } from '@tauri-apps/api/core';

interface Props {
    isOpen: boolean;
    onClose: () => void;
}

const CustomListModal = ({ isOpen, onClose }: Props) => {
    const [dragActive, setDragActive] = useState(false);
    const [file, setFile] = useState<File | null>(null);
    const [importing, setImporting] = useState(false);
    const [importStatus, setImportStatus] = useState<'idle' | 'success' | 'error'>('idle');
    const inputRef = useRef<HTMLInputElement>(null);

    const handleDrag = (e: React.DragEvent) => {
        e.preventDefault();
        e.stopPropagation();
        if (e.type === "dragenter" || e.type === "dragover") {
            setDragActive(true);
        } else if (e.type === "dragleave") {
            setDragActive(false);
        }
    };

    const handleDrop = (e: React.DragEvent) => {
        e.preventDefault();
        e.stopPropagation();
        setDragActive(false);
        if (e.dataTransfer.files && e.dataTransfer.files[0]) {
            handleFile(e.dataTransfer.files[0]);
        }
    };

    const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        e.preventDefault();
        if (e.target.files && e.target.files[0]) {
            handleFile(e.target.files[0]);
        }
    };

    const handleFile = (file: File) => {
        // Simple validation
        if (file.name.endsWith('.xlsx') || file.name.endsWith('.xls') || file.name.endsWith('.csv')) {
            setFile(file);
            setImportStatus('idle');
        } else {
            alert("请选择 Excel (.xlsx, .xls) 或 CSV 文件");
        }
    };

    const handleImport = async () => {
        if (!file) return;
        setImporting(true);

        // Mock Import Process
        setTimeout(() => {
            setImporting(false);
            setImportStatus('success');
            // Here we would parse excel and call backend or update state
            console.log("Importing file:", file.name);
        }, 1500);
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm p-4">
            <div className="bg-white rounded-xl shadow-2xl w-full max-w-lg flex flex-col overflow-hidden animate-in fade-in zoom-in-95 duration-200">
                {/* Header */}
                <div className="flex items-center justify-between p-4 border-b border-gray-100 bg-gray-50/50">
                    <h3 className="font-bold text-gray-800 text-lg flex items-center gap-2">
                        <FileSpreadsheet className="text-green-600" size={20} />
                        学生统计表导入
                    </h3>
                    <button onClick={onClose} className="p-1.5 hover:bg-gray-200 rounded-full text-gray-500 transition-colors">
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-8">
                    {/* Drag & Drop Zone */}
                    <div
                        className={`border-2 border-dashed rounded-xl p-10 flex flex-col items-center justify-center transition-all cursor-pointer bg-gray-50/50 ${dragActive ? 'border-blue-500 bg-blue-50/20' : 'border-gray-300 hover:border-blue-400 hover:bg-gray-50'}`}
                        onDragEnter={handleDrag}
                        onDragLeave={handleDrag}
                        onDragOver={handleDrag}
                        onDrop={handleDrop}
                        onClick={() => inputRef.current?.click()}
                    >
                        <input
                            ref={inputRef}
                            type="file"
                            className="hidden"
                            accept=".xlsx,.xls,.csv"
                            onChange={handleChange}
                        />

                        {file ? (
                            <div className="text-center">
                                <FileSpreadsheet size={48} className="text-green-600 mx-auto mb-4" />
                                <h4 className="font-bold text-gray-800 mb-1">{file.name}</h4>
                                <p className="text-sm text-gray-500">{(file.size / 1024).toFixed(1)} KB</p>
                            </div>
                        ) : (
                            <div className="text-center">
                                <Upload size={48} className="text-gray-400 mx-auto mb-4" />
                                <h4 className="font-bold text-gray-700 mb-2">点击或拖拽上传文件</h4>
                                <p className="text-sm text-gray-500 max-w-xs mx-auto">
                                    支持期中成绩单、学生体质表、小组管理表<br />
                                    <span className="text-xs text-gray-400">(.xlsx, .xls, .csv)</span>
                                </p>
                            </div>
                        )}
                    </div>

                    {/* Status & Actions */}
                    <div className="mt-8 flex items-center justify-center">
                        {importStatus === 'success' ? (
                            <div className="text-center animate-in fade-in slide-in-from-bottom-2 duration-300">
                                <div className="w-12 h-12 bg-green-100 text-green-600 rounded-full flex items-center justify-center mx-auto mb-2">
                                    <CheckCircle size={24} />
                                </div>
                                <h4 className="font-bold text-gray-800">导入成功</h4>
                                <p className="text-sm text-gray-500 mt-1">数据已同步并更新</p>
                                <button
                                    onClick={onClose}
                                    className="mt-4 px-6 py-2 bg-gray-100 hover:bg-gray-200 text-gray-700 rounded-lg text-sm font-medium transition-colors"
                                >
                                    关闭
                                </button>
                            </div>
                        ) : importStatus === 'error' ? (
                            <div className="text-center text-red-500">
                                <AlertCircle size={32} className="mx-auto mb-2" />
                                <p>导入失败，请检查文件格式</p>
                            </div>
                        ) : (
                            <button
                                onClick={handleImport}
                                disabled={!file || importing}
                                className={`w-full py-3 rounded-lg font-bold text-white transition-all transform active:scale-95 ${!file || importing ? 'bg-gray-300 cursor-not-allowed' : 'bg-green-600 hover:bg-green-700 shadow-md hover:shadow-lg'}`}
                            >
                                {importing ? (
                                    <span className="flex items-center justify-center gap-2">
                                        <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                                        处理中...
                                    </span>
                                ) : "开始导入"}
                            </button>
                        )}
                    </div>
                </div>

                {/* Templates Hint */}
                <div className="bg-amber-50 p-3 text-xs text-amber-800 text-center border-t border-amber-100">
                    <Info size={12} className="inline mr-1" />
                    请确保表格包含“学号”和“姓名”列以正确匹配学生数据
                </div>
            </div>
        </div>
    );
};

export default CustomListModal;
