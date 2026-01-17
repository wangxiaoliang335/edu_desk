import { useState, useEffect } from "react";
import { X, Lock, Phone as PhoneIcon, Key, CheckCircle, Shield } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";

interface ChangePasswordModalProps {
    isOpen: boolean;
    onClose: () => void;
    defaultPhone?: string;
}

const ChangePasswordModal = ({ isOpen, onClose, defaultPhone }: ChangePasswordModalProps) => {
    const [phone, setPhone] = useState("");
    const [password, setPassword] = useState("");
    const [confirmPassword, setConfirmPassword] = useState("");
    const [code, setCode] = useState("");

    const [isSending, setIsSending] = useState(false);
    const [countdown, setCountdown] = useState(0);
    const [isLoading, setIsLoading] = useState(false);
    const [error, setError] = useState("");
    const [success, setSuccess] = useState("");

    useEffect(() => {
        if (isOpen && defaultPhone) {
            setPhone(defaultPhone);
        }
        if (!isOpen) {
            // Reset state on close
            setPassword("");
            setConfirmPassword("");
            setCode("");
            setError("");
            setSuccess("");
        }
    }, [isOpen, defaultPhone]);

    // Countdown timer
    useEffect(() => {
        let timer: any;
        if (countdown > 0) {
            timer = setInterval(() => {
                setCountdown((prev) => prev - 1);
            }, 1000);
        }
        return () => clearInterval(timer);
    }, [countdown]);

    const handleSendCode = async () => {
        if (!phone) {
            setError("请输入手机号码");
            return;
        }
        setIsSending(true);
        setError("");

        try {
            await invoke('send_verification_code', { phone });
            setCountdown(60);
            setError(""); // Clear any previous error
            // Could show success toast
        } catch (e: any) {
            console.error("Failed to send code", e);
            setError("验证码发送失败: " + (e.message || e));
        } finally {
            setIsSending(false);
        }
    };

    const handleSubmit = async () => {
        if (!phone || !password || !code) {
            setError("请填写所有字段");
            return;
        }
        if (password !== confirmPassword) {
            setError("两次输入的密码不一致");
            return;
        }
        if (password.length < 6) {
            setError("密码长度不能少于6位");
            return;
        }

        setIsLoading(true);
        setError("");

        try {
            await invoke('reset_password', {
                phone,
                newPassword: password,
                verificationCode: code
            });
            setSuccess("密码修改成功！请使用新密码重新登录。");
            setTimeout(() => {
                onClose();
                // Optionally trigger logout?
                // window.location.reload(); 
                // But user might just want to change it. Qt usually doesn't force logout unless session invalidated.
            }, 2000);
        } catch (e: any) {
            console.error("Reset password failed", e);
            setError("密码修改失败: " + (e.message || e));
            setIsLoading(false);
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/30 backdrop-blur-sm p-4 animate-in fade-in duration-300">
            <div className="bg-white/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-full max-w-md overflow-hidden flex flex-col p-8 border border-white/50 ring-1 ring-sage-100/50">

                {/* Header */}
                <div className="flex items-center justify-between mb-8">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-2xl bg-sage-50 text-sage-500 flex items-center justify-center shadow-sm">
                            <Lock size={20} />
                        </div>
                        <h2 className="text-xl font-bold text-ink-800 tracking-tight">
                            修改密码
                        </h2>
                    </div>
                    <button onClick={onClose} className="p-2 hover:bg-clay-50 hover:text-clay-600 rounded-full text-ink-400 transition-colors">
                        <X size={20} />
                    </button>
                </div>

                {success ? (
                    <div className="flex flex-col items-center justify-center py-10 space-y-4 animate-in zoom-in-50">
                        <div className="w-16 h-16 bg-sage-100 text-sage-600 rounded-full flex items-center justify-center border-4 border-sage-50">
                            <CheckCircle size={32} />
                        </div>
                        <p className="text-xl font-bold text-ink-800">修改成功</p>
                        <p className="text-sm text-ink-400 font-medium">窗口即将关闭...</p>
                    </div>
                ) : (
                    <div className="space-y-5">
                        {/* Error Msg */}
                        {error && (
                            <div className="p-3 bg-clay-50 text-clay-600 text-xs font-bold rounded-xl border border-clay-100 flex items-start gap-2 animate-in slide-in-from-top-2">
                                <Shield size={14} className="mt-0.5 shrink-0" />
                                <span>{error}</span>
                            </div>
                        )}

                        {/* Phone */}
                        <div className="space-y-1.5 group">
                            <label className="text-xs font-bold text-ink-400 ml-1 uppercase tracking-wider group-focus-within:text-sage-600 transition-colors">手机号码</label>
                            <div className="relative">
                                <PhoneIcon className="absolute left-4 top-1/2 -translate-y-1/2 text-sage-300 transition-colors group-focus-within:text-sage-500" size={18} />
                                <input
                                    type="text"
                                    value={phone}
                                    readOnly={!!defaultPhone} // Read only if phone is provided
                                    onChange={(e) => setPhone(e.target.value)}
                                    className={`w-full pl-11 pr-4 py-3.5 bg-white border border-sage-200 rounded-2xl text-sm font-bold text-ink-800 outline-none focus:border-sage-400 focus:ring-4 focus:ring-sage-100 transition-all shadow-sm placeholder-sage-300 ${defaultPhone ? 'text-ink-400 cursor-not-allowed bg-sage-50/50' : ''}`}
                                    placeholder="请输入手机号"
                                />
                            </div>
                        </div>

                        {/* Code */}
                        <div className="space-y-1.5 group">
                            <label className="text-xs font-bold text-ink-400 ml-1 uppercase tracking-wider group-focus-within:text-sage-600 transition-colors">验证码</label>
                            <div className="flex gap-3">
                                <div className="relative flex-1">
                                    <Key className="absolute left-4 top-1/2 -translate-y-1/2 text-sage-300 transition-colors group-focus-within:text-sage-500" size={18} />
                                    <input
                                        type="text"
                                        value={code}
                                        onChange={(e) => setCode(e.target.value)}
                                        className="w-full pl-11 pr-4 py-3.5 bg-white border border-sage-200 rounded-2xl text-sm font-bold text-ink-800 outline-none focus:border-sage-400 focus:ring-4 focus:ring-sage-100 transition-all shadow-sm placeholder-sage-300"
                                        placeholder="请输入验证码"
                                    />
                                </div>
                                <button
                                    onClick={handleSendCode}
                                    disabled={isSending || countdown > 0 || !phone}
                                    className={`px-4 py-2 rounded-2xl text-xs font-bold whitespace-nowrap transition-all shadow-sm border ${isSending || countdown > 0 || !phone
                                        ? "bg-sage-50 text-sage-300 border-sage-100 cursor-not-allowed"
                                        : "bg-sage-500 text-white border-sage-500 hover:bg-sage-600 hover:shadow-md active:scale-95"
                                        }`}
                                >
                                    {countdown > 0 ? `${countdown}s 后重试` : (isSending ? "发送中..." : "获取验证码")}
                                </button>
                            </div>
                        </div>

                        {/* New Password */}
                        <div className="space-y-1.5 group">
                            <label className="text-xs font-bold text-ink-400 ml-1 uppercase tracking-wider group-focus-within:text-sage-600 transition-colors">新密码</label>
                            <div className="relative">
                                <Lock className="absolute left-4 top-1/2 -translate-y-1/2 text-sage-300 transition-colors group-focus-within:text-sage-500" size={18} />
                                <input
                                    type="password"
                                    value={password}
                                    onChange={(e) => setPassword(e.target.value)}
                                    className="w-full pl-11 pr-4 py-3.5 bg-white border border-sage-200 rounded-2xl text-sm font-bold text-ink-800 outline-none focus:border-sage-400 focus:ring-4 focus:ring-sage-100 transition-all shadow-sm placeholder-sage-300"
                                    placeholder="请输入新密码（至少6位）"
                                />
                            </div>
                        </div>

                        {/* Confirm Password */}
                        <div className="space-y-1.5 group">
                            <label className="text-xs font-bold text-ink-400 ml-1 uppercase tracking-wider group-focus-within:text-sage-600 transition-colors">确认密码</label>
                            <div className="relative">
                                <Lock className="absolute left-4 top-1/2 -translate-y-1/2 text-sage-300 transition-colors group-focus-within:text-sage-500" size={18} />
                                <input
                                    type="password"
                                    value={confirmPassword}
                                    onChange={(e) => setConfirmPassword(e.target.value)}
                                    className="w-full pl-11 pr-4 py-3.5 bg-white border border-sage-200 rounded-2xl text-sm font-bold text-ink-800 outline-none focus:border-sage-400 focus:ring-4 focus:ring-sage-100 transition-all shadow-sm placeholder-sage-300"
                                    placeholder="请再次输入新密码"
                                />
                            </div>
                        </div>

                        <button
                            onClick={handleSubmit}
                            disabled={isLoading}
                            className={`w-full py-4 mt-6 rounded-2xl text-white font-bold text-sm shadow-md transition-all active:scale-[0.98] ${isLoading
                                ? "bg-sage-300 cursor-not-allowed shadow-none"
                                : "bg-sage-600 hover:bg-sage-700 hover:shadow-lg shadow-sage-500/20"
                                }`}
                        >
                            {isLoading ? "提交中..." : "确认修改"}
                        </button>
                    </div>
                )}
            </div>
        </div>
    );
};

export default ChangePasswordModal;
