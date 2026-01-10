import { useEffect, useState } from 'react';
import { useParams } from 'react-router-dom';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { invoke } from '@tauri-apps/api/core';
import { Minus, X, Square, Copy, MessageCircle, Calendar, Users, BookOpen, Shuffle, Clock, Grid, LayoutDashboard, Layers, Award, Power, Mic, FileSpreadsheet } from 'lucide-react';
import { useWebSocket } from '../context/WebSocketContext';
import RandomCallModal from './modals/RandomCallModal';
import HomeworkModal from './modals/HomeworkModal';
import CountdownModal from './modals/CountdownModal';
import CourseScheduleModal from './modals/CourseScheduleModal';
import DutyRosterModal from './modals/DutyRosterModal';
import GroupScoreModal from './modals/GroupScoreModal';
import ClassInfoModal from './modals/ClassInfoModal';
import CustomListModal from './modals/CustomListModal';
import NotificationModal from './modals/NotificationModal';
import SeatMap from './SeatMap';
import { sendMessage } from '../utils/tim';

const ClassScheduleWindow = () => {
    const { groupclassId } = useParams();
    const [isMaximized, setIsMaximized] = useState(false);

    // WebSocket
    const { sendMessage: sendSignal, isConnected: isWSConnected } = useWebSocket();

    // View State
    const [currentView, setCurrentView] = useState<'overview' | 'seatmap'>('overview');
    const [heatmapMode, setHeatmapMode] = useState<'none' | 'score' | 'attendance'>('none');

    // Modal State
    const [isRandomCallOpen, setIsRandomCallOpen] = useState(false);
    const [isHomeworkOpen, setIsHomeworkOpen] = useState(false);
    const [isCountdownOpen, setIsCountdownOpen] = useState(false);
    const [isDutyRosterOpen, setIsDutyRosterOpen] = useState(false);
    const [isGroupScoreOpen, setIsGroupScoreOpen] = useState(false);
    const [isCourseScheduleOpen, setIsCourseScheduleOpen] = useState(false);
    const [isClassInfoOpen, setIsClassInfoOpen] = useState(false);
    const [isCustomListOpen, setIsCustomListOpen] = useState(false);
    const [isNotificationOpen, setIsNotificationOpen] = useState(false);

    useEffect(() => {
        const checkMaximized = async () => {
            const win = getCurrentWindow();
            setIsMaximized(await win.isMaximized());
        };
        checkMaximized();

        const unlisten = getCurrentWindow().onResized(async () => {
            const win = getCurrentWindow();
            setIsMaximized(await win.isMaximized());
        });

        return () => {
            unlisten.then(f => f());
        }
    }, []);

    const handleMinimize = () => getCurrentWindow().minimize();
    const handleMaximize = async () => {
        const win = getCurrentWindow();
        if (await win.isMaximized()) {
            await win.unmaximize();
        } else {
            await win.maximize();
        }
    };
    const handleClose = () => getCurrentWindow().close();

    const handleOpenChat = () => {
        invoke('open_chat_window', { groupclassId });
    };

    const handleOpenIntercom = () => {
        if (groupclassId) {
            invoke('open_intercom_window', { groupId: groupclassId });
        }
    };

    const handlePublishHomework = async (data: { subject: string; content: string; date: string }) => {
        console.log("Publishing Homework:", data);
        // Mock Implementation: Send notification to Chat
        if (groupclassId) {
            try {
                const msgText = `[作业发布] ${data.subject} (${data.date}): ${data.content}`;
                await sendMessage(groupclassId, msgText);
                alert("作业发布成功！(已同步到班级群)");
            } catch (e) {
                console.error(e);
                alert("作业发布失败");
            }
        }
    };

    const handleRemoteShutdown = async () => {
        if (confirm("确定要关闭当前班级设备吗？此操作将关闭计算机。")) {
            try {
                // Construct logic from ScheduleDialog.h:2658
                const userInfoStr = localStorage.getItem('user_info');
                let userInfo = {} as any;
                if (userInfoStr) {
                    try { userInfo = JSON.parse(userInfoStr); } catch (e) { }
                }

                if (!groupclassId || !userInfo.teacher_unique_id) {
                    alert("信息不完整，无法发送关机指令");
                    return;
                }

                const payload = {
                    type: "remote_shutdown",
                    action: "shutdown",
                    class_id: "", // TODO: Need class_id, but current param is groupclassId (which is group_id). 
                    // Qt checks m_classid. In frontend we only have groupclassId from URL (which is groupId).
                    // We might need to fetch class info to get class_id, or maybe groupclassId IS adequate or mapped.
                    // For now use groupclassId as group_id.
                    group_id: groupclassId,
                    sender_id: userInfo.teacher_unique_id,
                    sender_name: userInfo.strName || userInfo.name || "Teacher",
                    timestamp: Math.floor(Date.now() / 1000)
                };

                // The socket expects "to:{TargetId}:{JSON}"
                // C++: "to:%1:%2".arg(m_unique_group_id, compactJson)
                const msg = `to:${groupclassId}:${JSON.stringify(payload)}`; /* No extra spaces to match Compact JSON */

                sendSignal(msg);
                alert("关机指令已发送");

            } catch (e) {
                console.error(e);
                alert("关机指令发送失败: " + String(e));
            }
        }
    };

    return (
        <div className="h-screen w-screen bg-[#f8fbff] flex flex-col overflow-hidden border border-gray-300 select-none text-gray-700 font-sans">
            {/* Title Bar */}
            <div data-tauri-drag-region className="h-10 bg-white/50 backdrop-blur-md flex items-center justify-between px-4 border-b border-gray-200/50 z-50">
                <div className="flex items-center gap-4">
                    <div className="font-bold text-gray-700 flex items-center gap-2">
                        <div className="bg-blue-500 text-white p-1 rounded-md shadow-sm shadow-blue-200"><Users size={12} /></div>
                        <span className="text-sm tracking-wide">班级空间 - {groupclassId}</span>
                    </div>
                    {/* View Switcher */}
                    <div className="flex bg-gray-100/80 p-0.5 rounded-lg border border-gray-200/50">
                        <button
                            onClick={() => setCurrentView('overview')}
                            className={`px-2 py-0.5 text-xs rounded-md flex items-center gap-1 transition-all ${currentView === 'overview' ? 'bg-white text-blue-600 shadow-sm' : 'text-gray-500 hover:text-gray-700'}`}
                        >
                            <LayoutDashboard size={12} /> 概览
                        </button>
                        <button
                            onClick={() => setCurrentView('seatmap')}
                            className={`px-2 py-0.5 text-xs rounded-md flex items-center gap-1 transition-all ${currentView === 'seatmap' ? 'bg-white text-blue-600 shadow-sm' : 'text-gray-500 hover:text-gray-700'}`}
                        >
                            <Grid size={12} /> 座位表
                        </button>
                    </div>
                </div>
                <div className="flex items-center gap-1.5 opacity-80 hover:opacity-100 transition-opacity">
                    <button onClick={() => setIsClassInfoOpen(true)} className="px-2 py-0.5 mr-2 rounded text-gray-500 hover:bg-gray-200 font-bold">...</button>
                    <button onClick={handleMinimize} className="p-1 hover:bg-gray-200 rounded text-gray-500"><Minus size={14} /></button>
                    <button onClick={handleMaximize} className="p-1 hover:bg-gray-200 rounded text-gray-500">{isMaximized ? <Copy size={14} /> : <Square size={14} />}</button>
                    <button onClick={handleClose} className="p-1 hover:bg-red-500 hover:text-white rounded text-gray-500"><X size={14} /></button>
                </div>
            </div>

            {/* Main Content Area - Fixed Layout (No Scroll) */}
            <div className="flex-1 overflow-hidden p-4 md:p-6 relative flex flex-col gap-4">
                {currentView === 'overview' ? (
                    <div className="flex-1 grid grid-cols-12 gap-4 h-full">
                        {/* Left Column (30%) - Class Info & Notifications */}
                        <div className="col-span-3 flex flex-col gap-4 h-full">
                            {/* Hero Card */}
                            <div className="bg-white p-5 rounded-2xl border border-gray-100 shadow-sm relative overflow-hidden flex-shrink-0">
                                <div className="absolute top-0 right-0 w-32 h-32 bg-gradient-to-br from-blue-50 to-indigo-50 rounded-bl-full -mr-8 -mt-8 opacity-50 pointer-events-none"></div>
                                <div className="relative z-10">
                                    <h1 className="text-xl font-bold text-gray-800 mb-1">高二 (3) 班</h1>
                                    <p className="text-sm text-gray-500 font-medium">数学</p>
                                    <div className="mt-4 flex items-center gap-3 text-xs text-gray-400">
                                        <span className="flex items-center gap-1"><Users size={12} /> 45 人</span>
                                        <span className="w-px h-3 bg-gray-200"></span>
                                        <span className="flex items-center gap-1"><Calendar size={12} /> 2023-2024</span>
                                    </div>
                                </div>
                            </div>

                            {/* Notifications Card */}
                            <div
                                onClick={() => setIsNotificationOpen(true)}
                                className="flex-1 bg-gradient-to-br from-indigo-500 to-purple-600 rounded-2xl p-5 text-white shadow-lg shadow-purple-200 flex flex-col relative overflow-hidden group cursor-pointer transition-transform hover:-translate-y-0.5 active:translate-y-0"
                            >
                                <div className="absolute top-0 right-0 w-32 h-32 bg-white/10 rounded-full blur-2xl -mr-10 -mt-10 pointer-events-none"></div>
                                <div className="relative z-10 flex-1 flex flex-col">
                                    <div className="flex items-center justify-between mb-3">
                                        <h3 className="font-bold opacity-90 flex items-center gap-2">
                                            <span className="bg-white/20 p-1 rounded"><MessageCircle size={14} /></span>
                                            班级通知
                                        </h3>
                                        <span className="text-[10px] bg-red-500 text-white px-1.5 py-0.5 rounded-full shadow-sm">NEW</span>
                                    </div>
                                    <p className="text-sm opacity-80 leading-relaxed font-medium line-clamp-4 group-hover:line-clamp-none transition-all">
                                        请各位同学注意，本周五下午将进行期中考试模拟，请提前做好准备。

                                        下周一班会课将进行“文明礼仪”主题教育，请班长提前准备好PPT。
                                    </p>
                                </div>
                                <button onClick={(e) => { e.stopPropagation(); setIsNotificationOpen(true); }} className="mt-auto text-xs bg-white/20 hover:bg-white/30 text-center py-2 rounded-lg transition-colors backdrop-blur-sm self-stretch font-medium">
                                    查看全部通知
                                </button>
                            </div>
                        </div>

                        {/* Middle Column (45%) - Schedule & Status */}
                        <div className="col-span-5 flex flex-col gap-4 h-full">
                            {/* Featured Schedule Card */}
                            <div
                                onClick={() => setIsCourseScheduleOpen(true)}
                                className="flex-[2] bg-white rounded-2xl p-6 border border-gray-100 shadow-[0_2px_10px_rgba(0,0,0,0.02)] hover:shadow-[0_8px_30px_rgba(0,0,0,0.04)] transition-all cursor-pointer group relative overflow-hidden"
                            >
                                <div className="absolute top-0 right-0 p-4 opacity-50 group-hover:opacity-100 transition-opacity">
                                    <button className="text-xs text-blue-600 bg-blue-50 px-3 py-1.5 rounded-full font-medium">查看完整课表</button>
                                </div>
                                <div className="h-full flex flex-col justify-center">
                                    <div className="flex items-center gap-2 mb-6">
                                        <span className="w-1.5 h-6 bg-blue-500 rounded-full"></span>
                                        <h3 className="font-bold text-gray-800 text-lg">当前课程</h3>
                                    </div>

                                    <div className="flex items-center gap-6">
                                        <div className="w-24 h-24 rounded-2xl bg-gradient-to-br from-blue-500 to-indigo-600 text-white flex flex-col items-center justify-center shadow-xl shadow-blue-200/50">
                                            <span className="text-xs font-medium opacity-80 mb-1">14:00</span>
                                            <span className="text-2xl font-bold">数学</span>
                                            <span className="text-xs font-medium opacity-60 mt-1">进行中</span>
                                        </div>
                                        <div className="flex-1">
                                            <h4 className="text-2xl font-bold text-gray-800 mb-2">立体几何</h4>
                                            <p className="text-gray-500 font-medium mb-1">空间向量的应用与计算</p>
                                            <div className="flex items-center gap-3 text-sm text-gray-400 mt-3">
                                                <span className="flex items-center gap-1 bg-gray-100 px-2 py-1 rounded-md"><Grid size={12} /> 第 3 节</span>
                                                <span className="flex items-center gap-1 bg-gray-100 px-2 py-1 rounded-md"><Users size={12} /> 教学楼 A-302</span>
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </div>

                            {/* Homework Mini Status */}
                            <div className="flex-1 bg-white rounded-2xl p-5 border border-gray-100 shadow-sm flex items-center justify-between hover:border-green-200 transition-all cursor-pointer" onClick={() => setIsHomeworkOpen(true)}>
                                <div className="flex items-center gap-4">
                                    <div className="w-12 h-12 bg-green-50 text-green-600 rounded-xl flex items-center justify-center">
                                        <BookOpen size={24} />
                                    </div>
                                    <div>
                                        <h4 className="font-bold text-gray-800">作业批改</h4>
                                        <p className="text-xs text-gray-500 mt-0.5">今日待批改 3 份</p>
                                    </div>
                                </div>
                                <div className="flex items-end gap-1">
                                    <span className="text-2xl font-bold text-gray-800">12</span>
                                    <span className="text-xs text-gray-400 mb-1.5">/ 45 提交</span>
                                </div>
                            </div>
                        </div>

                        {/* Right Column (25%) - Tools Grid */}
                        <div className="col-span-4 bg-white/50 rounded-2xl border border-gray-100 p-4 h-full flex flex-col">
                            <h3 className="text-sm font-bold text-gray-600 mb-4 flex items-center gap-2">
                                <Layers size={16} /> 快捷工具
                            </h3>
                            <div className="grid grid-cols-2 gap-3 flex-1 content-start">
                                <button onClick={() => setIsHomeworkOpen(true)} className="h-24 bg-white rounded-xl border border-gray-200/50 shadow-sm hover:shadow-md hover:border-purple-200 transition-all flex flex-col items-center justify-center gap-2 group">
                                    <div className="p-2.5 rounded-full bg-purple-50 text-purple-500 group-hover:scale-110 group-hover:bg-purple-100 transition-all">
                                        <BookOpen size={20} />
                                    </div>
                                    <span className="text-xs font-medium text-gray-600">作业管理</span>
                                </button>

                                <button onClick={handleOpenIntercom} className="h-24 bg-white rounded-xl border border-gray-200/50 shadow-sm hover:shadow-md hover:border-red-200 transition-all flex flex-col items-center justify-center gap-2 group">
                                    <div className="p-2.5 rounded-full bg-red-50 text-red-500 group-hover:scale-110 group-hover:bg-red-100 transition-all">
                                        <Mic size={20} />
                                    </div>
                                    <span className="text-xs font-medium text-gray-600">语音对讲</span>
                                </button>

                                <button onClick={() => setIsCustomListOpen(true)} className="h-24 bg-white rounded-xl border border-gray-200/50 shadow-sm hover:shadow-md hover:border-orange-200 transition-all flex flex-col items-center justify-center gap-2 group">
                                    <div className="p-2.5 rounded-full bg-orange-50 text-orange-500 group-hover:scale-110 group-hover:bg-orange-100 transition-all">
                                        <FileSpreadsheet size={20} />
                                    </div>
                                    <span className="text-xs font-medium text-gray-600">更多导入</span>
                                </button>

                                <button onClick={() => setIsRandomCallOpen(true)} className="h-24 bg-white rounded-xl border border-gray-200/50 shadow-sm hover:shadow-md hover:border-cyan-200 transition-all flex flex-col items-center justify-center gap-2 group">
                                    <div className="p-2.5 rounded-full bg-cyan-50 text-cyan-500 group-hover:scale-110 group-hover:bg-cyan-100 transition-all">
                                        <Shuffle size={20} />
                                    </div>
                                    <span className="text-xs font-medium text-gray-600">随机点名</span>
                                </button>

                                <button onClick={() => setIsCountdownOpen(true)} className="h-24 bg-white rounded-xl border border-gray-200/50 shadow-sm hover:shadow-md hover:border-rose-200 transition-all flex flex-col items-center justify-center gap-2 group">
                                    <div className="p-2.5 rounded-full bg-rose-50 text-rose-500 group-hover:scale-110 group-hover:bg-rose-100 transition-all">
                                        <Clock size={20} />
                                    </div>
                                    <span className="text-xs font-medium text-gray-600">倒计时</span>
                                </button>

                                <button onClick={() => setIsGroupScoreOpen(true)} className="h-24 bg-white rounded-xl border border-gray-200/50 shadow-sm hover:shadow-md hover:border-violet-200 transition-all flex flex-col items-center justify-center gap-2 group">
                                    <div className="p-2.5 rounded-full bg-violet-50 text-violet-500 group-hover:scale-110 group-hover:bg-violet-100 transition-all">
                                        <Award size={20} />
                                    </div>
                                    <span className="text-xs font-medium text-gray-600">小组评价</span>
                                </button>

                                <button onClick={handleRemoteShutdown} className="h-24 bg-white rounded-xl border border-gray-200/50 shadow-sm hover:shadow-md hover:border-red-200 transition-all flex flex-col items-center justify-center gap-2 group relative">
                                    <div className="absolute top-2 right-2">
                                        <div className={`w-2 h-2 rounded-full ${isWSConnected ? 'bg-green-500' : 'bg-red-400'}`}></div>
                                    </div>
                                    <div className="p-2.5 rounded-full bg-red-50 text-red-500 group-hover:scale-110 group-hover:bg-red-100 transition-all">
                                        <Power size={20} />
                                    </div>
                                    <span className="text-xs font-medium text-gray-600">远程开机</span>
                                </button>
                            </div>
                        </div>
                    </div>
                ) : (
                    <div className="h-full flex flex-col bg-white rounded-2xl border border-gray-100 shadow-sm overflow-hidden">
                        <div className="h-12 border-b border-gray-100 flex items-center justify-between px-4 bg-gray-50/50">
                            <h3 className="font-bold text-gray-700 flex items-center gap-2">
                                <Grid size={16} className="text-blue-500" /> 座位表
                            </h3>
                            <div className="flex items-center gap-2">
                                <span className="text-xs text-gray-500">热力图模式:</span>
                                <select
                                    className="bg-white border border-gray-200 text-xs rounded-md p-1 outline-none focus:border-blue-500 transition-colors"
                                    value={heatmapMode}
                                    onChange={(e) => setHeatmapMode(e.target.value as any)}
                                >
                                    <option value="none">无</option>
                                    <option value="score">成绩</option>
                                    <option value="attendance">考勤</option>
                                </select>
                            </div>
                        </div>
                        <div className="flex-1 overflow-hidden p-2">
                            <SeatMap classId={undefined} />
                        </div>
                    </div>
                )}
            </div>

            {/* Bottom Dock - Glassmorphism */}
            <div className="h-20 flex items-center justify-center px-10 relative">
                <div className="absolute inset-0 bg-white/80 backdrop-blur-lg border-t border-white/50 shadow-[0_-10px_40px_rgba(0,0,0,0.03)] z-0"></div>
                <div className="z-10 bg-white border border-gray-100 p-1.5 px-3 rounded-2xl flex items-center gap-6 shadow-lg shadow-gray-200/50">
                    <button
                        onClick={handleOpenChat}
                        className="flex flex-col items-center gap-1 group relative px-4"
                    >
                        <div className="relative">
                            <div className="w-10 h-10 bg-gradient-to-tr from-blue-500 to-blue-600 rounded-xl flex items-center justify-center text-white shadow-lg shadow-blue-200 group-hover:shadow-blue-300 group-hover:-translate-y-1 transition-all duration-300">
                                <MessageCircle size={20} />
                            </div>
                            <span className="absolute -top-1 -right-1 w-3 h-3 bg-red-500 border-2 border-white rounded-full"></span>
                        </div>
                        <span className="text-[10px] font-bold text-gray-500 group-hover:text-blue-600 transition-colors mt-0.5">进入群聊</span>
                    </button>

                    <div className="w-[1px] h-8 bg-gray-200"></div>

                    {[
                        { label: '备课', icon: <BookOpen size={18} /> },
                        { label: '直播', icon: <Calendar size={18} /> },
                    ].map((btn, i) => (
                        <button key={i} className="flex flex-col items-center gap-1 group px-2 text-gray-400 hover:text-gray-600 transition-colors">
                            <div className="w-9 h-9 bg-gray-50 rounded-lg flex items-center justify-center group-hover:bg-gray-100 transition-colors">
                                {btn.icon}
                            </div>
                            <span className="text-[10px] font-medium">{btn.label}</span>
                        </button>
                    ))}
                </div>
            </div>

            {/* Modals */}
            <RandomCallModal
                isOpen={isRandomCallOpen}
                onClose={() => setIsRandomCallOpen(false)}
            />
            <HomeworkModal
                isOpen={isHomeworkOpen}
                onClose={() => setIsHomeworkOpen(false)}
                onPublish={handlePublishHomework}
            />
            <CountdownModal
                isOpen={isCountdownOpen}
                onClose={() => setIsCountdownOpen(false)}
            />
            <DutyRosterModal
                isOpen={isDutyRosterOpen}
                onClose={() => setIsDutyRosterOpen(false)}
            />
            <GroupScoreModal
                isOpen={isGroupScoreOpen}
                onClose={() => setIsGroupScoreOpen(false)}
            />
            <CourseScheduleModal
                isOpen={isCourseScheduleOpen}
                onClose={() => setIsCourseScheduleOpen(false)}
            />
            <ClassInfoModal
                isOpen={isClassInfoOpen}
                onClose={() => setIsClassInfoOpen(false)}
                groupId={groupclassId}
                isClassGroup={true}
            />
            <CustomListModal
                isOpen={isCustomListOpen}
                onClose={() => setIsCustomListOpen(false)}
            />
            <NotificationModal
                isOpen={isNotificationOpen}
                onClose={() => setIsNotificationOpen(false)}
                groupId={groupclassId}
            />
        </div>
    );
};

export default ClassScheduleWindow;
