import { useState, useEffect } from 'react';
import Taskbar, { TaskbarItemType } from './Taskbar';
import { Minus, X, Square, Copy, LogOut } from 'lucide-react'; // Square for Maximize, Copy for Restore (simulated)
import { getCurrentWindow } from '@tauri-apps/api/window';
import TeacherSchedule from './TeacherSchedule';
import ClassManagement from './ClassManagement';
import CreateClassGroupModal from './modals/CreateClassGroupModal';
import CreateNormalGroupModal from './modals/CreateNormalGroupModal';
import SearchAddModal from './modals/SearchAddModal';
import UserInfoModal from './modals/UserInfoModal';
import ChangePasswordModal from './modals/ChangePasswordModal';
import SchoolInfoModal from './modals/SchoolInfoModal';
import SchoolCourseScheduleModal from './modals/SchoolCourseScheduleModal';
import SchoolCalendar from './SchoolCalendar';


import { invoke } from '@tauri-apps/api/core';

interface DashboardProps {
    userInfo: any;
}

const Dashboard = ({ userInfo }: DashboardProps) => {
    const [activeApp, setActiveApp] = useState<TaskbarItemType | null>(null);
    const [isMaximized, setIsMaximized] = useState(false);

    // Modal States
    const [showCreateClassGroup, setShowCreateClassGroup] = useState(false);
    const [showCreateNormalGroup, setShowCreateNormalGroup] = useState(false);
    const [showSearchAdd, setShowSearchAdd] = useState(false);
    const [showUserInfo, setShowUserInfo] = useState(false);
    const [showChangePassword, setShowChangePassword] = useState(false);
    const [showSchoolInfo, setShowSchoolInfo] = useState(false);
    const [showSchoolSchedule, setShowSchoolSchedule] = useState(false);
    const [showSchoolCalendar, setShowSchoolCalendar] = useState(false);

    // Countdown Config (Now managed mostly by the widget itself, but checking if we need to auto-launch)
    // Actually, if it's a separate window, Dashboard doesn't need to know its config.
    // Dashboard just launches it.
    // But auto-launch?
    // User said: "If it was there (active), it should be there on startup".
    // "On Startup" usually means when Dashboard loads, it checks if it should launch the widget.
    // So we DO need to read 'countdown_visible' and launch it if true.

    useEffect(() => {
        const shouldShow = localStorage.getItem('countdown_visible') === 'true';
        if (shouldShow) {
            invoke('open_countdown_minimal_window');
        }
    }, []);

    const handleMinimize = () => getCurrentWindow().minimize();
    const handleClose = () => invoke('exit_app');
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

    const handleToolClick = async (toolId: string) => {
        if (toolId === 'schedule') {
            try {
                await invoke('open_teacher_schedule_window');
            } catch (e) {
                console.error(e);
                alert('打开教师课程表失败');
            }
        } else if (toolId === 'class_mgr') {
            setActiveApp('class');
        } else if (toolId === 'create_class_group') {
            setShowCreateClassGroup(true);
        } else if (toolId === 'create_normal_group') {
            setShowCreateNormalGroup(true);
        } else if (toolId === 'search_add') {
            setShowSearchAdd(true);
        } else if (toolId === 'school_info') {
            setShowSchoolInfo(true);
        } else if (toolId === 'school_course_schedule') {
            setShowSchoolSchedule(true);
        } else if (toolId === 'calendar') {
            setShowSchoolCalendar(true);
        } else if (toolId === 'file_manager') {
            try {
                const { createNewBox } = await import('../utils/DesktopManager');
                const box = await createNewBox();
                await invoke('open_file_box_window', { boxId: box.id });
                // Optional: Minimize main window if desired, or keep as is
            } catch (e) {
                console.error(e);
                alert('创建文件盒子失败');
            }
        } else if (toolId === 'countdown') {
            console.log('Invoking open_countdown_edit_window...');
            invoke('open_countdown_edit_window').then(() => console.log('Invoke success')).catch(e => console.error('Invoke failed:', e));
            localStorage.setItem('countdown_visible', 'true');
        } else {
            console.log('Tool clicked:', toolId);
        }
    };

    const handleLogout = () => {
        if (confirm("确定要退出登录吗？")) {
            // Update login preferences to disable auto login
            const prefsStr = localStorage.getItem('login_prefs');
            if (prefsStr) {
                try {
                    const prefs = JSON.parse(prefsStr);
                    prefs.autoLogin = false;
                    localStorage.setItem('login_prefs', JSON.stringify(prefs));
                } catch (e) {
                    console.error("Failed to update login prefs", e);
                }
            }

            // Clear credentials
            localStorage.removeItem('token');

            // Reload to reset app state and trigger Login view
            window.location.reload();
        }
    };

    // Placeholder content generators
    const renderContent = () => {
        if (!activeApp) return null;

        // Windows 11 Internal Window
        // Animation: Fly upwards slightly + Fade
        return (
            <div className="absolute inset-4 bottom-24 bg-white rounded-3xl shadow-[0_8px_30px_rgb(0,0,0,0.04)] border border-sage-100 flex flex-col overflow-hidden animate-in fade-in slide-in-from-bottom-4 duration-300">
                {/* Window Header */}
                <div className="h-12 flex items-center justify-between px-6 shrink-0 bg-white border-b border-sage-50 select-none draggable-region">
                    <div className="flex items-center gap-3">
                        <span className="text-base font-bold text-ink-800 tracking-wide">
                            {activeApp === 'folder' && '桌面资源'}
                            {activeApp === 'class' && '班级管理'}
                            {activeApp === 'schedule' && '我的课表'}
                            {activeApp === 'user' && '个人中心'}
                        </span>
                    </div>
                    {/* Internal Window Controls (simulated) */}
                    <div className="flex items-center gap-2">
                        <button className="w-8 h-8 flex items-center justify-center hover:bg-sage-50 rounded-full text-ink-400 transition-colors" onClick={() => setActiveApp(null)}>
                            <Minus size={16} />
                        </button>
                        <button className="w-8 h-8 flex items-center justify-center hover:bg-sage-50 rounded-full text-ink-400 transition-colors">
                            <Square size={14} />
                        </button>
                        <button className="w-8 h-8 flex items-center justify-center hover:bg-clay-100 hover:text-clay-600 rounded-full text-ink-400 transition-colors" onClick={() => setActiveApp(null)}>
                            <X size={18} />
                        </button>
                    </div>
                </div>

                {/* Window Body */}
                <div className="flex-1 p-8 overflow-auto bg-paper">
                    {activeApp === 'folder' && (
                        <div className="h-full flex flex-col">
                            <h2 className="text-2xl font-bold text-ink-800 mb-8 px-2 tracking-tight">桌面资源</h2>
                            <div className="grid grid-cols-4 gap-6">
                                {[
                                    { id: 'file_manager', name: '文件管理', icon: '📂', color: 'bg-orange-100 text-orange-600' },
                                    { id: 'new_folder', name: '新建文件夹', icon: '➕', color: 'bg-sage-100 text-sage-600' },
                                    { id: 'countdown', name: '倒计时', icon: '⏳', color: 'bg-clay-500/10 text-clay-600' },
                                    { id: 'time', name: '时钟', icon: '🕒', color: 'bg-blue-100 text-blue-600' },
                                    { id: 'school_info', name: '学校配置', icon: '🏫', color: 'bg-emerald-100 text-emerald-600' },
                                    { id: 'wallpaper', name: '壁纸设置', icon: '🖼️', color: 'bg-pink-100 text-pink-600' },
                                    { id: 'schedule', name: '我的课表', icon: '📅', color: 'bg-amber-100 text-amber-600' },
                                    { id: 'school_course_schedule', name: '班级课表', icon: '📅', color: 'bg-amber-200 text-amber-700' },
                                    { id: 'calendar', name: '校历日程', icon: '🗓️', color: 'bg-cyan-100 text-cyan-600' },
                                    // New Tools
                                    { id: 'create_class_group', name: '创建班级群', icon: '🎓', color: 'bg-blue-100 text-blue-600' },
                                    { id: 'create_normal_group', name: '创建讨论组', icon: '💬', color: 'bg-sage-200 text-sage-700' },
                                    { id: 'search_add', name: '查找添加', icon: '🔍', color: 'bg-purple-100 text-purple-600' },
                                ].map((tool) => (
                                    <button
                                        key={tool.id}
                                        onClick={() => handleToolClick(tool.id)}
                                        className="flex flex-col items-center justify-center p-6 rounded-2xl bg-white shadow-[0_2px_10px_rgb(0,0,0,0.03)] hover:shadow-[0_8px_20px_rgb(0,0,0,0.06)] hover:-translate-y-1 transition-all duration-300 group border border-transparent hover:border-sage-200"
                                    >
                                        <div className={`w-16 h-16 rounded-2xl flex items-center justify-center text-3xl mb-4 ${tool.color} group-hover:scale-110 transition-transform duration-300`}>
                                            {tool.icon}
                                        </div>
                                        <span className="text-base font-bold text-ink-600 group-hover:text-ink-800 tracking-wide">{tool.name}</span>
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
                        <div className="h-full flex flex-col items-center justify-center space-y-8 animate-in zoom-in-95 duration-300">

                            {/* Avatar Section */}
                            <div className="relative group cursor-pointer" onClick={() => setShowUserInfo(true)}>
                                <div className="w-32 h-32 rounded-full overflow-hidden border-4 border-white shadow-2xl transition-transform duration-300 group-hover:scale-105">
                                    <img
                                        src={userInfo?.avatar || "https://api.dicebear.com/7.x/avataaars/svg?seed=Felix"}
                                        alt="Avatar"
                                        className="w-full h-full object-cover"
                                    />
                                    <div className="absolute inset-0 bg-black/30 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity">
                                        <span className="text-white text-xs font-bold">查看资料</span>
                                    </div>
                                </div>
                                <div className="absolute bottom-1 right-1 w-6 h-6 bg-green-500 border-4 border-white rounded-full"></div>
                            </div>

                            {/* Info Section */}
                            <div className="text-center space-y-2">
                                <h2 className="text-3xl font-bold text-gray-800 tracking-tight">{userInfo?.name || '未知用户'}</h2>
                                <div className="flex flex-col gap-1 text-gray-500 font-medium">
                                    <span className="bg-blue-50 text-blue-600 px-3 py-1 rounded-full text-sm inline-block mx-auto">
                                        {userInfo?.school_name || '未知学校'}
                                    </span>
                                    <span className="text-sm mt-1">
                                        ID: {userInfo?.teacher_unique_id || userInfo?.id || '---'}
                                    </span>
                                </div>
                            </div>

                            {/* Actions Section */}
                            <div className="pt-4">
                                <button
                                    onClick={handleLogout}
                                    className="flex items-center gap-2 px-8 py-3 bg-red-50 hover:bg-red-100 text-red-600 rounded-xl font-bold transition-all hover:shadow-md active:scale-95"
                                >
                                    <LogOut size={20} />
                                    <span>退出登录</span>
                                </button>
                            </div>
                        </div>
                    )}

                </div>
            </div >
        );
    };

    return (
        <div className={`h-screen w-screen overflow-hidden relative font-sans select-none text-ink-600 bg-paper transition-all duration-300 ${!isMaximized ? 'rounded-[2rem] border border-sage-200 shadow-2xl' : ''}`}>
            {/* Background - Organic Paper Texture */}
            <div className="absolute inset-0 z-0 bg-paper">
                <div className="absolute top-0 right-0 w-[500px] h-[500px] bg-sage-50/50 rounded-full blur-[100px] -translate-y-1/2 translate-x-1/2"></div>
                <div className="absolute bottom-0 left-0 w-[600px] h-[600px] bg-clay-500/5 rounded-full blur-[120px] translate-y-1/2 -translate-x-1/3"></div>
            </div>

            {/* Custom Title Bar (Clean) */}
            <div data-tauri-drag-region className="absolute top-0 left-0 right-0 h-10 flex items-center justify-between z-[100] px-4">
                {/* Title */}
                <div className="text-xs font-bold text-ink-400 flex items-center gap-2 pointer-events-none uppercase tracking-widest">
                    Teacher Assistant
                </div>

                {/* Window Controls */}
                <div className="flex items-center gap-2 h-full py-2">
                    <button onClick={handleMinimize} className="w-8 h-8 flex items-center justify-center rounded-full hover:bg-sage-100 text-ink-400 transition-colors">
                        <Minus size={16} strokeWidth={2} />
                    </button>
                    <button onClick={handleMaximize} className="w-8 h-8 flex items-center justify-center rounded-full hover:bg-sage-100 text-ink-400 transition-colors">
                        {isMaximized ? <Copy size={14} className="rotate-180" strokeWidth={2} /> : <Square size={14} strokeWidth={2} />}
                    </button>
                    <button onClick={handleClose} className="w-8 h-8 flex items-center justify-center rounded-full hover:bg-clay-100 hover:text-clay-600 text-ink-400 transition-colors">
                        <X size={16} strokeWidth={2} />
                    </button>
                </div>
            </div>

            {/* Main Desktop Area */}
            <div className="absolute inset-0 z-10 pt-12 pb-24 px-8">
                {/* Welcome Message (if no app open) */}
                {activeApp === null && (
                    <div className="h-full flex flex-col items-center justify-center text-center -mt-10 animate-in fade-in duration-700 slide-in-from-bottom-8">
                        <div className="w-24 h-24 rounded-3xl bg-white shadow-[0_8px_30px_rgb(0,0,0,0.04)] flex items-center justify-center mb-8 border border-sage-50 relative overflow-hidden group hover:scale-105 transition-transform duration-500">
                            {(userInfo?.avatar || userInfo?.icon || userInfo?.face_url) ? (
                                <img
                                    src={userInfo?.avatar || userInfo?.icon || userInfo?.face_url}
                                    alt="Avatar"
                                    className="w-full h-full object-cover"
                                />
                            ) : (
                                <div className="w-full h-full bg-gradient-to-br from-sage-50 to-sage-100 flex items-center justify-center text-sage-500 font-bold text-3xl">
                                    {userInfo?.name?.[0] || '师'}
                                </div>
                            )}
                            {/* Shine effect */}
                            <div className="absolute inset-0 bg-white/20 translate-x-[-100%] group-hover:translate-x-[100%] transition-transform duration-1000 rotate-45 pointer-events-none"></div>
                        </div>
                        <h1 className="text-4xl font-bold text-ink-800 mb-3 tracking-tight">
                            {new Date().getHours() < 12 ? '早上好' : '下午好'}，{userInfo?.name || '老师'}
                        </h1>
                        <p className="text-ink-400 text-lg font-medium">
                            {new Date().toLocaleDateString('zh-CN', { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' })}
                        </p>
                    </div>
                )}

                {renderContent()}
            </div>

            {/* Taskbar */}
            <Taskbar activeItem={activeApp} onItemClick={setActiveApp} />

            {/* Modals */}
            <CreateClassGroupModal
                isOpen={showCreateClassGroup}
                onClose={() => setShowCreateClassGroup(false)}
                userInfo={userInfo}
                onSuccess={() => window.dispatchEvent(new CustomEvent('refresh-class-list'))}
            />
            <CreateNormalGroupModal
                isOpen={showCreateNormalGroup}
                onClose={() => setShowCreateNormalGroup(false)}
                userInfo={userInfo}
                onSuccess={() => window.dispatchEvent(new CustomEvent('refresh-class-list'))}
            />
            <SearchAddModal
                isOpen={showSearchAdd}
                onClose={() => setShowSearchAdd(false)}
                userInfo={userInfo}
            />
            <UserInfoModal
                isOpen={showUserInfo}
                onClose={() => setShowUserInfo(false)}
                userInfo={userInfo}
                onChangePassword={() => {
                    setShowUserInfo(false);
                    setShowChangePassword(true);
                }}
            />
            <ChangePasswordModal
                isOpen={showChangePassword}
                onClose={() => setShowChangePassword(false)}
                defaultPhone={userInfo?.phone}
            />
            <SchoolInfoModal
                isOpen={showSchoolInfo}
                onClose={() => setShowSchoolInfo(false)}
                userInfo={userInfo}
            />
            <SchoolCourseScheduleModal
                isOpen={showSchoolSchedule}
                onClose={() => setShowSchoolSchedule(false)}
                userInfo={userInfo}
            />
            {showSchoolCalendar && (
                <div className="absolute inset-0 z-50 flex items-center justify-center bg-black/30 backdrop-blur-[2px]">
                    <div className="pointer-events-auto">
                        <SchoolCalendar onClose={() => setShowSchoolCalendar(false)} />
                    </div>
                </div>
            )}
        </div>
    );
};

export default Dashboard;
