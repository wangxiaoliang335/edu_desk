import { useState } from 'react';
import Taskbar, { TaskbarItemType } from './Taskbar';
import { Minus, X, Square, Copy } from 'lucide-react'; // Square for Maximize, Copy for Restore (simulated)
import { getCurrentWindow } from '@tauri-apps/api/window';
import TeacherSchedule from './TeacherSchedule';
import ClassManagement from './ClassManagement';


interface DashboardProps {
    userInfo: any;
}

const Dashboard = ({ userInfo }: DashboardProps) => {
    const [activeApp, setActiveApp] = useState<TaskbarItemType | null>(null);
    const [isMaximized, setIsMaximized] = useState(false);

    const handleMinimize = () => getCurrentWindow().minimize();
    const handleClose = () => getCurrentWindow().close();
    const handleMaximize = async () => {
        const win = getCurrentWindow();
        const max = await win.isMaximized();
        if (max) {
            win.unmaximize();
            setIsMaximized(false);
        } else {
            win.maximize();
            setIsMaximized(true);
        }
    };

    const handleToolClick = (toolId: string) => {
        if (toolId === 'schedule') {
            setActiveApp('schedule');
        } else if (toolId === 'class_mgr') {
            setActiveApp('class');
        } else {
            console.log('Tool clicked:', toolId);
        }
    };

    // Placeholder content generators
    const renderContent = () => {
        if (!activeApp) return null;

        // Windows 11 Internal Window
        // Animation: Fly upwards slightly + Fade
        return (
            <div className="absolute inset-4 bottom-20 bg-white/95 backdrop-blur-3xl rounded-lg shadow-[0_0_20px_rgba(0,0,0,0.1),0_0_0_1px_rgba(0,0,0,0.05)] flex flex-col overflow-hidden animate-in fade-in slide-in-from-bottom-4 duration-200">
                {/* Window Header */}
                <div className="h-10 flex items-center justify-between px-4 shrink-0 bg-transparent select-none draggable-region">
                    <div className="flex items-center gap-3">
                        <span className="text-sm font-semibold text-gray-700">
                            {activeApp === 'folder' && '资源云盘'}
                            {activeApp === 'class' && '班级管理'}
                            {activeApp === 'schedule' && '课程表'}
                            {activeApp === 'user' && '个人中心'}
                        </span>
                    </div>
                    {/* Internal Window Controls (simulated) */}
                    <div className="flex items-center">
                        <button className="w-9 h-8 flex items-center justify-center hover:bg-black/5 rounded text-gray-500" onClick={() => setActiveApp(null)}>
                            <Minus size={14} />
                        </button>
                        <button className="w-9 h-8 flex items-center justify-center hover:bg-black/5 rounded text-gray-500">
                            <Square size={12} />
                        </button>
                        <button className="w-9 h-8 flex items-center justify-center hover:bg-red-600 hover:text-white rounded text-gray-500 transition-colors" onClick={() => setActiveApp(null)}>
                            <X size={16} />
                        </button>
                    </div>
                </div>

                {/* Window Body */}
                <div className="flex-1 p-6 overflow-auto bg-[#f9f9f9]/80">
                    {activeApp === 'folder' && (
                        <div className="h-full flex flex-col">
                            <h2 className="text-xl font-semibold text-gray-800 mb-6 px-2">桌面管理</h2>
                            <div className="grid grid-cols-4 gap-4">
                                {[
                                    { id: 'file_manager', name: '文件管理', icon: '📂', color: 'bg-blue-100 text-blue-600' },
                                    { id: 'new_folder', name: '创建文件夹', icon: '➕', color: 'bg-yellow-100 text-yellow-600' },
                                    { id: 'countdown', name: '倒计时', icon: '⏳', color: 'bg-red-100 text-red-600' },
                                    { id: 'time', name: '时间', icon: '🕒', color: 'bg-purple-100 text-purple-600' },
                                    { id: 'class_mgr', name: '学校/班级', icon: '🏫', color: 'bg-emerald-100 text-emerald-600' },
                                    { id: 'wallpaper', name: '壁纸', icon: '🖼️', color: 'bg-pink-100 text-pink-600' },
                                    { id: 'schedule', name: '教师课程表', icon: '📅', color: 'bg-orange-100 text-orange-600' },
                                    { id: 'calendar', name: '校历', icon: '🗓️', color: 'bg-cyan-100 text-cyan-600' },
                                    { id: 'table', name: '表格', icon: '📊', color: 'bg-gray-100 text-gray-600' },
                                    { id: 'text', name: '文本', icon: '📝', color: 'bg-indigo-100 text-indigo-600' },
                                    { id: 'image', name: '图片', icon: '🏞️', color: 'bg-rose-100 text-rose-600' },
                                ].map((tool) => (
                                    <button
                                        key={tool.id}
                                        onClick={() => handleToolClick(tool.id)}
                                        className="flex flex-col items-center justify-center p-4 rounded-xl hover:bg-white hover:shadow-sm transition-all duration-200 group active:scale-95 border border-transparent hover:border-gray-200"
                                    >
                                        <div className={`w-14 h-14 rounded-2xl flex items-center justify-center text-3xl mb-3 shadow-sm ${tool.color} group-hover:scale-110 transition-transform duration-300`}>
                                            {tool.icon}
                                        </div>
                                        <span className="text-sm font-medium text-gray-700 group-hover:text-gray-900">{tool.name}</span>
                                    </button>
                                ))}
                            </div>
                        </div>
                    )}
                    {activeApp === 'schedule' && (
                        <div className="h-full flex flex-col">
                            <TeacherSchedule />
                        </div>
                    )}
                    {activeApp === 'class' && (
                        <div className="h-full flex flex-col">
                            <ClassManagement userInfo={userInfo} />
                        </div>
                    )}
                    {activeApp === 'user' && (
                        <div className="h-full flex flex-col items-center justify-center text-gray-400 space-y-4">
                            <div className="w-24 h-24 rounded-full overflow-hidden border-2 border-gray-100 shadow-lg">
                                <img src={userInfo?.avatar || "https://api.dicebear.com/7.x/avataaars/svg?seed=Felix"} alt="Avatar" className="w-full h-full object-cover" />
                            </div>
                            <div className="text-center">
                                <h2 className="text-2xl font-bold text-gray-800">{userInfo?.name || '未知用户'}</h2>
                                <p className="text-gray-500">{userInfo?.school_name || '未知学校'} · {userInfo?.grade_level || ''}</p>
                            </div>

                            <div className="grid grid-cols-2 gap-4 mt-8 w-full max-w-md">
                                <div className="p-4 bg-white rounded-lg shadow-sm border border-gray-100 text-center hover:border-blue-200 transition-colors cursor-default">
                                    <div className="text-lg font-bold text-gray-800">12</div>
                                    <div className="text-xs text-gray-500">待办事项</div>
                                </div>
                                <div className="p-4 bg-white rounded-lg shadow-sm border border-gray-100 text-center hover:border-blue-200 transition-colors cursor-default">
                                    <div className="text-lg font-bold text-gray-800">5</div>
                                    <div className="text-xs text-gray-500">我的消息</div>
                                </div>
                            </div>
                        </div>
                    )}
                </div>
            </div>
        );
    };

    return (
        <div className={`h-screen w-screen overflow-hidden relative font-['Segoe_UI',system-ui] select-none text-gray-800 bg-[#f3f3f3] transition-all duration-300 ${!isMaximized ? 'rounded-2xl border border-gray-400/30 shadow-[0_0_15px_rgba(0,0,0,0.3)]' : ''}`}>
            {/* Windows 11 Mica-like Background */}
            <div className="absolute inset-0 z-0 bg-cover bg-center opacity-80" style={{ backgroundImage: 'url("https://4.bing.com/th?id=OHR.BlueHourParis_EN-US8633396684_1920x1080.jpg&rf=LaDigue_1920x1080.jpg")' }}></div>
            <div className="absolute inset-0 z-0 bg-white/40 backdrop-blur-[50px]"></div>

            {/* Custom Title Bar (Windows Style) */}
            <div data-tauri-drag-region className="absolute top-0 left-0 right-0 h-8 flex items-center justify-between z-[100]">
                {/* Title */}
                <div className="px-3 text-xs text-gray-600 flex items-center gap-2 pointer-events-none">
                    <span className="font-semibold">Teacher Assistant</span>
                </div>

                {/* Window Controls (Windows 11 Style) */}
                <div className="flex items-start h-full">
                    <button onClick={handleMinimize} className="w-11 h-full flex items-center justify-center hover:bg-black/5 hover:text-black text-gray-600 transition-colors active:bg-black/10">
                        <Minus size={16} strokeWidth={1.5} />
                    </button>
                    <button onClick={handleMaximize} className="w-11 h-full flex items-center justify-center hover:bg-black/5 hover:text-black text-gray-600 transition-colors active:bg-black/10">
                        {isMaximized ? <Copy size={14} className="rotate-180" strokeWidth={1.5} /> : <Square size={14} strokeWidth={1.5} />}
                    </button>
                    <button onClick={handleClose} className="w-11 h-full flex items-center justify-center hover:bg-[#c42b1c] hover:text-white text-gray-600 transition-colors active:bg-[#b02619]">
                        <X size={16} strokeWidth={1.5} />
                    </button>
                </div>
            </div>

            {/* Main Desktop Area */}
            <div className="absolute inset-0 z-10 pt-8 pb-14">
                {/* Welcome Message (if no app open) */}
                {activeApp === null && (
                    <div className="h-full flex flex-col items-center justify-center text-center space-y-4 animate-in fade-in duration-500">
                        <div className="w-20 h-20 rounded-full bg-blue-100 flex items-center justify-center mb-4 text-blue-600 font-bold text-2xl shadow-xl">
                            {userInfo?.name?.[0] || 'T'}
                        </div>
                        <h1 className="text-3xl font-semibold text-gray-800 drop-shadow-sm">
                            {new Date().getHours() < 12 ? '早上好' : '下午好'}，{userInfo?.name || '老师'}
                        </h1>
                        <p className="text-gray-600 text-base">
                            {new Date().toLocaleDateString(undefined, { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' })}
                        </p>
                    </div>
                )}

                {renderContent()}
            </div>

            {/* Taskbar */}
            <Taskbar activeItem={activeApp} onItemClick={setActiveApp} />
        </div>
    );
};

export default Dashboard;
