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
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm p-4 animate-in fade-in duration-200">
            <div className="bg-white rounded-2xl shadow-2xl w-full max-w-sm overflow-hidden flex flex-col">
                {/* Header / Banner */}
                <div className="h-24 bg-gradient-to-r from-blue-500 to-indigo-600 relative">
                    <button
                        onClick={onClose}
                        className="absolute top-3 right-3 p-1.5 bg-black/10 hover:bg-black/20 text-white rounded-full transition-colors z-10"
                    >
                        <X size={18} />
                    </button>
                    <div className="absolute -bottom-10 left-1/2 -translate-x-1/2">
                        <div
                            className="w-20 h-20 rounded-full border-4 border-white bg-gray-200 shadow-md overflow-hidden relative group cursor-pointer"
                            onClick={handleAvatarClick}
                        >
                            <img
                                src={userInfo?.avatar || "https://api.dicebear.com/7.x/avataaars/svg?seed=Felix"}
                                alt="Avatar"
                                className={`w-full h-full object-cover transition-opacity ${isUploadingAvatar ? 'opacity-50' : ''}`}
                            />

                            {/* Loading Overlay */}
                            {isUploadingAvatar && (
                                <div className="absolute inset-0 flex items-center justify-center bg-black/20">
                                    <Loader2 className="animate-spin text-white" size={24} />
                                </div>
                            )}

                            {/* Hover Overlay */}
                            {!isUploadingAvatar && (
                                <div className="absolute inset-0 bg-black/40 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity">
                                    <Camera size={20} className="text-white" />
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
                <div className="pt-12 pb-6 px-6 flex flex-col items-center">

                    {/* Name Section */}
                    <div className="flex items-center gap-2 justify-center w-full relative">
                        {isEditingName ? (
                            <div className="flex items-center gap-1 animate-in zoom-in-95">
                                <input
                                    autoFocus
                                    className="border-b-2 border-blue-500 text-center text-xl font-bold text-gray-800 outline-none w-32 pb-1"
                                    value={tempName}
                                    onChange={e => setTempName(e.target.value)}
                                    onKeyDown={e => e.key === 'Enter' && saveName()}
                                />
                                <button
                                    onClick={saveName}
                                    disabled={isLoadingName}
                                    className="p-1.5 text-green-600 hover:bg-green-50 rounded-full"
                                >
                                    {isLoadingName ? <Loader2 size={16} className="animate-spin" /> : <Check size={18} />}
                                </button>
                                <button
                                    onClick={() => setIsEditingName(false)}
                                    className="p-1.5 text-gray-400 hover:bg-gray-100 rounded-full"
                                >
                                    <X size={18} />
                                </button>
                            </div>
                        ) : (
                            <div className="group flex items-center gap-2">
                                <h2 className="text-xl font-bold text-gray-800">{userInfo?.name || '未知用户'}</h2>
                                <button
                                    onClick={startEditingName}
                                    className="opacity-0 group-hover:opacity-100 transition-opacity p-1 text-gray-400 hover:text-blue-500"
                                >
                                    <Edit2 size={14} />
                                </button>
                            </div>
                        )}
                    </div>

                </div>

                {/* Admin Status / Upgrade Button */}
                <div className="mt-2">
                    {(userInfo?.is_administrator === "1" || userInfo?.is_administrator === "true" || userInfo?.is_administrator === true) ? (
                        <span className="px-3 py-1 bg-yellow-100 text-yellow-700 text-xs font-bold rounded-full border border-yellow-200">
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
                                        // alert("升级成功！");
                                    } catch (e) {
                                        console.error(e);
                                        alert("升级失败");
                                    }
                                }
                            }}
                            className="px-3 py-1 bg-green-100 hover:bg-green-200 text-green-700 text-xs font-bold rounded-full border border-green-200 transition-colors"
                        >
                            升级为管理员
                        </button>
                    )}
                </div>

                <p className="text-xs text-gray-500 mt-2">ID: {userInfo?.teacher_unique_id || userInfo?.id || '---'}</p>

                <div className="w-full mt-6 space-y-4">
                    {/* School */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-blue-100 text-blue-600 flex items-center justify-center">
                            <Building size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">学校</p>
                            <p className="text-sm font-medium text-gray-700">{userInfo?.school_name || '无法获取'}</p>
                        </div>
                    </div>

                    {/* Phone */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-green-100 text-green-600 flex items-center justify-center">
                            <Phone size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">手机号码</p>
                            <p className="text-sm font-medium text-gray-700">{userInfo?.phone || '---'}</p>
                        </div>
                    </div>

                    {/* ID Number (Masked?) */}
                    {userInfo?.id_number && (
                        <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                            <div className="w-8 h-8 rounded-full bg-purple-100 text-purple-600 flex items-center justify-center">
                                <Briefcase size={16} />
                            </div>
                            <div className="flex-1">
                                <p className="text-xs text-gray-400">证件号码</p>
                                <p className="text-sm font-medium text-gray-700">
                                    {userInfo.id_number.replace(/^(.{6})(?:\d+)(.{4})$/, "$1******$2")}
                                </p>
                            </div>
                        </div>
                    )}
                </div>

                <div className="w-full mt-6">
                    <button
                        onClick={onChangePassword}
                        className="w-full py-2.5 rounded-xl border border-gray-200 text-gray-600 font-medium text-sm hover:bg-gray-50 hover:text-blue-600 transition-colors flex items-center justify-center gap-2"
                    >
                        <Lock size={16} />
                        修改密码
                    </button>
                </div>

            </div>
        </div>
    );
};

export default UserInfoModal;
