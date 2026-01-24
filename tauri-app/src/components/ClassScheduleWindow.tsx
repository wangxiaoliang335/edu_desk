import React, { useEffect, useState, useCallback, useRef } from 'react';
import { useParams } from 'react-router-dom';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { invoke } from '@tauri-apps/api/core';
import { Minus, X, Square, Copy, MessageCircle, Calendar, Users, BookOpen, Shuffle, Clock, Grid, LayoutDashboard, Layers, Award, Power, Mic, FileSpreadsheet, BarChart2, ArrowUpDown, Bell, Camera, Video } from 'lucide-react';
import { useWebSocket } from '../context/WebSocketContext';
import { getTIMGroups, isSDKReady, loginTIM } from '../utils/tim';
import RandomCallModal from './modals/RandomCallModal';
import HomeworkModal, { PublishedHomework } from './modals/HomeworkModal';
import CountdownModal from './modals/CountdownModal';
import CourseScheduleModal from './modals/CourseScheduleModal';
import DutyRosterModal from './modals/DutyRosterModal';
import GroupScoreModal from './modals/GroupScoreModal';
import ClassInfoModal from './modals/ClassInfoModal';
import mpegts from 'mpegts.js';
import CustomListModal from './modals/CustomListModal';
import NotificationModal from './modals/NotificationModal';
import StudentImportModal from './modals/StudentImportModal';
import ScoreAnalysisModal from './modals/ScoreAnalysisModal';
import ArrangeSeatModal from './modals/ArrangeSeatModal';
import PrepareClassModal from './modals/PrepareClassModal';
import PostEvaluationModal from './modals/PostEvaluationModal';
import NotificationCenterModal, { NotificationItem } from './modals/NotificationCenterModal';
import DailySchedule from './DailySchedule';
import SeatMap from './SeatMap';



