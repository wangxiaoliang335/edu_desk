import { useState, useEffect, useRef } from "react";
import { X, Phone, Building, Briefcase, Lock, Camera, Edit2, Check, Loader2 } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";

interface UserInfoModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo: any;
    onChangePassword: () => void;
}

const UserInfoModal = ({ isOpen, onClose, userInfo: initialUserInfo, onChangePassword }: UserInfoModalProps) => {
    const [userInfo, setUserInfo] = useState(initialUserInfo);
    const [isEditingName, setIsEditingName] = useState(false);
    const [tempName, setTempName] = useState("");
    const [isLoadingName, setIsLoadingName] = useState(false);
    const [isUploadingAvatar, setIsUploadingAvatar] = useState(false);

    const fileInputRef = useRef<HTMLInputElement>(null);

    useEffect(() => {
        setUserInfo(initialUserInfo);
    }, [initialUserInfo, isOpen]);

    if (!isOpen) return null;

    const handleAvatarClick = () => {
        fileInputRef.current?.click();
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        // Convert to Base64
        const reader = new FileReader();
        reader.onloadend = async () => {
            const base64String = reader.result as string;
            // Remove prefix if present (optional, but backend likely expects raw base64 or complete data URI)
            // Usually data URI "data:image/png;base64,..." is what web uses. Rust backend will likely pass it as is.
            // But we should check if backend handles stripping. 
            // The C++ logic read raw bytes -> toBase64. So it's pure Base64 without "data:image..." prefix?
            // "QString imageBase64 = QString::fromLatin1(imageData.toBase64());"
            // Yes, it acts as pure Base64.
            const pureBase64 = base64String.split(',')[1] || base64String;

            try {
                setIsUploadingAvatar(true);
                // Call update_user_info with FULL object but updated Avatar
                // Need to construct full payload from current userInfo
                const payload = {
                    phone: userInfo.phone || "",
                    id_number: userInfo.id_number || "",
                    name: userInfo.name || "",
                    avatar: base64String, // Send FULL data URL for rendering convenience if backend stores it as string?
                    // Wait, C++ sent PURE Base64. But if web renders it, data URI is better.
                    // Let's assume backend just stores string. If I send Data URI, it might be safer for Web.
                    // C++: m_userInfo.avatar = imageBase64; 
                    // If backend expects pure base64 for legacy compat, might be issue.
                    // However, let's try sending Data URI first as it's standard for web.
                    // If image breaks on other clients, we revert.
                    // Updated strategy: Send pure Base64 if needed, but <img src> needs prefix.
                    // Re-reading logic: Backend just passes param.
                    sex: userInfo.sex || "未知",
                    address: userInfo.address || "",
                    school_name: userInfo.school_name || "",
                    grade_level: userInfo.grade_level || "",
                    is_administrator: userInfo.is_administrator || "0"
                };

                // NOTE: We used `update_user_info` for avatar in plan.
                await invoke('update_user_info', payload);

                // Update local state immediately
                setUserInfo({ ...userInfo, avatar: base64String });
                // Trigger global refresh if needed?
                // window.dispatchEvent(new CustomEvent('refresh-user-info')); 

            } catch (err) {
                console.error("Failed to upload avatar:", err);
                alert("头像上传失败，请重试");
            } finally {
                setIsUploadingAvatar(false);
            }
        };
        reader.readAsDataURL(file);
    };

    const startEditingName = () => {
        setTempName(userInfo.name);
        setIsEditingName(true);
    };

    const saveName = async () => {
        if (!tempName.trim()) {
            alert("姓名不能为空");
            return;
        }
        if (tempName === userInfo.name) {
            setIsEditingName(false);
            return;
        }

        try {
            setIsLoadingName(true);
            await invoke('update_user_name', {
                phone: userInfo.phone,
                name: tempName,
                id_number: userInfo.id_number || ""
            });

            setUserInfo({ ...userInfo, name: tempName });
            setIsEditingName(false);
        } catch (err) {
            console.error("Failed to update name:", err);
            alert("姓名修改失败");
        } finally {
            setIsLoadingName(false);
        }
    };

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/30 backdrop-blur-sm p-4 animate-in fade-in duration-300">
            <div className="bg-white/95 backdrop-blur-xl rounded-3xl shadow-2xl w-full max-w-sm overflow-hidden flex flex-col border border-white/50 ring-1 ring-sage-100">
                {/* Header / Banner - Warm Sage Gradient */}
                <div className="h-28 bg-gradient-to-br from-sage-400 to-sage-600 relative overflow-hidden">
                    {/* Decorative blobs */}
                    <div className="absolute top-0 right-0 w-32 h-32 bg-white/10 rounded-full blur-2xl -translate-y-1/2 translate-x-1/2"></div>
                    <div className="absolute bottom-0 left-0 w-24 h-24 bg-clay-400/20 rounded-full blur-xl translate-y-1/2 -translate-x-1/2"></div>

                    <button
                        onClick={onClose}
                        className="absolute top-4 right-4 p-2 bg-white/20 hover:bg-white/40 text-white rounded-full transition-all duration-300 z-10 backdrop-blur-md"
                    >
                        <X size={18} />
                    </button>
                    <div className="absolute -bottom-10 left-1/2 -translate-x-1/2 z-20">
                        <div
                            className="w-24 h-24 rounded-[2rem] border-4 border-white bg-white shadow-xl shadow-sage-500/20 overflow-hidden relative group cursor-pointer transition-transform hover:scale-105"
                            onClick={handleAvatarClick}
                        >
                            <img
                                src={userInfo?.avatar || "https://api.dicebear.com/7.x/avataaars/svg?seed=Teacher"}
                                alt="Avatar"
                                className={`w-full h-full object-cover transition-opacity duration-300 ${isUploadingAvatar ? 'opacity-50' : ''}`}
                            />

                            {/* Loading Overlay */}
                            {isUploadingAvatar && (
                                <div className="absolute inset-0 flex items-center justify-center bg-black/20">
                                    <Loader2 className="animate-spin text-white" size={24} />
                                </div>
                            )}

                            {/* Hover Overlay */}
                            {!isUploadingAvatar && (
                                <div className="absolute inset-0 bg-black/20 backdrop-blur-[2px] flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity duration-300">
                                    <Camera size={24} className="text-white drop-shadow-md" />
                                </div>
                            )}

                            <input
                                type="file"
                                ref={fileInputRef}
                                className="hidden"
                                accept="image/png,image/jpeg,image/jpg"
                                onChange={handleFileChange}
                            />
                        </div>
                    </div>
                </div>

                {/* Body */}
                <div className="pt-14 pb-8 px-8 flex flex-col items-center flex-1">

                    {/* Name Section */}
                    <div className="flex items-center gap-2 justify-center w-full relative mb-1">
                        {isEditingName ? (
                            <div className="flex items-center gap-2 animate-in zoom-in-95">
                                <input
                                    autoFocus
                                    className="border-b-2 border-sage-500 text-center text-xl font-bold text-ink-800 outline-none w-32 pb-1 bg-transparent"
                                    value={tempName}
                                    onChange={e => setTempName(e.target.value)}
                                    onKeyDown={e => e.key === 'Enter' && saveName()}
                                />
                                <button
                                    onClick={saveName}
                                    disabled={isLoadingName}
                                    className="p-1.5 text-sage-600 hover:bg-sage-50 rounded-full transition-colors"
                                >
                                    {isLoadingName ? <Loader2 size={16} className="animate-spin" /> : <Check size={18} />}
                                </button>
                                <button
                                    onClick={() => setIsEditingName(false)}
                                    className="p-1.5 text-ink-400 hover:bg-clay-50 hover:text-clay-600 rounded-full transition-colors"
                                >
                                    <X size={18} />
                                </button>
                            </div>
                        ) : (
                            <div className="group flex items-center gap-2">
                                <h2 className="text-2xl font-bold text-ink-800 tracking-tight">{userInfo?.name || '未知用户'}</h2>
                                <button
                                    onClick={startEditingName}
                                    className="opacity-0 group-hover:opacity-100 transition-all duration-200 p-1.5 text-sage-400 hover:text-sage-600 hover:bg-sage-50 rounded-full -mr-8"
                                >
                                    <Edit2 size={14} />
                                </button>
                            </div>
                        )}
                    </div>

                    {/* Admin Status */}
                    <div className="mb-6">
                        {(userInfo?.is_administrator === "1" || userInfo?.is_administrator === "true" || userInfo?.is_administrator === true) ? (
                            <span className="px-3 py-0.5 bg-amber-50 text-amber-600 text-[10px] font-bold rounded-full border border-amber-100 tracking-wider uppercase">
                                管理员
                            </span>
                        ) : (
                            <button
                                onClick={async () => {
                                    if (confirm("确定要升级为管理员吗？")) {
                                        try {
                                            await invoke('update_user_administrator', {
                                                phone: userInfo.phone,
                                                id_number: userInfo.id_number || "",
                                                is_administrator: "1"
                                            });
                                            setUserInfo({ ...userInfo, is_administrator: "1" });
                                        } catch (e) {
                                            console.error(e);
                                            alert("升级失败");
                                        }
                                    }
                                }}
                                className="px-3 py-1 bg-sage-50 hover:bg-sage-100 text-sage-600 text-xs font-bold rounded-full border border-sage-200 transition-colors"
                            >
                                升级为管理员
                            </button>
                        )}
                    </div>

                    {/* ID Badge */}
                    <div className="text-xs font-mono text-ink-300 bg-ink-50 px-3 py-1 rounded-lg mb-6">
                        ID: {userInfo?.teacher_unique_id || userInfo?.id || '---'}
                    </div>

                    <div className="w-full space-y-3">
                        {/* School */}
                        <div className="flex items-center gap-4 p-4 bg-white rounded-2xl border border-sage-50 shadow-[0_2px_8px_rgba(0,0,0,0.02)] hover:shadow-[0_4px_12px_rgba(0,0,0,0.04)] transition-shadow duration-300">
                            <div className="w-10 h-10 rounded-2xl bg-blue-50 text-blue-500 flex items-center justify-center">
                                <Building size={20} />
                            </div>
                            <div className="flex-1">
                                <p className="text-[10px] font-bold text-ink-300 uppercase tracking-wider">学校</p>
                                <p className="text-sm font-bold text-ink-700">{userInfo?.school_name || '无法获取'}</p>
                            </div>
                        </div>

                        {/* Phone */}
                        <div className="flex items-center gap-4 p-4 bg-white rounded-2xl border border-sage-50 shadow-[0_2px_8px_rgba(0,0,0,0.02)] hover:shadow-[0_4px_12px_rgba(0,0,0,0.04)] transition-shadow duration-300">
                            <div className="w-10 h-10 rounded-2xl bg-sage-50 text-sage-500 flex items-center justify-center">
                                <Phone size={20} />
                            </div>
                            <div className="flex-1">
                                <p className="text-[10px] font-bold text-ink-300 uppercase tracking-wider">手机号码</p>
                                <p className="text-sm font-bold text-ink-700 font-mono tracking-tight">{userInfo?.phone || '---'}</p>
                            </div>
                        </div>

                        {/* ID Number */}
                        {userInfo?.id_number && (
                            <div className="flex items-center gap-4 p-4 bg-white rounded-2xl border border-sage-50 shadow-[0_2px_8px_rgba(0,0,0,0.02)] hover:shadow-[0_4px_12px_rgba(0,0,0,0.04)] transition-shadow duration-300">
                                <div className="w-10 h-10 rounded-2xl bg-purple-50 text-purple-500 flex items-center justify-center">
                                    <Briefcase size={20} />
                                </div>
                                <div className="flex-1">
                                    <p className="text-[10px] font-bold text-ink-300 uppercase tracking-wider">证件号码</p>
                                    <p className="text-sm font-bold text-ink-700 font-mono tracking-tight">
                                        {userInfo.id_number.replace(/^(.{6})(?:\d+)(.{4})$/, "$1******$2")}
                                    </p>
                                </div>
                            </div>
                        )}
                    </div>

                    <div className="w-full mt-8">
                        <button
                            onClick={onChangePassword}
                            className="w-full py-3.5 rounded-2xl border border-sage-200 text-ink-500 font-bold text-sm hover:bg-sage-50 hover:text-sage-700 hover:border-sage-300 transition-all duration-300 flex items-center justify-center gap-2 group active:scale-[0.98]"
                        >
                            <Lock size={16} className="text-sage-400 group-hover:text-sage-600" />
                            修改密码
                        </button>
                    </div>

                </div>
            </div>
        </div>
    );
};

export default UserInfoModal;
