import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { Minus, X, ChevronLeft, ArrowRight } from 'lucide-react';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';
import Register from './Register';
import ResetPassword from './ResetPassword';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

type ViewState = 'login' | 'register' | 'reset';

const Login = ({ onLoginSuccess }: { onLoginSuccess: (data: any) => void }) => {
    const [view, setView] = useState<ViewState>('login');
    const [loginMode, setLoginMode] = useState<'password' | 'code'>('password');
    const [phone, setPhone] = useState('');
    const [password, setPassword] = useState('');
    const [status, setStatus] = useState('');
    const [countdown, setCountdown] = useState(0);
    const [autoLogin, setAutoLogin] = useState(false);
    const [rememberPwd, setRememberPwd] = useState(false);

    // Countdown logic
    useEffect(() => {
        if (countdown > 0) {
            const timer = setTimeout(() => setCountdown(countdown - 1), 1000);
            return () => clearTimeout(timer);
        }
    }, [countdown]);

    useEffect(() => {
        const prefsStr = localStorage.getItem('login_prefs');
        if (prefsStr) {
            try {
                const prefs = JSON.parse(prefsStr);
                setPhone(prefs.phone || '');
                if (prefs.rememberPwd) {
                    setPassword(prefs.password || '');
                    setRememberPwd(true);
                }
                if (prefs.autoLogin) {
                    setAutoLogin(true);
                    if (prefs.phone && prefs.password && prefs.rememberPwd) {
                        performLogin(prefs.phone, prefs.password);
                    }
                }
            } catch (e) { console.error(e); }
        }
    }, []);

    const handleMinimize = () => {
        getCurrentWindow().minimize();
    };

    const handleClose = () => {
        getCurrentWindow().close();
    };

    const handleGetCode = () => {
        if (!/^1[3-9]\d{9}$/.test(phone)) {
            setStatus('请输入有效的11位手机号码');
            return;
        }
        setCountdown(60);
        setStatus('验证码已发送');
    };

    const performLogin = async (currentPhone: string, currentPassword: string) => {
        try {
            setStatus('正在登录...');
            const response = await invoke('login', { phone: currentPhone, password: currentPassword }) as string;

            let json;
            try {
                json = JSON.parse(response);
            } catch (e) {
                setStatus('服务器响应错误');
                return;
            }

            if (json.data && json.data.code === 200) {
                setStatus('登录成功');
                console.log("Login Response Data:", json.data);
                localStorage.setItem('user_info', JSON.stringify({ ...json.data, phone: currentPhone }));
                setTimeout(() => onLoginSuccess({ ...json.data, phone: currentPhone }), 200);
            } else {
                setStatus(json.data?.message || '登录失败');
            }
        } catch (error) {
            setStatus('请求失败: ' + String(error));
        }
    };

    const handleLogin = async () => {
        if (!phone) {
            setStatus('请输入账号');
            return;
        }
        if (!password) {
            setStatus(loginMode === 'code' ? '请输入验证码' : '请输入密码');
            return;
        }

        if (rememberPwd) {
            localStorage.setItem('login_prefs', JSON.stringify({
                phone,
                password,
                rememberPwd,
                autoLogin
            }));
        } else {
            localStorage.setItem('login_prefs', JSON.stringify({
                phone,
                password: "",
                rememberPwd: false,
                autoLogin: false
            }));
        }

        await performLogin(phone, password);
    };

    const getTitle = () => {
        switch (view) {
            case 'register': return '注册账号';
            case 'reset': return '重置密码';
            default: return '教师助手';
        }
    };

    return (
        <div className="flex flex-col h-screen w-screen bg-paper rounded-[2rem] shadow-2xl overflow-hidden font-sans select-none border border-sage-200 relative">

            {/* Header Area - Warm Organic Style */}
            <div
                data-tauri-drag-region
                onMouseDown={() => getCurrentWindow().startDragging()}
                className="h-24 bg-white/50 relative shrink-0 cursor-move border-b border-sage-50"
            >
                {/* Logo & Text */}
                <div className="absolute top-6 left-6 flex items-center gap-3 z-20">
                    {view !== 'login' ? (
                        <button
                            onClick={() => setView('login')}
                            onMouseDown={(e) => e.stopPropagation()}
                            className="flex items-center gap-1 text-ink-600 hover:bg-sage-50 rounded-xl px-3 py-1.5 -ml-2 transition-all cursor-pointer active:scale-95 group"
                        >
                            <ChevronLeft size={20} className="text-sage-500 group-hover:-translate-x-0.5 transition-transform" />
                            <span className="text-ink-800 text-sm font-bold tracking-wide">{getTitle()}</span>
                        </button>
                    ) : (
                        <div className="flex items-center gap-3 pointer-events-none">
                            <div className="w-10 h-10 bg-gradient-to-br from-sage-400 to-sage-600 rounded-xl flex items-center justify-center shadow-lg shadow-sage-200">
                                <div className="w-4 h-4 bg-white rounded-full shadow-inner opacity-90"></div>
                            </div>
                            <div className="flex flex-col">
                                <span className="text-ink-800 text-base font-bold tracking-tight leading-none">教师助手</span>
                                <span className="text-sage-500 text-[10px] font-semibold tracking-wider mt-1 uppercase">Smart Education</span>
                            </div>
                        </div>
                    )}
                </div>

                {/* Window Controls - Organic Pills */}
                <div className="absolute top-4 right-4 flex items-center gap-2 z-20" onMouseDown={(e) => e.stopPropagation()}>
                    {view === 'login' && (
                        <button
                            onClick={() => setLoginMode(loginMode === 'password' ? 'code' : 'password')}
                            className="text-sage-600 text-xs px-4 py-1.5 rounded-full bg-white border border-sage-200 hover:border-sage-300 hover:bg-sage-50 transition-all active:scale-95 mr-2 font-bold shadow-sm"
                        >
                            {loginMode === 'password' ? "切换验证码登录" : "切换密码登录"}
                        </button>
                    )}
                    <button onClick={handleMinimize} className="p-2 hover:bg-sage-100 rounded-full text-sage-400 hover:text-sage-600 transition-colors">
                        <Minus size={18} />
                    </button>
                    <button onClick={handleClose} className="p-2 hover:bg-clay-100 hover:text-clay-600 rounded-full text-sage-400 transition-all">
                        <X size={18} />
                    </button>
                </div>
            </div>

            {/* Main Body - Clean & Warm */}
            <div className="flex-1 w-full relative flex flex-col items-center justify-center -mt-6">

                {/* Decorative Blobs */}
                <div className="absolute top-0 right-0 w-64 h-64 bg-sage-100/50 rounded-full blur-3xl -translate-y-1/2 translate-x-1/2 pointer-events-none"></div>
                <div className="absolute bottom-0 left-0 w-64 h-64 bg-clay-100/30 rounded-full blur-3xl translate-y-1/2 -translate-x-1/2 pointer-events-none"></div>

                {/* Card Container */}
                {view === 'login' && (
                    <div className="w-80 flex flex-col relative z-10 animate-in fade-in slide-in-from-bottom-4 duration-500">

                        <div className="text-center mb-8">
                            <h1 className="text-2xl font-bold text-ink-800 tracking-tight">欢迎回来</h1>
                            <p className="text-ink-400 text-sm mt-2">请登录您的教师账号</p>
                        </div>

                        {/* Input Group */}
                        <div className="flex flex-col gap-4">
                            <div className="group relative">
                                <input
                                    type="text"
                                    placeholder="手机号 / 账号"
                                    className="peer w-full py-3.5 px-5 bg-white border-2 border-transparent focus:border-sage-200 rounded-2xl outline-none text-ink-600 text-sm placeholder:text-ink-300 transition-all shadow-[0_2px_10px_rgba(0,0,0,0.02)] focus:shadow-[0_4px_20px_rgba(0,0,0,0.05)] hover:bg-white/80"
                                    value={phone}
                                    onChange={(e) => setPhone(e.target.value)}
                                />
                            </div>

                            <div className="group relative flex items-center">
                                <input
                                    type={loginMode === 'password' ? "password" : "text"}
                                    placeholder={loginMode === 'password' ? "密码" : "验证码"}
                                    className="peer w-full py-3.5 px-5 bg-white border-2 border-transparent focus:border-sage-200 rounded-2xl outline-none text-ink-600 text-sm placeholder:text-ink-300 transition-all shadow-[0_2px_10px_rgba(0,0,0,0.02)] focus:shadow-[0_4px_20px_rgba(0,0,0,0.05)] hover:bg-white/80"
                                    value={password}
                                    onChange={(e) => setPassword(e.target.value)}
                                />
                                {loginMode === 'code' && (
                                    <button
                                        onClick={handleGetCode}
                                        disabled={countdown > 0}
                                        className={cn(
                                            "absolute right-2 top-2 bottom-2 text-xs px-3 rounded-xl bg-sage-50 text-sage-600 font-bold hover:bg-sage-100 transition-all",
                                            countdown > 0 && "bg-gray-100 text-gray-400 cursor-not-allowed"
                                        )}
                                    >
                                        {countdown > 0 ? `${countdown}s` : '获取'}
                                    </button>
                                )}
                            </div>
                        </div>

                        {/* Options Row */}
                        <div className="flex justify-between items-center px-2 mt-5">
                            <label className="flex items-center gap-2.5 cursor-pointer group select-none">
                                <div className="relative flex items-center">
                                    <input
                                        type="checkbox"
                                        checked={rememberPwd}
                                        onChange={(e) => setRememberPwd(e.target.checked)}
                                        className="peer h-4 w-4 rounded-md border-2 border-sage-200 text-sage-500 focus:ring-sage-500/20 cursor-pointer transition-all checked:bg-sage-500 checked:border-sage-500"
                                    />
                                </div>
                                <span className="text-xs font-bold text-ink-400 group-hover:text-ink-600 transition-colors">记住密码</span>
                            </label>

                            <label className="flex items-center gap-2.5 cursor-pointer group select-none">
                                <div className="relative flex items-center">
                                    <input
                                        type="checkbox"
                                        checked={autoLogin}
                                        onChange={(e) => setAutoLogin(e.target.checked)}
                                        className="peer h-4 w-4 rounded-md border-2 border-sage-200 text-sage-500 focus:ring-sage-500/20 cursor-pointer transition-all checked:bg-sage-500 checked:border-sage-500"
                                    />
                                </div>
                                <span className="text-xs font-bold text-ink-400 group-hover:text-ink-600 transition-colors">自动登录</span>
                            </label>
                        </div>

                        {/* Action Button */}
                        <button
                            onClick={handleLogin}
                            className="w-full mt-8 bg-ink-800 hover:bg-ink-900 text-white py-3.5 rounded-2xl shadow-xl shadow-ink-200/50 text-sm font-bold tracking-wide transition-all active:scale-[0.98] hover:shadow-2xl hover:shadow-ink-300/50 flex items-center justify-center gap-2 group"
                        >
                            <span>进入课堂</span>
                            <ArrowRight size={16} className="text-sage-400 group-hover:translate-x-1 transition-transform" />
                        </button>

                        {/* Status Message */}
                        <div className={cn(
                            "text-center text-xs font-bold transition-all duration-300 h-6 mt-4 flex items-center justify-center",
                            status ? "opacity-100" : "opacity-0"
                        )}>
                            <span className={status.includes('成功') ? "text-sage-600 bg-sage-50 px-3 py-1 rounded-lg" : "text-clay-600 bg-clay-50 px-3 py-1 rounded-lg"}>
                                {status}
                            </span>
                        </div>

                        {/* Footer Links */}
                        <div className="absolute -bottom-16 left-0 right-0 flex justify-center pb-4">
                            <div className="flex justify-center items-center gap-6 text-xs font-bold text-ink-300">
                                <button onClick={() => setView('reset')} className="hover:text-sage-600 hover:underline transition-all">找回密码</button>
                                <div className="w-1 h-1 bg-sage-200 rounded-full"></div>
                                <button onClick={() => setView('register')} className="hover:text-sage-600 hover:underline transition-all">注册账号</button>
                            </div>
                        </div>
                    </div>
                )}

                {view === 'register' && (
                    <div className="relative z-10 w-80 pt-4 animate-in fade-in slide-in-from-right-8 duration-300">
                        <Register
                            onBack={() => setView('login')}
                            onSuccess={(data) => {
                                setView('login');
                                setPhone(data.phone || '');
                                setStatus('注册成功，请登录');
                            }}
                        />
                    </div>
                )}

                {view === 'reset' && (
                    <div className="relative z-10 w-80 pt-4 animate-in fade-in slide-in-from-left-8 duration-300">
                        <ResetPassword
                            onBack={() => setView('login')}
                            onSuccess={() => {
                                setView('login');
                                setStatus('密码重置成功，请登录');
                            }}
                        />
                    </div>
                )}

            </div>
        </div>
    );
};

export default Login;