const ClassScheduleWindow = () => {
    const { groupclassId } = useParams();
    const [isMaximized, setIsMaximized] = useState(false);

    // WebSocket
    const { sendMessage: sendSignal, isConnected: isWSConnected } = useWebSocket();

    // View State
    const [currentView, setCurrentView] = useState<'overview' | 'seatmap'>('overview');
    const [heatmapMode, setHeatmapMode] = useState<'none' | 'score' | 'attendance'>('none');

    // Data State
    const [className, setClassName] = useState("");
    const [studentCount, setStudentCount] = useState(0);

    // Monitor State
    const [monitorStatus, setMonitorStatus] = useState<'idle' | 'requesting' | 'monitoring'>('idle');
    const [pullUrl, setPullUrl] = useState("");

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
    const [isNotificationCenterOpen, setIsNotificationCenterOpen] = useState(false); // New Notification Center
    const [notifications, setNotifications] = useState<NotificationItem[]>([]); // Data for Notification Center
    const [isStudentImportOpen, setIsStudentImportOpen] = useState(false);
    const [isScoreAnalysisOpen, setIsScoreAnalysisOpen] = useState(false);
    const [isArrangeSeatOpen, setIsArrangeSeatOpen] = useState(false);
    const [seatColorMap, setSeatColorMap] = useState<Record<string, string>>({});
    const [seatAnalysisMode, setSeatAnalysisMode] = useState<'none' | 'segment' | 'gradient'>('none');
    const [seatMapKey, setSeatMapKey] = useState(0);

    // PrepareClass / PostEvaluation Modal State
    const [isPrepareClassOpen, setIsPrepareClassOpen] = useState(false);
    const [isPostEvaluationOpen, setIsPostEvaluationOpen] = useState(false);
    const [selectedSubject, setSelectedSubject] = useState("");
    const [selectedTime, setSelectedTime] = useState("");

    const [backgroundImage, setBackgroundImage] = useState<string | null>(null);
    const [teachSubjects, setTeachSubjects] = useState<string[]>([]);
    const [latestHomework, setLatestHomework] = useState<PublishedHomework | null>(null);

    const getHomeworkStorageKey = (classId: string) => `homework_history_${classId}`;
    const loadLatestHomework = (classId: string) => {
        try {
            const data = localStorage.getItem(getHomeworkStorageKey(classId));
            if (!data) return null;
            const list = JSON.parse(data) as PublishedHomework[];
            return list.length > 0 ? list[0] : null;
        } catch {
            return null;
        }
    };

    const loadTeachSubjects = (groupId?: string) => {
        if (!groupId) return [];
        try {
            const raw = localStorage.getItem(`teach_subjects_${groupId}`);
            const parsed = raw ? JSON.parse(raw) : [];
            return Array.isArray(parsed) ? parsed : [];
        } catch {
            return [];
        }
    };

    const openHomeworkModal = () => {
        if (!teachSubjects || teachSubjects.length === 0) {
            alert('请先在班级信息中设置任教科目');
            setIsClassInfoOpen(true);
            return;
        }
        setIsHomeworkOpen(true);
    };

    // Fetch Wallpaper Logic
    const fetchWallpaper = async () => {
        if (!groupclassId) return;
        try {
            // 1. Check Weekly Config First
            const weeklyResStr = await invoke<string>('fetch_weekly_config', { groupId: groupclassId });
            const weeklyRes = JSON.parse(weeklyResStr);

            if (weeklyRes.code === 200 && weeklyRes.data && weeklyRes.data.is_enabled && weeklyRes.data.weekly_wallpapers) {
                // Determine today's key (Backend: 0=Mon ... 6=Sun)
                const jsDay = new Date().getDay(); // 0=Sun, 1=Mon...
                const backendKey = jsDay === 0 ? "6" : String(jsDay - 1);

                const url = weeklyRes.data.weekly_wallpapers[backendKey];
                if (url) {
                    console.log("[ClassSchedule] Using Weekly Wallpaper for day:", backendKey, url);
                    setBackgroundImage(url);
                    return; // Priority to weekly
                }
            }

            // 2. Fallback to Current Class Wallpaper
            const wallpapersResStr = await invoke<string>('fetch_class_wallpapers', { groupId: groupclassId });
            const wallpapersRes = JSON.parse(wallpapersResStr);

            if (wallpapersRes.code === 200 && wallpapersRes.data && wallpapersRes.data.wallpapers) {
                const current = wallpapersRes.data.wallpapers.find((wp: any) => wp.is_current === 1);
                if (current) {
                    console.log("[ClassSchedule] Using Current Class Wallpaper:", current.image_url);
                    setBackgroundImage(current.image_url);
                } else {
                    setBackgroundImage(null);
                    console.log("[ClassSchedule] No current wallpaper set");
                }
            }
        } catch (e) {
            console.error("[ClassSchedule] Failed to fetch wallpaper:", e);
        }
    };

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

        // Fetch Class Info
        fetchClassInfo();
        // Fetch Wallpaper
        fetchWallpaper();
        if (groupclassId) {
            setLatestHomework(loadLatestHomework(groupclassId));
            setTeachSubjects(loadTeachSubjects(groupclassId));
        }

        return () => {
            unlisten.then(f => f());
        }
    }, [groupclassId]);

    // Listen for WebSocket Messages (Notification Center & Monitor)
    useEffect(() => {
        const handleWSMessage = (event: CustomEvent) => {
            try {
                const msg = JSON.parse(event.detail);
                // console.log("[ClassSchedule] WS Message Received:", msg);

                // Monitor Protocol
                if (msg.type === 'monitor') {
                    console.log("[MonitorFlow] Received monitor signal:", msg.action, msg);
                    if (groupclassId && msg.group_id && String(msg.group_id) !== String(groupclassId)) {
                        console.warn("[MonitorFlow] Signal ignored: Group ID mismatch. This window:", groupclassId, "Message group:", msg.group_id);
                        return;
                    }

                    if (msg.action === 'ready') {
                        console.log("[MonitorFlow] Signal READY. Pull URL:", msg.pull_url);
                        setMonitorStatus('monitoring');

                        // Parse SRT to HTTP-FLV
                        // Input: srt://47.100.126.194:10080?streamid=#!::r=live/class_123,m=publish
                        // Output: http://47.100.126.194:8080/live/class_123.flv
                        let finalUrl = msg.pull_url;
                        if (msg.pull_url && msg.pull_url.startsWith('srt://')) {
                            console.log("[MonitorFlow] Converting SRT to HTTP-FLV...");
                            try {
                                const hostMatch = msg.pull_url.match(/srt:\/\/([^:?\/]+)/);
                                const streamIdMatch = msg.pull_url.match(/streamid=([^&]+)/);

                                if (hostMatch && hostMatch[1] && streamIdMatch && streamIdMatch[1]) {
                                    const ip = hostMatch[1];
                                    const streamid = decodeURIComponent(streamIdMatch[1]);
                                    const rMatch = streamid.match(/r=([^,]+)/);

                                    if (rMatch && rMatch[1]) {
                                        const streamKey = rMatch[1];
                                        finalUrl = `http://${ip}:8080/${streamKey}.flv`;
                                        console.log(`[MonitorFlow] Converted to: ${finalUrl}`);
                                    }
                                }
                            } catch (e) {
                                console.error('[MonitorFlow] URL Conversion Failed', e);
                            }
                        } else if (msg.pull_url && msg.pull_url.startsWith('webrtc://')) {
                            console.log("[MonitorFlow] Using direct WebRTC URL:", msg.pull_url);
                            finalUrl = msg.pull_url;
                        }

                        setPullUrl(finalUrl);
                        // Switch to monitor view (seatmap area)
                        console.log("[MonitorFlow] Switching to monitor view.");
                        setCurrentView('seatmap');
                    } else if (msg.action === 'stopped') {
                        console.log("[MonitorFlow] Signal STOPPED.");
                        setMonitorStatus('idle');
                        setPullUrl("");
                    } else if (msg.action === 'pending') {
                        console.log("[MonitorFlow] Signal PENDING.");
                        setMonitorStatus('requesting');
                    }
                }

                if (msg.type === "unread_notifications") {
                    console.log("[ClassSchedule] Received Notifications Payload:", msg.data);
                    // ... (existing notification logic) ...
                    const newItems = (msg.data || []) as NotificationItem[];
                    setNotifications(prev => {
                        const existingMap = new Map(prev.map(item => [item.id, item]));
                        newItems.forEach(item => { existingMap.set(item.id, item); });
                        return Array.from(existingMap.values());
                    });
                }
            } catch (e) {
                console.error("[ClassSchedule] Error parsing WS message:", e);
            }
        };

        window.addEventListener('ws-message', handleWSMessage as EventListener);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage as EventListener);
        };
    }, [groupclassId]);

    const handleToggleMonitor = useCallback(() => {
        console.log("[MonitorFlow] handleToggleMonitor clicked. Status:", monitorStatus);
        if (!groupclassId) {
            console.error("[MonitorFlow] Missing groupclassId");
            return;
        }

        // Get user ID
        let currentUserId = localStorage.getItem('teacher_unique_id') || "0";
        const userInfoStr = localStorage.getItem('user_info');

        if (currentUserId === "0" && userInfoStr) {
            try {
                const u = JSON.parse(userInfoStr);
                // Fallback logic
                currentUserId = u.teacher_unique_id || u.unique_id || u.id || u.user_id || "0";
            } catch (e) { }
        }
        console.log("[MonitorFlow] Current User ID:", currentUserId);

        if (monitorStatus === 'idle') {
            console.log("[MonitorFlow] Mode: IDLE -> Sending REQUEST to group:", groupclassId);
            // Request
            const msg = {
                type: "monitor",
                action: "request",
                group_id: groupclassId,
                class_id: groupclassId,
                ts: Date.now() / 1000,
                sender_id: currentUserId,
            };
            const payload = `to:${groupclassId}:${JSON.stringify(msg)}`;
            console.log("[MonitorFlow] WebSocket Payload:", payload);
            sendSignal(payload);
            setMonitorStatus('requesting');

            // Timeout reset (Extended to 15s for hardware response)
            setTimeout(() => {
                setMonitorStatus(prev => {
                    if (prev === 'requesting') {
                        console.warn("[MonitorFlow] Request timed out after 15s. Resetting to IDLE.");
                        return 'idle';
                    }
                    return prev;
                });
            }, 15000);
        } else {
            console.log("[MonitorFlow] Mode:", monitorStatus, "-> Sending STOP to group:", groupclassId);
            // Stop
            const msg = {
                type: "monitor",
                action: "stop_watch",
                group_id: groupclassId,
                sender_id: currentUserId,
            };
            sendSignal(`to:${groupclassId}:${JSON.stringify(msg)}`);
            setMonitorStatus('idle');
        }
    }, [groupclassId, monitorStatus, sendSignal]);

    const fetchClassInfo = async () => {
        if (!groupclassId) return;

        // Initial fallback
        setClassName(prev => prev || groupclassId || "未知班级");

        try {
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            let idCard = "";
            let teacherId = "";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                    idCard = u.id_number || u.data?.id_number || u.strIdNumber || localStorage.getItem('id_number') || "";
                    teacherId = u.teacher_unique_id || u.data?.teacher_unique_id || localStorage.getItem('teacher_unique_id') || "";
                } catch (e) { }
            }

            console.log(`[ClassSchedule] Fetching Info for ID: ${groupclassId}, Teacher: ${teacherId}`);

            // Helper to normalize subjects (same as in ClassInfoModal)
            const normalizeSubjects = (value: any): string[] => {
                if (Array.isArray(value)) return value.filter((v) => typeof v === 'string' && v.trim());
                if (typeof value === 'string') {
                    try {
                        const parsed = JSON.parse(value);
                        if (Array.isArray(parsed)) return parsed.filter((v) => typeof v === 'string' && v.trim());
                    } catch { }
                    return value.split(',').map((v) => v.trim()).filter(Boolean);
                }
                return [];
            };

            // Parallel requests to try to find the name
            const p1 = invoke<string>('get_group_members', { groupId: groupclassId, token })
                .then(resStr => {
                    const res = JSON.parse(resStr);
                    if (res.code === 200 || (res.data && res.data.code === 200)) {
                        const data = res.data || {};
                        const list = data.members || [];
                        setStudentCount(list.length || data.group_info?.MemberNum || 0);

                        // Sync teach subjects for current teacher
                        if (teacherId && list.length > 0) {
                            const self = list.find((m: any) => String(m.user_id || m.id || m.teacher_unique_id) === String(teacherId));
                            if (self && self.teach_subjects) {
                                const normalized = normalizeSubjects(self.teach_subjects);
                                if (normalized.length > 0) {
                                    console.log("[ClassSchedule] Synced teach subjects from server:", normalized);
                                    setTeachSubjects(normalized);
                                    localStorage.setItem(`teach_subjects_${groupclassId}`, JSON.stringify(normalized));
                                }
                            }
                        }

                        const name = data.group_info?.Name || data.group_info?.GroupName;
                        console.log("[ClassSchedule] p1 (Members) found:", name);
                        return name;
                    }
                    return null;
                })
                .catch(e => { console.error("[ClassSchedule] p1 error:", e); return null; });

            const p2 = idCard ? invoke<string>('get_user_friends', { idCard, token })
                .then(resStr => {
                    const res = JSON.parse(resStr);
                    if (res.data) {
                        const allGroups = [...(res.data.owner_groups || []), ...(res.data.member_groups || [])];
                        // Fuzzy match because string ID from URL vs maybe formatted ID from server
                        const target = allGroups.find((g: any) => g.group_id == groupclassId);
                        console.log("[ClassSchedule] p2 (Friends) found:", target?.group_name);
                        return target?.group_name;
                    }
                    return null;
                })
                .catch(e => { console.error("[ClassSchedule] p2 error:", e); return null; }) : Promise.resolve(null);

            const p3 = teacherId ? invoke<string>('get_teacher_classes', { teacherUniqueId: teacherId, token })
                .then(resStr => {
                    const res = JSON.parse(resStr);
                    const classes = res.data?.classes || ((Array.isArray(res.data)) ? res.data : []);
                    const target = classes.find((c: any) => c.class_code == groupclassId);
                    console.log("[ClassSchedule] p3 (Classes) found:", target?.class_name);
                    return target?.class_name;
                })
                .catch(e => { console.error("[ClassSchedule] p3 error:", e); return null; }) : Promise.resolve(null);

            // 4. TIM SDK Groups (Most accurate for "Group Chats")
            const p4 = (async () => {
                if (!teacherId) return null;
                try {
                    // We must login because this is a new window/context
                    let ready = isSDKReady;
                    if (!ready) {
                        console.log("[ClassSchedule] TIM Login required. Fetching Sig...");
                        const userSig = await invoke<string>('get_user_sig', { userId: teacherId });
                        if (userSig) {
                            ready = await loginTIM(teacherId, userSig) as boolean;
                            console.log("[ClassSchedule] TIM Login Result:", ready);
                        }
                    }

                    if (ready) {
                        const groups = await getTIMGroups();
                        console.log(`[ClassSchedule] TIM Groups fetched: ${groups.length}`);
                        const target = groups.find((g: any) => g.groupID == groupclassId);
                        if (target) {
                            console.log("[ClassSchedule] TIM Match Found:", target.name, target.groupID);
                        } else {
                            console.log("[ClassSchedule] TIM No Match for:", groupclassId);
                        }
                        return target?.name;
                    } else {
                        console.warn("[ClassSchedule] TIM SDK not ready after login attempt");
                    }
                } catch (e) {
                    console.error("[ClassSchedule] TIM Fetch failed in Window:", e);
                }
                return null;
            })();

            // Wait for results and pick the best name
            const [nameFromGroup, nameFromFriends, nameFromClasses, nameFromTIM] = await Promise.all([p1, p2, p3, p4]);

            console.log("[ClassSchedule] Final Candidates:", { nameFromGroup, nameFromFriends, nameFromClasses, nameFromTIM });

            // Priority: TIM Name > Server Group Name > Teacher Classes Name > Group Info Name
            const finalName = nameFromTIM || nameFromFriends || nameFromClasses || nameFromGroup;
            if (finalName) {
                setClassName(finalName);
            } else {
                // If we still found nothing, keep showing ID or default, but remove "Loading..." logic if any
                setClassName(prev => prev || groupclassId || "未命名班级");
            }

        } catch (e) {
            console.error("Failed to fetch class info:", e);
            setClassName(prev => prev || groupclassId || "网络错误");
        }
    };


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

    // Note: handlePublishHomework removed - HomeworkModal now handles WebSocket messaging internally

    const handleRemoteShutdown = async () => {
        if (confirm("确定要关闭当前班级设备吗？此操作将关闭计算机。")) {
            try {
                // Construct logic from ScheduleDialog.h:2658
                const userInfoStr = localStorage.getItem('user_info');
                let teacherId = localStorage.getItem('teacher_unique_id') || "";
                let senderName = "Teacher";

                if (userInfoStr) {
                    try {
                        const u = JSON.parse(userInfoStr);
                        teacherId = u.teacher_unique_id || u.data?.teacher_unique_id || teacherId || "";
                        senderName = u.name || u.data?.name || u.strName || "Teacher";
                    } catch (e) { }
                }

                if (!groupclassId || !teacherId) {
                    alert("信息不完整，无法发送关机指令");
                    console.error("[Shutdown] Missing info:", { groupclassId, teacherId });
                    return;
                }

                const payload = {
                    type: "remote_shutdown",
                    action: "shutdown",
                    class_id: groupclassId ? (groupclassId.endsWith('01') ? groupclassId.slice(0, -2) : groupclassId) : "",
                    group_id: groupclassId,
                    sender_id: teacherId,
                    sender_name: senderName,
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
        <div
            className="h-screen w-screen bg-paper flex flex-col overflow-hidden border border-sage-200 select-none text-ink-600 font-sans relative transition-all duration-500 bg-cover bg-center rounded-[2rem] shadow-2xl"
            style={backgroundImage ? { backgroundImage: `url(${backgroundImage})` } : {}}
        >
            {/* Overlay to ensure text readability if background is set */}
            {backgroundImage && (
                <div className="absolute inset-0 bg-paper/80 backdrop-blur-sm z-0 pointer-events-none"></div>
            )}

            {/* Title Bar - Floating Style */}
            <div data-tauri-drag-region className="h-14 shrink-0 flex items-center justify-between px-6 z-50 relative mt-2 mx-2">
                <div className="flex items-center gap-4 bg-white/60 backdrop-blur-md px-4 py-2 rounded-2xl border border-white/50 shadow-sm">
                    <div className="font-bold text-ink-800 flex items-center gap-2.5">
                        <div className="bg-sage-100 text-sage-600 p-1.5 rounded-lg"><Users size={14} /></div>
                        <span className="text-sm tracking-wide font-bold">{className || groupclassId}</span>
                    </div>
                    {/* View Switcher */}
                    <div className="flex bg-sage-50/80 p-1 rounded-xl">
                        <button
                            onClick={() => setCurrentView('overview')}
                            className={`px-3 py-1 text-xs rounded-lg flex items-center gap-1.5 transition-all font-bold ${currentView === 'overview' ? 'bg-white text-sage-600 shadow-sm' : 'text-ink-400 hover:text-ink-600'}`}
                        >
                            <LayoutDashboard size={14} /> 概览
                        </button>
                        <button
                            onClick={() => setCurrentView('seatmap')}
                            className={`px-3 py-1 text-xs rounded-lg flex items-center gap-1.5 transition-all font-bold ${currentView === 'seatmap' ? 'bg-white text-sage-600 shadow-sm' : 'text-ink-400 hover:text-ink-600'}`}
                        >
                            <Grid size={14} /> 座位表
                        </button>
                    </div>
                </div>

                <div className="flex items-center gap-3">
                    {/* Notification Bell */}
                    <button
                        onClick={() => setIsNotificationCenterOpen(true)}
                        className="w-9 h-9 flex items-center justify-center bg-white/60 hover:bg-white rounded-full text-ink-400 hover:text-sage-500 transition-all shadow-sm border border-transparent hover:border-sage-100 relative group"
                    >
                        <Bell size={18} />
                        {notifications.length > 0 && notifications.some(n => n.is_read === 0) && (
                            <span className="absolute top-0 right-0 w-2.5 h-2.5 bg-clay-500 rounded-full border-2 border-white"></span>
                        )}
                        <span className="absolute top-10 right-0 bg-ink-800 text-white text-[10px] px-2 py-1 rounded-md opacity-0 group-hover:opacity-100 transition-opacity whitespace-nowrap pointer-events-none">通知中心</span>
                    </button>

                    <div className="h-4 w-px bg-sage-200/50 mx-1"></div>

                    <div className="flex items-center gap-2 bg-white/60 backdrop-blur-md px-2 py-1.5 rounded-full border border-white/50 shadow-sm">
                        <button onClick={() => setIsClassInfoOpen(true)} className="w-8 h-8 flex items-center justify-center rounded-full text-ink-400 hover:bg-sage-50 hover:text-sage-600 transition-colors">
                            <Layers size={16} />
                        </button>
                        <button onClick={handleMinimize} className="w-8 h-8 flex items-center justify-center rounded-full text-ink-400 hover:bg-sage-50 hover:text-sage-600 transition-colors">
                            <Minus size={18} />
                        </button>
                        <button onClick={handleMaximize} className="w-8 h-8 flex items-center justify-center rounded-full text-ink-400 hover:bg-sage-50 hover:text-sage-600 transition-colors">
                            {isMaximized ? <Copy size={16} /> : <Square size={16} />}
                        </button>
                        <button onClick={handleClose} className="w-8 h-8 flex items-center justify-center rounded-full text-ink-400 hover:bg-clay-500 hover:text-white transition-all">
                            <X size={18} />
                        </button>
                    </div>
                </div>
            </div>

            {/* Main Content Area */}
            <div className="flex-1 overflow-hidden px-6 pb-6 pt-2 relative flex flex-col gap-5 z-10">
                {currentView === 'overview' ? (
                    <div className="flex-1 flex flex-col gap-5 h-full">
                        {/* Top Row - Daily Schedule Timeline (Full Width) */}
                        <div className="bg-white/80 backdrop-blur-xl rounded-[1.5rem] p-5 border border-white shadow-[0_8px_30px_rgb(0,0,0,0.04)] flex-shrink-0">
                            <div className="flex items-center justify-between mb-4">
                                <div className="flex items-center gap-3">
                                    <div className="w-1.5 h-6 bg-sage-400 rounded-full"></div>
                                    <h3 className="text-lg font-bold text-ink-800">今日课程</h3>
                                </div>
                                <button
                                    onClick={() => setIsCourseScheduleOpen(true)}
                                    className="text-xs text-sage-600 bg-sage-50 hover:bg-sage-100 px-4 py-2 rounded-xl font-bold transition-colors"
                                >
                                    查看完整课表
                                </button>
                            </div>
                            <DailySchedule
                                classId={groupclassId}
                                onPrepareClass={(subject, time) => {
                                    setSelectedSubject(subject);
                                    setSelectedTime(time);
                                    setIsPrepareClassOpen(true);
                                }}
                                onPostEvaluation={(subject) => {
                                    setSelectedSubject(subject);
                                    setIsPostEvaluationOpen(true);
                                }}
                            />
                        </div>

                        {/* Bottom Row - 3 Column Grid */}
                        <div className="flex-1 grid grid-cols-12 gap-5 min-h-0">
                            {/* Left Column - Class Info & Notifications */}
                            <div className="col-span-3 flex flex-col gap-5">
                                {/* Hero Card */}
                                <div className="bg-white/90 backdrop-blur rounded-[1.5rem] p-6 border border-white shadow-[0_4px_20px_rgb(0,0,0,0.02)] relative overflow-hidden group hover:shadow-lg transition-all">
                                    <div className="absolute top-0 right-0 w-32 h-32 bg-gradient-to-br from-sage-50 to-clay-50 rounded-bl-full -mr-8 -mt-8 opacity-60 group-hover:scale-110 transition-transform duration-500"></div>
                                    <div className="relative z-10">
                                        <h1 className="text-xl font-bold text-ink-800 mb-1 tracking-tight">{className}</h1>
                                        <p className="text-sm text-ink-400 font-medium">智慧课堂班级空间</p>
                                        <div className="mt-6 flex flex-col gap-2">
                                            <div className="flex items-center gap-2 text-xs font-bold text-ink-500 bg-paper/50 p-2 rounded-lg w-fit">
                                                <Users size={14} className="text-sage-500" />
                                                <span>{studentCount} 位学员</span>
                                            </div>
                                            <div className="flex items-center gap-2 text-xs font-bold text-ink-500 bg-paper/50 p-2 rounded-lg w-fit">
                                                <Calendar size={14} className="text-clay-500" />
                                                <span>2025-2026学年 第一学期</span>
                                            </div>
                                        </div>
                                    </div>
                                </div>

                                {/* Notifications Card */}
                                <div
                                    onClick={() => setIsNotificationOpen(true)}
                                    className="flex-1 bg-gradient-to-br from-sage-500 to-sage-600 rounded-[1.5rem] p-5 text-white shadow-xl shadow-sage-200/50 flex flex-col cursor-pointer transition-all hover:-translate-y-1 hover:shadow-2xl hover:shadow-sage-300/50 group"
                                >
                                    <div className="flex items-center justify-between mb-3">
                                        <h3 className="font-bold text-base opacity-95 flex items-center gap-2">
                                            <div className="bg-white/20 p-1.5 rounded-lg"><MessageCircle size={14} /></div>
                                            班级通知
                                        </h3>
                                    </div>
                                    <p className="text-xs opacity-80 leading-relaxed line-clamp-3 flex-1 font-medium">
                                        向班级学生和家长发送重要通知与公告。
                                    </p>
                                    <button className="mt-4 text-xs bg-white/20 hover:bg-white/30 py-2.5 rounded-xl transition-colors font-bold w-full flex items-center justify-center gap-2 group-hover:bg-white/25">
                                        立即发布 <ArrowUpDown size={12} className="rotate-90" />
                                    </button>
                                </div>
                            </div>

                            {/* Middle Column - Homework & Stats */}
                            <div className="col-span-5 flex flex-col gap-5">
                                {/* Homework Card */}
                                <div
                                    onClick={openHomeworkModal}
                                    className="bg-white/90 backdrop-blur rounded-[1.5rem] p-6 border border-white shadow-[0_4px_20px_rgb(0,0,0,0.02)] flex items-center justify-between hover:border-sage-200 hover:shadow-md transition-all cursor-pointer group"
                                >
                                    <div className="flex items-center gap-5">
                                        <div className="w-14 h-14 bg-clay-50 text-clay-600 rounded-2xl flex items-center justify-center group-hover:scale-105 transition-transform shadow-inner">
                                            <BookOpen size={28} />
                                        </div>
                                        <div>
                                            <h4 className="font-bold text-lg text-ink-800">作业批改</h4>
                                            {latestHomework ? (
                                                <p className="text-xs text-ink-400 mt-1 font-medium bg-paper px-2 py-1 rounded-md inline-block border border-sage-50">
                                                    {latestHomework.subject} · {latestHomework.date}
                                                </p>
                                            ) : (
                                                <p className="text-xs text-ink-400 mt-1">暂无作业数据</p>
                                            )}
                                        </div>
                                    </div>
                                    <div className="flex flex-col items-end gap-1">
                                        <span className="text-3xl font-bold text-sage-200 group-hover:text-sage-400 transition-colors">--</span>
                                        <span className="text-xs font-bold text-ink-300">提交率</span>
                                    </div>
                                </div>

                                {/* Quick Stats */}
                                <div className="flex-1 grid grid-cols-3 gap-4">
                                    <div className="bg-white/80 backdrop-blur rounded-[1.2rem] p-4 border border-white shadow-sm flex flex-col items-center justify-center gap-2">
                                        <div className="w-8 h-8 rounded-full bg-sage-50 flex items-center justify-center text-sage-500 mb-1"><Users size={16} /></div>
                                        <span className="text-2xl font-bold text-ink-800">--%</span>
                                        <span className="text-xs font-bold text-ink-400">出勤率</span>
                                    </div>
                                    <div className="bg-white/80 backdrop-blur rounded-[1.2rem] p-4 border border-white shadow-sm flex flex-col items-center justify-center gap-2">
                                        <div className="w-8 h-8 rounded-full bg-blue-50 flex items-center justify-center text-blue-500 mb-1"><BarChart2 size={16} /></div>
                                        <span className="text-2xl font-bold text-ink-800">--</span>
                                        <span className="text-xs font-bold text-ink-400">平均分</span>
                                    </div>
                                    <div className="bg-white/80 backdrop-blur rounded-[1.2rem] p-4 border border-white shadow-sm flex flex-col items-center justify-center gap-2">
                                        <div className="w-8 h-8 rounded-full bg-amber-50 flex items-center justify-center text-amber-500 mb-1"><Award size={16} /></div>
                                        <span className="text-2xl font-bold text-ink-800">A+</span>
                                        <span className="text-xs font-bold text-ink-400">班级评级</span>
                                    </div>
                                </div>
                            </div>

                            {/* Right Column - Tools Grid */}
                            <div className="col-span-4 bg-white/60 backdrop-blur rounded-[1.5rem] border border-white/60 p-5 flex flex-col shadow-inner overflow-hidden">
                                <h3 className="text-sm font-bold text-ink-500 mb-4 flex items-center gap-2 px-1">
                                    <Layers size={14} /> 快捷工具
                                </h3>
                                <div className="flex-1 overflow-y-auto custom-scrollbar pr-1">
                                    <div className="grid grid-cols-2 gap-3 content-start">
                                        <button onClick={handleRemoteShutdown} className="h-16 bg-red-50 rounded-2xl border border-red-100 shadow-sm hover:bg-red-500 hover:text-white transition-all flex items-center gap-3 px-3.5 group relative overflow-hidden">
                                            <div className={`absolute top-2 right-2 w-2 h-2 rounded-full ${isWSConnected ? 'bg-green-500' : 'bg-red-400'} ring-2 ring-white`}></div>
                                            <div className="p-2.5 rounded-xl bg-white text-red-500 group-hover:bg-white/20 group-hover:text-white transition-all">
                                                <Power size={18} />
                                            </div>
                                            <span className="text-xs font-bold text-red-600 group-hover:text-white transition-colors">远程关机</span>
                                        </button>

                                        {[
                                            { title: "摄像头", icon: <Camera size={18} />, color: monitorStatus === 'monitoring' ? "text-red-500 bg-red-50" : (monitorStatus === 'requesting' ? "text-gray-500 bg-gray-50" : "text-emerald-500 bg-emerald-50"), onClick: handleToggleMonitor },
                                            { title: "作业管理", icon: <BookOpen size={18} />, color: "text-purple-500 bg-purple-50", onClick: openHomeworkModal },
                                            { title: "语音对讲", icon: <Mic size={18} />, color: "text-red-500 bg-red-50", onClick: handleOpenIntercom },
                                            { title: "更多导入", icon: <FileSpreadsheet size={18} />, color: "text-orange-500 bg-orange-50", onClick: () => setIsCustomListOpen(true) },
                                            { title: "随机点名", icon: <Shuffle size={18} />, color: "text-cyan-500 bg-cyan-50", onClick: () => setIsRandomCallOpen(true) },
                                            { title: "倒计时", icon: <Clock size={18} />, color: "text-rose-500 bg-rose-50", onClick: () => setIsCountdownOpen(true) },
                                            { title: "小组评价", icon: <Award size={18} />, color: "text-violet-500 bg-violet-50", onClick: () => setIsGroupScoreOpen(true) },
                                            { title: "座位导入", icon: <Grid size={18} />, color: "text-blue-500 bg-blue-50", onClick: () => setIsStudentImportOpen(true) },
                                            { title: "成绩分析", icon: <BarChart2 size={18} />, color: "text-indigo-500 bg-indigo-50", onClick: () => setIsScoreAnalysisOpen(true) },
                                            { title: "智能排座", icon: <ArrowUpDown size={18} />, color: "text-teal-500 bg-teal-50", onClick: () => setIsArrangeSeatOpen(true) },
                                        ].map((tool, idx) => (
                                            <button
                                                key={idx}
                                                onClick={tool.onClick}
                                                className="h-16 bg-white rounded-2xl border border-transparent shadow-sm hover:border-sage-200 hover:shadow-md transition-all flex items-center gap-3 px-3.5 group"
                                            >
                                                <div className={`p-2.5 rounded-xl ${tool.color} group-hover:scale-110 transition-transform`}>
                                                    {tool.icon}
                                                </div>
                                                <span className="text-xs font-bold text-ink-600">{tool.title}</span>
                                            </button>
                                        ))}
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                ) : (
                    <div className="h-full flex flex-col bg-white/80 backdrop-blur rounded-[2rem] border border-white shadow-sm overflow-hidden">
                        <div className="h-16 border-b border-sage-100 flex items-center justify-between px-6 bg-white/50">
                            <h3 className="font-bold text-ink-700 flex items-center gap-2.5">
                                <div className="bg-sage-100 p-1.5 rounded-lg text-sage-600"><Grid size={18} /></div>
                                <span className="text-lg">座位表概览</span>
                            </h3>
                            <div className="flex items-center gap-3">
                                <span className="text-xs font-bold text-ink-400">显示模式:</span>
                                <div className="relative">
                                    <select
                                        className="appearance-none bg-white border border-sage-200 text-xs font-bold text-ink-600 rounded-xl py-2 pl-3 pr-8 outline-none focus:border-sage-400 hover:bg-sage-50 transition-colors cursor-pointer"
                                        value={heatmapMode}
                                        onChange={(e) => setHeatmapMode(e.target.value as any)}
                                    >
                                        <option value="none">默认视图</option>
                                        <option value="score">成绩热力图</option>
                                        <option value="attendance">考勤热力图</option>
                                    </select>
                                    <div className="absolute right-2 top-1/2 -translate-y-1/2 text-ink-400 pointer-events-none">
                                        <ArrowUpDown size={12} />
                                    </div>
                                </div>
                            </div>
                        </div>
                        <div className="flex-1 overflow-hidden p-4 relative bg-black rounded-xl">
                            {monitorStatus === 'monitoring' ? (
                                pullUrl.startsWith('webrtc://') ? (
                                    <WebRtcPlayer url={pullUrl} className={className} onClose={handleToggleMonitor} />
                                ) : (
                                    <FlvPlayer url={pullUrl} className={className} onClose={handleToggleMonitor} />
                                )
                            ) : (
                                <SeatMap classId={groupclassId} key={seatMapKey} colorMap={seatColorMap} isHeatmapMode={seatAnalysisMode === 'gradient'} />
                            )}
                        </div>
                    </div>
                )}
            </div>

            {/* Bottom Dock - Floating Organic Pill */}
            <div className="h-24 flex items-center justify-center px-10 relative z-50 pointer-events-none">
                <div className="bg-white/90 backdrop-blur-2xl border border-sage-100 p-2 pl-4 pr-4 rounded-full flex items-center gap-4 shadow-[0_20px_40px_-10px_rgba(0,0,0,0.1)] pointer-events-auto hover:scale-105 transition-transform duration-300">
                    <button
                        onClick={handleOpenChat}
                        className="flex flex-col items-center gap-1 group relative px-2"
                    >
                        <div className="relative">
                            <div className="w-12 h-12 bg-gradient-to-tr from-blue-500 to-blue-600 rounded-2xl flex items-center justify-center text-white shadow-lg shadow-blue-200 group-hover:shadow-blue-300 group-hover:-translate-y-1 transition-all duration-300">
                                <MessageCircle size={22} />
                            </div>
                            <span className="absolute -top-1 -right-1 w-3.5 h-3.5 bg-red-500 border-2 border-white rounded-full flex items-center justify-center">
                                <span className="w-1 h-1 bg-white rounded-full"></span>
                            </span>
                        </div>
                        {/* Tooltip */}
                        <span className="absolute -top-10 left-1/2 -translate-x-1/2 bg-ink-800 text-white text-[10px] px-2 py-1 rounded-md opacity-0 group-hover:opacity-100 transition-opacity">群聊</span>
                    </button>

                    <div className="w-px h-8 bg-sage-200"></div>

                    <button
                        onClick={() => setIsNotificationOpen(true)}
                        className="flex flex-col items-center gap-1 group px-1 relative"
                    >
                        <div className="w-10 h-10 bg-clay-50 text-clay-500 rounded-xl flex items-center justify-center group-hover:bg-clay-100 group-hover:-translate-y-1 transition-all">
                            <Bell size={20} />
                        </div>
                    </button>

                    <button
                        onClick={() => setIsHomeworkOpen(true)}
                        className="flex flex-col items-center gap-1 group px-1 relative"
                    >
                        <div className="w-10 h-10 bg-sage-50 text-sage-500 rounded-xl flex items-center justify-center group-hover:bg-sage-100 group-hover:-translate-y-1 transition-all">
                            <BookOpen size={20} />
                        </div>
                    </button>

                    <button
                        onClick={() => setIsCourseScheduleOpen(true)}
                        className="flex flex-col items-center gap-1 group px-1 relative"
                    >
                        <div className="w-10 h-10 bg-purple-50 text-purple-500 rounded-xl flex items-center justify-center group-hover:bg-purple-100 group-hover:-translate-y-1 transition-all">
                            <Calendar size={20} />
                        </div>
                    </button>
                </div>
            </div>

            {/* Modals */}
            <RandomCallModal
                isOpen={isRandomCallOpen}
                onClose={() => setIsRandomCallOpen(false)}
                classId={groupclassId}
            />
            <HomeworkModal
                isOpen={isHomeworkOpen}
                onClose={() => setIsHomeworkOpen(false)}
                classId={groupclassId}
                groupId={groupclassId}
                groupName={className}
                teachSubjects={teachSubjects}
                onPublished={(latest, all) => {
                    setLatestHomework(latest);
                    if (groupclassId) {
                        localStorage.setItem(getHomeworkStorageKey(groupclassId), JSON.stringify(all));
                    }
                }}
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
                classId={groupclassId}
            />
            <ClassInfoModal
                isOpen={isClassInfoOpen}
                onClose={() => {
                    setIsClassInfoOpen(false);
                    fetchWallpaper(); // Refresh wallpaper when settings might have changed
                    if (groupclassId) {
                        setTeachSubjects(loadTeachSubjects(groupclassId));
                    }
                }}
                groupId={groupclassId}
                isClassGroup={true}
            />
            <CustomListModal
                isOpen={isCustomListOpen}
                onClose={() => setIsCustomListOpen(false)}
                classId={groupclassId}
            />
            <StudentImportModal
                isOpen={isStudentImportOpen}
                onClose={() => setIsStudentImportOpen(false)}
                classId={groupclassId || ""}
                onSuccess={() => setSeatMapKey(prev => prev + 1)}
            />
            <ScoreAnalysisModal
                isOpen={isScoreAnalysisOpen}
                onClose={() => setIsScoreAnalysisOpen(false)}
                classId={groupclassId || ""}
                onApply={(map, mode) => {
                    setSeatColorMap(map);
                    setSeatAnalysisMode(mode);
                    // Also switch to seat view if not already
                    if (currentView !== 'seatmap') setCurrentView('seatmap');
                }}
            />
            <NotificationModal
                isOpen={isNotificationOpen}
                onClose={() => setIsNotificationOpen(false)}
                classId={groupclassId}
                groupId={groupclassId}
                groupName={className}
            />
            <NotificationCenterModal
                isOpen={isNotificationCenterOpen}
                onClose={() => setIsNotificationCenterOpen(false)}
                notifications={notifications}
            />
            <ArrangeSeatModal
                isOpen={isArrangeSeatOpen}
                onClose={() => setIsArrangeSeatOpen(false)}
                classId={groupclassId}
                onArrange={async (arrangedStudents) => {
                    // Upload arranged seats to server
                    try {
                        const seatData = arrangedStudents.map((s, idx) => ({
                            student_id: s.id,
                            student_name: s.name,
                            row: Math.floor(idx / 11) + 1,
                            col: (idx % 11) + 1,
                        }));

                        await invoke('save_seat_arrangement', {
                            classId: groupclassId,
                            seatsJson: JSON.stringify(seatData)
                        });

                        // Refresh seat map
                        setSeatMapKey(prev => prev + 1);
                        setCurrentView('seatmap');
                        console.log('[Arrange] Seat arrangement saved successfully');
                    } catch (err) {
                        console.error('[Arrange] Failed to save seat arrangement:', err);
                        alert('保存座位表失败');
                    }
                }}
            />
            <PrepareClassModal
                isOpen={isPrepareClassOpen}
                onClose={() => setIsPrepareClassOpen(false)}
                subject={selectedSubject}
                time={selectedTime}
                classId={groupclassId}
                groupId={groupclassId}
            />
            <PostEvaluationModal
                isOpen={isPostEvaluationOpen}
                onClose={() => setIsPostEvaluationOpen(false)}
                subject={selectedSubject}
                classId={groupclassId}
                groupId={groupclassId}
            />
        </div>
    );
};

const WebRtcPlayer = ({ url, className, onClose }: { url: string, className: string, onClose: () => void }) => {
    const videoRef = useRef<HTMLVideoElement>(null);
    const pcRef = useRef<RTCPeerConnection | null>(null);
    const [error, setError] = useState("");
    const [retryCount, setRetryCount] = useState(0);
    const maxRetries = 10; // Increased for hardware/push delay

    useEffect(() => {
        if (!url || !videoRef.current) return;
        const videoElement = videoRef.current;
        console.log("[MonitorFlow] WebRtcPlayer mounted. URL:", url, "Retry:", retryCount);

        const stop = () => {
            if (pcRef.current) {
                console.log("[MonitorFlow] Closing WebRTC connection.");
                pcRef.current.close();
                pcRef.current = null;
            }
        };

        const startPlaying = async () => {
            try {
                // 1. Parse IP from webrtc://HOST/app/stream
                const match = url.match(/webrtc:\/\/([^/]+)/);
                if (!match) throw new Error("无效的 WebRTC 地址目标 (Invalid URL)");
                const host = match[1];
                // Standard SRS API port is 1985
                const api = `http://${host}:1985/rtc/v1/play/`;

                console.log("[MonitorFlow] WebRTC SDP Offer Exchange ->", api);

                // 2. Create PC
                const pc = new RTCPeerConnection();
                pcRef.current = pc;

                pc.ontrack = (event) => {
                    console.log("[MonitorFlow] Received WebRTC Track:", event.track.kind);
                    if (videoElement && event.streams[0]) {
                        videoElement.srcObject = event.streams[0];
                    }
                };

                // Add transceivers for both video and audio for better compatibility
                pc.addTransceiver("video", { direction: "recvonly" });
                pc.addTransceiver("audio", { direction: "recvonly" });

                // 4. Create local offer
                const offer = await pc.createOffer();
                await pc.setLocalDescription(offer);

                // 5. Exchange SDP via SRS Signal API
                const response = await fetch(api, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        api: api,
                        streamurl: url,
                        sdp: offer.sdp
                    })
                });

                if (!response.ok) {
                    throw new Error(`网络请求失败: ${response.statusText}`);
                }

                const data = await response.json();
                if (data.code !== 0) {
                    // SRS Code 400 usually means 'Stream Not Found', common during startup
                    const errMsg = data.message || data.desc || data.server || "未知错误";
                    throw new Error(`SRS Error ${data.code}: ${errMsg}`);
                }

                // 6. Set Remote Answer
                console.log("[MonitorFlow] WebRTC Answer Accepted.");
                await pc.setRemoteDescription(new RTCSessionDescription({ type: 'answer', sdp: data.sdp }));

            } catch (e: any) {
                console.error("[MonitorFlow] WebRTC Play Failed:", e);
                if (retryCount < maxRetries) {
                    setError(`监控流加载中，正在重试 (${retryCount + 1}/${maxRetries})...`);
                    setTimeout(() => {
                        setRetryCount(prev => prev + 1);
                    }, 2500); // 2.5s interval
                } else {
                    setError("播放失败: " + (e.message || "请求超时"));
                }
            }
        };

        const handlePlaying = () => {
            console.log("[MonitorFlow] WebRTC Video is PLAYING - clearing overlays.");
            setError("");
            setRetryCount(0);
        };
        videoElement.addEventListener('playing', handlePlaying);

        startPlaying();

        return () => {
            videoElement.removeEventListener('playing', handlePlaying);
            stop();
        };
    }, [url, retryCount]);

    return (
        <div className="absolute inset-0 bg-black flex flex-col items-center justify-center text-white z-50 overflow-hidden rounded-xl">
            <div className="absolute top-4 left-4 z-20 flex items-center gap-2 bg-black/40 backdrop-blur px-3 py-1.5 rounded-lg text-xs font-bold font-mono">
                <div className="w-2 h-2 rounded-full bg-blue-500 animate-pulse"></div>
                RTC: {className}
                {retryCount > 0 && <span className="text-amber-400 ml-2">正在重连...</span>}
            </div>

            {error && !error.includes("重连") ? (
                <div className="text-center p-8 bg-zinc-900 rounded-2xl border border-zinc-800">
                    <Video size={48} className="text-zinc-600 mb-4 mx-auto" />
                    <p className="text-red-400 font-bold mb-2">WebRTC 播放失败</p>
                    <p className="text-zinc-500 text-xs font-mono max-w-md break-all">{error}</p>
                    <p className="text-zinc-600 text-xs mt-2">{url}</p>
                </div>
            ) : (
                <>
                    {error.includes("重连") && (
                        <div className="absolute inset-0 z-10 bg-black/60 backdrop-blur-sm flex items-center justify-center">
                            <div className="text-center">
                                <div className="w-12 h-12 border-4 border-blue-500 border-t-transparent rounded-full animate-spin mx-auto mb-4"></div>
                                <p className="text-sm font-bold text-blue-200">{error}</p>
                            </div>
                        </div>
                    )}
                    <video
                        ref={videoRef}
                        className="w-full h-full object-contain bg-black"
                        controls
                        autoPlay
                        muted={false}
                    />
                </>
            )}

            <button
                onClick={onClose}
                className="absolute bottom-8 bg-white/10 hover:bg-white/20 backdrop-blur-md text-white px-6 py-2.5 rounded-xl font-bold transition-all border border-white/10 hover:border-white/30 flex items-center gap-2 shadow-xl group"
            >
                <Power size={16} className="text-red-400 group-hover:text-red-300 transition-colors" />
                停止监控
            </button>
        </div>
    );
};

