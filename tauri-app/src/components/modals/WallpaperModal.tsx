import { useRef, useState, useEffect } from 'react';
import { X, Image as ImageIcon, Upload, Check, Download, Layers, Globe, RefreshCw, Loader2 } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { useDraggable } from '../../hooks/useDraggable';

interface WallpaperModalProps {
    isOpen: boolean;
    onClose: () => void;
    groupId?: string;
}

interface Wallpaper {
    id: number;
    name: string;
    image_url: string;
    is_current?: number; // 1 for true
}

interface BackendResponse {
    code: number;
    message: string;
    data: {
        wallpapers: Wallpaper[];
    };
}

const WallpaperModal = ({ isOpen, onClose, groupId }: WallpaperModalProps) => {
    const [activeTab, setActiveTab] = useState<'class' | 'library'>('class');
    const [wallpapers, setWallpapers] = useState<Wallpaper[]>([]);
    const [selectedId, setSelectedId] = useState<number | null>(null);
    const [loading, setLoading] = useState(false);
    const [actionLoading, setActionLoading] = useState(false);
    const [uploading, setUploading] = useState(false);
    const [error, setError] = useState<string | null>(null);

    const fileInputRef = useRef<HTMLInputElement>(null);
    const { style, handleMouseDown } = useDraggable();

    useEffect(() => {
        if (isOpen && groupId) {
            fetchWallpapers();
            setSelectedId(null);
        }
    }, [isOpen, groupId, activeTab]);

    const fetchWallpapers = async () => {
        setLoading(true);
        setError(null);
        setWallpapers([]);
        try {
            let resStr = "";
            if (activeTab === 'class' && groupId) {
                resStr = await invoke<string>('fetch_class_wallpapers', { groupId });
            } else if (activeTab === 'library') {
                resStr = await invoke<string>('fetch_wallpaper_library');
            }

            if (resStr) {
                const res: BackendResponse = JSON.parse(resStr);
                if (res.data && res.data.wallpapers) {
                    setWallpapers(res.data.wallpapers);
                }
            }
        } catch (e) {
            console.error(e);
            setError("加载壁纸失败");
        } finally {
            setLoading(false);
        }
    };

    const handleAction = async () => {
        if (!selectedId || !groupId) return;
        setActionLoading(true);

        try {
            if (activeTab === 'class') {
                // Set as Wallpaper
                await invoke('set_class_wallpaper', { groupId, wallpaperId: selectedId });
                alert("已设置壁纸成功");
                // Refresh list to update 'is_current'
                fetchWallpapers();
            } else {
                // Download from Library
                await invoke('download_wallpaper', { groupId, wallpaperId: selectedId });
                alert("已下载到班级壁纸");
                // Switch to class view?
                setActiveTab('class');
            }
        } catch (e) {
            console.error(e);
            alert("操作失败: " + String(e));
        } finally {
            setActionLoading(false);
        }
    };

    const handleSelectFile = () => {
        if (fileInputRef.current) {
            fileInputRef.current.click();
        }
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        // Reset value so same file can be selected again
        if (fileInputRef.current) fileInputRef.current.value = "";

        // Validate type
        if (!["image/png", "image/jpeg", "image/gif", "image/bmp"].includes(file.type)) {
            alert("请选择图片文件 (PNG, JPG, BMP, GIF)");
            return;
        }

        setUploading(true);
        try {
            const buffer = await file.arrayBuffer();
            const bytes = Array.from(new Uint8Array(buffer)); // Convert to regular array for serialization if needed, or check tauri invoke support for Uint8Array. 
            // Tauri v2 invoke can handle Uint8Array directly usually, but let's see. 
            // If `invoke` args expects JSON, default serialization of Uint8Array is standard array.

            const res = await invoke<string>('upload_wallpaper', {
                groupId,
                imageData: bytes,
                mimeType: file.type
            });
            console.log("Upload Wallpaper Response:", res);
            try {
                const parsed = JSON.parse(res);
                if (parsed.code === 200 || (parsed.data && parsed.data.code === 200)) {
                    alert("上传成功");
                    fetchWallpapers(); // Refresh
                } else {
                    alert("上传失败: " + (parsed.message || parsed.data?.message || "未知错误"));
                }
            } catch (e) {
                console.error("Parse error", e);
                // If parse fails, it might be raw string or failure.
                // Assuming standard JSON response structure.
                alert("上传响应解析失败");
            }
        } catch (e) {
            console.error(e);
            alert("上传失败: " + String(e));
        } finally {
            setUploading(false);
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/60 backdrop-blur-sm animate-in fade-in duration-200">
            {/* Dark Theme Container to Match QT */}
            <div
                style={style}
                className="bg-[#2b2b2b] text-white rounded-lg shadow-2xl w-[900px] h-[700px] flex flex-col overflow-hidden border border-[#444]"
            >
                {/* Header / Tabs */}
                <div
                    onMouseDown={handleMouseDown}
                    className="h-14 flex items-center justify-between px-4 bg-[#333] border-b border-[#444] cursor-move select-none"
                >
                    <div className="flex items-center gap-4">
                        <button
                            onClick={() => setActiveTab('class')}
                            className={`px-4 py-2 rounded font-bold text-sm transition-colors ${activeTab === 'class' ? 'bg-[#608bd0] text-white' : 'bg-[#555] text-white hover:bg-[#666]'}`}
                        >
                            班级壁纸
                        </button>
                        <button
                            onClick={() => setActiveTab('library')}
                            className={`px-4 py-2 rounded font-bold text-sm transition-colors ${activeTab === 'library' ? 'bg-[#608bd0] text-white' : 'bg-[#555] text-white hover:bg-[#666]'}`}
                        >
                            壁纸库
                        </button>
                    </div>
                    <div className="flex items-center gap-2">
                        {activeTab === 'class' ? (
                            <button
                                onClick={handleAction}
                                disabled={!selectedId || actionLoading}
                                className={`px-4 py-2 rounded font-bold text-sm transition-colors flex items-center gap-2 ${!selectedId ? 'bg-[#444] text-gray-400 cursor-not-allowed' : 'bg-[#555] text-white hover:bg-[#666]'}`}
                            >
                                {actionLoading ? <Loader2 size={14} className="animate-spin" /> : <Check size={14} />} 设为壁纸
                            </button>
                        ) : (
                            <button
                                onClick={handleAction}
                                disabled={!selectedId || actionLoading}
                                className={`px-4 py-2 rounded font-bold text-sm transition-colors flex items-center gap-2 ${!selectedId ? 'bg-[#444] text-gray-400 cursor-not-allowed' : 'bg-[#608bd0] text-white hover:bg-[#4f78be]'}`}
                            >
                                {actionLoading ? <Loader2 size={14} className="animate-spin" /> : <Download size={14} />} 下载
                            </button>
                        )}
                        <button onClick={onClose} className="ml-2 p-1.5 hover:bg-red-500 rounded-full text-gray-400 hover:text-white transition-colors">
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-y-auto p-4 bg-[#2b2b2b]">
                    {loading ? (
                        <div className="flex items-center justify-center h-full text-gray-400 gap-2">
                            <RefreshCw className="animate-spin" /> 加载中...
                        </div>
                    ) : error ? (
                        <div className="flex flex-col items-center justify-center h-full gap-2">
                            <span className="text-red-400">{error}</span>
                            <button onClick={fetchWallpapers} className="text-blue-400 underline">重试</button>
                        </div>
                    ) : (
                        <div className="grid grid-cols-5 gap-4">
                            {/* Upload Button (Class Tab Only) - Temporarily disabled as server interface is missing
                            {activeTab === 'class' && (
                                <div
                                    onClick={handleSelectFile}
                                    className="aspect-[3/4] bg-[#444] border-2 border-dashed border-[#888] rounded-lg flex flex-col items-center justify-center gap-2 cursor-pointer hover:border-white hover:bg-[#555] transition-all relative"
                                >
                                    {uploading ? (
                                        <>
                                            <Loader2 size={32} className="animate-spin text-white" />
                                            <span className="text-xs text-gray-300">上传中...</span>
                                        </>
                                    ) : (
                                        <>
                                            <Upload size={32} className="text-white opactiy-80" />
                                            <span className="text-xs text-gray-300">添加本地壁纸</span>
                                        </>
                                    )}
                                    <input
                                        type="file"
                                        ref={fileInputRef}
                                        onChange={handleFileChange}
                                        accept="image/png, image/jpeg, image/gif, image/bmp"
                                        className="hidden"
                                    />
                                </div>
                            )}
                            */}

                            {wallpapers.map(wp => {
                                const isSelected = selectedId === wp.id;
                                const isCurrent = activeTab === 'class' && wp.is_current === 1; // Backend returns 1 for true
                                return (
                                    <div
                                        key={wp.id}
                                        onClick={() => setSelectedId(wp.id)}
                                        className={`group relative aspect-[3/4] bg-[#333] rounded-lg overflow-hidden cursor-pointer border-2 transition-all ${isSelected ? 'border-blue-500' : 'border-[#666]'}`}
                                    >
                                        <img src={wp.image_url} alt={wp.name} className="w-full h-full object-cover" />

                                        {/* Overlay for Name */}
                                        <div className="absolute bottom-0 left-0 right-0 bg-black/60 p-2 text-center text-xs truncate">
                                            {wp.name || `Wallpaper ${wp.id}`}
                                        </div>

                                        {isCurrent && (
                                            <div className="absolute top-2 right-2 bg-green-500 text-white text-[10px] px-2 py-0.5 rounded shadow">
                                                当前
                                            </div>
                                        )}

                                        {isSelected && (
                                            <div className="absolute inset-0 border-4 border-blue-500 rounded-lg pointer-events-none"></div>
                                        )}
                                    </div>
                                );
                            })}
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default WallpaperModal;