const FlvPlayer = ({ url, className, onClose }: { url: string, className: string, onClose: () => void }) => {
    const videoRef = React.useRef<HTMLVideoElement>(null);
    const playerRef = React.useRef<mpegts.Player | null>(null);
    const [error, setError] = useState("");
    const [retryCount, setRetryCount] = useState(0);
    const maxRetries = 3;

    useEffect(() => {
        if (!url || !videoRef.current) return;
        console.log("[MonitorFlow] FlvPlayer mounted. URL:", url, "Retry:", retryCount);

        if (!mpegts.getFeatureList().mseLivePlayback) {
            console.error("[MonitorFlow] Browser does not support MSE Live Playback.");
            setError("当前浏览器不支持 FLV 直播播放 (MSE Not Supported)");
            return;
        }

        const videoElement = videoRef.current;
        // Clear error when video actually starts playing
        const handlePlaying = () => {
            setError("");
            setRetryCount(0);
            console.log("[MonitorFlow] Video playing - cleared error/retries");
        };
        videoElement.addEventListener('playing', handlePlaying);

        const buildPlayer = () => {
            try {
                console.log("[MonitorFlow] Creating mpegts player... (Low Latency Mode)");
                const player = mpegts.createPlayer({
                    type: 'flv',
                    url: url,
                    isLive: true,
                    cors: true,
                    hasAudio: false, // Monitor usually has no audio, setting to false for faster loading
                }, {
                    enableWorker: true,
                    enableStashBuffer: false, // Crucial for low latency
                    stashInitialSize: 128,    // 128KB initial buffer
                    liveBufferLatencyChasing: true, // Auto-catch up to live edge
                    lazyLoadMaxDuration: 3 * 60,
                    seekType: 'range',
                });

                player.attachMediaElement(videoElement);
                player.load();

                const playPromise = player.play();
                if (playPromise instanceof Promise) {
                    playPromise.catch(e => {
                        if (e.name !== 'AbortError') {
                            console.error("[MonitorFlow] Play failed:", e);
                        }
                    });
                }

                player.on(mpegts.Events.ERROR, (type, detail) => {
                    console.error("[MonitorFlow] Mpegts Player Error:", type, detail);

                    if (retryCount < maxRetries) {
                        setError(`播放出错了，正在尝试重连 (${retryCount + 1}/${maxRetries})...`);
                        setTimeout(() => {
                            setRetryCount(prev => prev + 1);
                        }, 2000);
                    } else {
                        setError(`播放错误: ${type} ${detail}`);
                    }
                });

                playerRef.current = player;
            } catch (e: any) {
                console.error("[MonitorFlow] Failed to create player:", e);
                setError("播放器创建失败: " + e.message);
            }
        };

        buildPlayer();

        return () => {
            if (videoElement) {
                videoElement.removeEventListener('playing', handlePlaying);
            }
            if (playerRef.current) {
                console.log("[MonitorFlow] Destroying player.");
                playerRef.current.destroy();
                playerRef.current = null;
            }
        };
    }, [url, retryCount]); // Re-run effect on URL or retryCount change

    return (
        <div className="absolute inset-0 bg-black flex flex-col items-center justify-center text-white z-50 overflow-hidden rounded-xl">
            <div className="absolute top-4 left-4 z-20 flex items-center gap-2 bg-black/40 backdrop-blur px-3 py-1.5 rounded-lg text-xs font-bold font-mono">
                <div className="w-2 h-2 rounded-full bg-red-500 animate-pulse"></div>
                LIVE: {className}
                {retryCount > 0 && <span className="text-amber-400 ml-2">正在重连...</span>}
            </div>

            {error && !error.includes("重连") ? (
                <div className="text-center p-8 bg-zinc-900 rounded-2xl border border-zinc-800">
                    <Video size={48} className="text-zinc-600 mb-4 mx-auto" />
                    <p className="text-red-400 font-bold mb-2">无法播放视频</p>
                    <p className="text-zinc-500 text-xs font-mono max-w-md break-all">{error}</p>
                    <p className="text-zinc-600 text-xs mt-2">{url}</p>
                </div>
            ) : (
                <>
                    {error.includes("重连") && (
                        <div className="absolute inset-0 z-10 bg-black/60 backdrop-blur-sm flex items-center justify-center">
                            <div className="text-center">
                                <div className="w-12 h-12 border-4 border-sage-500 border-t-transparent rounded-full animate-spin mx-auto mb-4"></div>
                                <p className="text-sm font-bold text-sage-200">{error}</p>
                            </div>
                        </div>
                    )}
                    <video
                        ref={videoRef}
                        className="w-full h-full object-contain bg-black"
                        controls
                        autoPlay
                        muted={false}
                    />
                </>
            )}

            <button
                onClick={onClose}
                className="absolute bottom-8 bg-white/10 hover:bg-white/20 backdrop-blur-md text-white px-6 py-2.5 rounded-xl font-bold transition-all border border-white/10 hover:border-white/30 flex items-center gap-2 shadow-xl group"
            >
                <Power size={16} className="text-red-400 group-hover:text-red-300 transition-colors" />
                停止监控
            </button>
        </div>
    );
};

export default ClassScheduleWindow;
