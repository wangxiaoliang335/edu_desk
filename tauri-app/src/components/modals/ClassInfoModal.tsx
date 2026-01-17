import { useEffect, useState } from "react";
import { X, Users, Search, Plus, Minus, LogOut, Trash2, UserCheck } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";
import { getGroupMemberList, isSDKReady, dismissGroup, quitGroup, changeGroupOwner } from '../../utils/tim';
import DutyRosterModal from "./DutyRosterModal";
import WallpaperModal from "./WallpaperModal";
import CourseScheduleModal from "./CourseScheduleModal";

interface Props {
    isOpen: boolean;
    onClose: () => void;
    groupId?: string; // TIM Group ID or Class ID
    groupName?: string;
    isClassGroup?: boolean;
}

interface GroupMember {
    user_id: number;
    name: string;
    role: 'owner' | 'manager' | 'member';
    avatar?: string;
    phone?: string;
    teach_subjects?: string[];
}

const getRoleFromSelfRole = (role?: number): 'owner' | 'manager' | 'member' => {
    // 400: Owner, 300: Admin, 200: Member
    if (role === 400) return 'owner';
    if (role === 300) return 'manager';
    return 'member';
};

const ClassInfoModal = ({ isOpen, onClose, groupId, groupName, isClassGroup = true }: Props) => {
    const [members, setMembers] = useState<GroupMember[]>([]);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState("");
    const [searchTerm, setSearchTerm] = useState("");

    const [teachSubjects, setTeachSubjects] = useState<string[]>([]);
    const [newSubject, setNewSubject] = useState("");
    const [showAddSubject, setShowAddSubject] = useState(false);
    const [intercomEnabled, setIntercomEnabled] = useState(false);

    // Modal States
    const [showDutyRoster, setShowDutyRoster] = useState(false);
    const [showWallpaper, setShowWallpaper] = useState(false);
    const [showCourseSchedule, setShowCourseSchedule] = useState(false);
    const [showTransferModal, setShowTransferModal] = useState(false);

    // Owner state for Exit/Disband logic
    const [isOwner, setIsOwner] = useState(false);
    const [currentUserId, setCurrentUserId] = useState("");

    const normalizeSubjects = (value: any): string[] => {
        if (Array.isArray(value)) return value.filter((v) => typeof v === 'string' && v.trim());
        if (typeof value === 'string') {
            try {
                const parsed = JSON.parse(value);
                if (Array.isArray(parsed)) {
                    return parsed.filter((v) => typeof v === 'string' && v.trim());
                }
            } catch { }
            return value.split(',').map((v) => v.trim()).filter(Boolean);
        }
        return [];
    };

    useEffect(() => {
        if (isOpen && groupId) {
            setLoading(true);
            if (isClassGroup) {
                fetchClassMembers();
                // Load cached subjects
                const cached = localStorage.getItem(`teach_subjects_${groupId}`);
                if (cached) {
                    try {
                        setTeachSubjects(JSON.parse(cached));
                    } catch (e) { }
                } else {
                    setTeachSubjects([]);
                }
            } else {
                setLoading(false);
            }
        }
    }, [isOpen, groupId]);

    const fetchClassMembers = async () => {
        if (!groupId) return;
        setLoading(true);
        setError("");
        try {
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            // Try multiple sources for teacher_unique_id
            let currentUserId = localStorage.getItem('teacher_unique_id') || "0";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                    // Also check user_info for teacher_unique_id if not found
                    if (currentUserId === "0" && u.teacher_unique_id) {
                        currentUserId = String(u.teacher_unique_id);
                    }
                } catch (e) {
                    console.error("Failed to parse user_info", e);
                }
            }

            const resStr = await invoke<string>('get_group_members', {
                groupId: groupId,
                token
            });
            console.log("[ClassInfoModal] get_group_members 原始响应:", resStr);
            const res = JSON.parse(resStr);
            console.log("[ClassInfoModal] 解析后响应对象:", res);


            // Fetch TIM members reliably - Wait for SDK if needed
            let timMembers: any[] = [];

            // Wait up to 2 seconds for SDK to be ready
            let retries = 0;
            while (!isSDKReady && retries < 10) {
                await new Promise(r => setTimeout(r, 200));
                retries++;
            }

            if (isSDKReady) {
                try {
                    timMembers = await getGroupMemberList(groupId);
                    console.log(`[ClassInfoModal] Fetched ${timMembers.length} TIM members`);
                } catch (e) {
                    console.error("[ClassInfoModal] Failed to fetch TIM members", e);
                }
            } else {
                console.warn("[ClassInfoModal] SDK still not ready after waiting, skipping TIM fetch");
            }

            if (res.code === 200 || (res.data && res.data.code === 200)) {
                const list = res.data?.members || res.data || [];

                const groupInfo = res.data?.group_info;

                const mappedMembers: GroupMember[] = Array.isArray(list) ? list.map((m: any) => {
                    const userIdStr = String(m.user_id || m.id || m.Member_Account);
                    const timMember = timMembers.find((tm: any) => tm.userID == userIdStr);

                    // Avatar Fallback Logic
                    let avatarUrl = timMember?.avatar || m.face_url || m.avatar;
                    // If avatar is missing, and this is the Class bot (matches classid or name is '班级'), use group face_url
                    if (!avatarUrl && groupInfo) {
                        if (String(groupInfo.classid) === userIdStr || m.user_name === '班级' || m.name === '班级') {
                            avatarUrl = groupInfo.face_url;
                        }
                    }

                    return {
                        user_id: m.user_id || m.id || m.Member_Account,
                        name: m.user_name || m.name || m.student_name || m.NameCard || m.Nick || timMember?.nick || "未知用户",
                        role: m.role || getRoleFromSelfRole(m.self_role) || 'member',
                        avatar: avatarUrl,
                        phone: m.phone,
                        teach_subjects: m.teach_subjects || []
                    };
                }) : [];

                mappedMembers.sort((a, b) => {
                    const getScore = (m: GroupMember) => {
                        if (m.role === 'owner') return 4;
                        if (m.name === '班级') return 3;
                        if (m.role === 'manager') return 2;
                        return 1;
                    };
                    return getScore(b) - getScore(a);
                });

                setMembers(mappedMembers);

                // Detect owner status from TIM member list (more reliable)
                const currentUserIdLocal = currentUserId;
                const selfInTim = timMembers.find((tm: any) => tm.userID === currentUserIdLocal);
                console.log("[ClassInfoModal] Current User ID:", currentUserIdLocal);
                console.log("[ClassInfoModal] Self in TIM:", selfInTim);
                console.log("[ClassInfoModal] TIM Role:", selfInTim?.role);

                if (selfInTim && selfInTim.role === 'Owner') {
                    console.log("[ClassInfoModal] User is OWNER");
                    setIsOwner(true);
                } else {
                    // Fallback: check backend member data
                    const currentUserObj = mappedMembers.find(m => String(m.user_id) === currentUserIdLocal);
                    const isOwnerBackend = currentUserObj?.role === 'owner' ||
                        (currentUserObj as any)?.self_role === 400;
                    console.log("[ClassInfoModal] Backend role check:", currentUserObj?.role, isOwnerBackend);
                    setIsOwner(isOwnerBackend);
                }
                setCurrentUserId(currentUserIdLocal);

                // Teach subjects
                const currentUserObj = mappedMembers.find(m => String(m.user_id) === currentUserIdLocal);
                if (currentUserObj?.teach_subjects) {
                    const normalized = normalizeSubjects(currentUserObj.teach_subjects);
                    if (normalized.length > 0) {
                        setTeachSubjects(normalized);
                        localStorage.setItem(`teach_subjects_${groupId}`, JSON.stringify(normalized));
                    }
                }

                if (res.data?.group_info) {
                    setIntercomEnabled(!!res.data.group_info.enable_intercom);
                }

            } else {
                setMembers([]);
            }
        } catch (e: any) {
            console.error("Failed to fetch members:", e);
            setError(e.message || "加载失败");
        } finally {
            setLoading(false);
        }
    };

    const handleOpenSchedule = async () => {
        if (groupId) {
            setShowCourseSchedule(true);
        }
    };

    const handleAddSubject = () => {
        if (newSubject.trim()) {
            const updated = [...teachSubjects, newSubject.trim()];
            setTeachSubjects(updated);
            saveTeachSubjects(updated);
            setNewSubject("");
            setShowAddSubject(false);
        }
    };

    const removeSubject = (index: number) => {
        const updated = teachSubjects.filter((_, i) => i !== index);
        setTeachSubjects(updated);
        saveTeachSubjects(updated);
    };

    const saveTeachSubjects = async (subjects: string[]) => {
        const normalized = normalizeSubjects(subjects);
        localStorage.setItem(`teach_subjects_${groupId}`, JSON.stringify(normalized));

        if (!groupId) {
            alert('任教科目保存失败：缺少群ID');
            return;
        }
        try {
            const userInfoStr = localStorage.getItem('user_info');
            let userId = "";
            if (userInfoStr) {
                const u = JSON.parse(userInfoStr);
                userId = u.teacher_unique_id || "";
            }
            if (!userId) {
                userId = currentUserId || localStorage.getItem('teacher_unique_id') || "";
            }
            if (!userId) {
                alert('任教科目保存失败：缺少教师ID');
                return;
            }

            const resp = await invoke<string>('save_teach_subjects', {
                groupId,
                userId,
                teachSubjects: normalized
            });
            try {
                const parsed = JSON.parse(resp);
                const code = parsed?.data?.code ?? parsed?.code;
                if (code !== 200) {
                    alert(`任教科目保存失败：${parsed?.data?.message || parsed?.message || resp}`);
                }
            } catch {
                // If response is not JSON, assume success unless it looks like an error
                if (typeof resp === 'string' && /error|失败|缺少/i.test(resp)) {
                    alert(`任教科目保存失败：${resp}`);
                }
            }
        } catch (e) {
            console.error("Failed to save subjects", e);
            alert('任教科目保存失败，请重试');
        }
    }

    const handleToggleIntercom = async () => {
        if (!groupId) return;
        const newState = !intercomEnabled;
        setIntercomEnabled(newState);
        try {
            await invoke('toggle_group_intercom', {
                groupId,
                enable: newState
            });
        } catch (e) {
            console.error("Failed to toggle intercom", e);
            setIntercomEnabled(!newState);
            alert("切换对讲状态失败");
        }
    };

    // Handler: Exit Group
    const handleExitGroup = async () => {
        if (!groupId) return;

        if (isOwner) {
            // Owner needs to transfer ownership first
            const otherMembers = members.filter(m => String(m.user_id) !== currentUserId);
            if (otherMembers.length === 0) {
                alert('您是群内唯一成员，无法转让群主。请使用"解散群聊"。');
                return;
            }
            setShowTransferModal(true);
        } else {
            // Regular member can just quit
            if (!confirm('确定要退出群聊吗？')) return;
            try {
                await quitGroup(groupId);
                alert('已成功退出群聊');

                // Refresh class list (Tauri cross-window event)
                const { emit } = await import('@tauri-apps/api/event');
                await emit('refresh-class-list');

                // Call Server to Leave
                await invoke('request_server_leave_group', {
                    groupId: groupId,
                    userId: currentUserId
                });

                // Close any open windows for this group (schedule and chat)
                try {
                    const { WebviewWindow } = await import('@tauri-apps/api/webviewWindow');
                    // Close ClassScheduleWindow
                    const scheduleWindow = await WebviewWindow.getByLabel(`class_schedule_${groupId}`);
                    if (scheduleWindow) await scheduleWindow.close();
                    // Close ClassChatWindow
                    const chatWindow = await WebviewWindow.getByLabel(`class_chat_${groupId}`);
                    if (chatWindow) await chatWindow.close();
                } catch (e) {
                    console.log('[ClassInfoModal] No windows to close or error:', e);
                }

                onClose();
            } catch (e: any) {
                alert('退出群聊失败: ' + (e.message || e));
            }
        }
    };

    // Handler: Transfer Ownership and then Quit (for owner exit)
    const handleTransferAndQuit = async (newOwnerId: string) => {
        if (!groupId) return;
        try {
            await changeGroupOwner(groupId, newOwnerId);
            await quitGroup(groupId);
            // Call Server to Leave (after transfer and quit)
            await invoke('request_server_leave_group', {
                groupId: groupId,
                userId: currentUserId
            });
            alert('已成功转让群主并退出群聊');

            // Refresh class list (Tauri cross-window event)
            const { emit } = await import('@tauri-apps/api/event');
            await emit('refresh-class-list');

            // Close any open windows for this group (schedule and chat)
            try {
                const { WebviewWindow } = await import('@tauri-apps/api/webviewWindow');
                // Close ClassScheduleWindow
                const scheduleWindow = await WebviewWindow.getByLabel(`class_schedule_${groupId}`);
                if (scheduleWindow) await scheduleWindow.close();
                // Close ClassChatWindow
                const chatWindow = await WebviewWindow.getByLabel(`class_chat_${groupId}`);
                if (chatWindow) await chatWindow.close();
            } catch (e) {
                console.log('[ClassInfoModal] No windows to close or error:', e);
            }

            onClose();
        } catch (e: any) {
            alert('操作失败: ' + (e.message || e));
        }
        setShowTransferModal(false);
    };

    // Handler: Disband Group (Owner only)
    const handleDisbandGroup = async () => {
        if (!groupId) return;
        if (!confirm('确定要解散群聊吗？此操作不可恢复！')) return;

        try {
            await dismissGroup(groupId);
            // Call Server to Dismis
            await invoke('request_server_dismiss_group', {
                groupId: groupId,
                userId: currentUserId
            });
            // Success - proceed without alert

            // Refresh class list (Tauri cross-window event)
            const { emit } = await import('@tauri-apps/api/event');
            await emit('refresh-class-list');

            // Close any open windows for this group (schedule and chat)
            try {
                const { WebviewWindow } = await import('@tauri-apps/api/webviewWindow');
                // Close ClassScheduleWindow
                const scheduleWindow = await WebviewWindow.getByLabel(`class_schedule_${groupId}`);
                if (scheduleWindow) await scheduleWindow.close();
                // Close ClassChatWindow
                const chatWindow = await WebviewWindow.getByLabel(`class_chat_${groupId}`);
                if (chatWindow) await chatWindow.close();
            } catch (e) {
                console.log('[ClassInfoModal] No windows to close or error:', e);
            }

            onClose();
        } catch (e: any) {
            alert('解散群聊失败: ' + (e.message || e));
        }
    };

    if (!isOpen) return null;

    const filteredMembers = members.filter(m =>
        m.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
        m.phone?.includes(searchTerm)
    );

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm p-4 animate-in fade-in duration-300 font-sans">
            <div
                className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-full max-w-sm flex flex-col overflow-hidden animate-in zoom-in-95 duration-200 border border-white/60 ring-1 ring-sage-100/50"
                style={{ height: '700px' }}
            >
                {/* Header */}
                <div className="flex items-center justify-between p-5 border-b border-sage-100/50 bg-white/40 backdrop-blur-md">
                    <div className="flex items-center gap-3">
                        <div className="w-12 h-12 rounded-2xl bg-gradient-to-br from-sage-400 to-sage-600 text-white flex items-center justify-center font-bold text-lg shadow-lg shadow-sage-500/20">
                            {groupName ? groupName[0] : <Users size={24} />}
                        </div>
                        <div>
                            <h3 className="font-bold text-ink-800 text-lg tracking-tight">
                                {groupName || "班级信息"}
                            </h3>
                            <p className="text-xs text-ink-400 font-medium tracking-wide">ID: {groupId}</p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-10 h-10 flex items-center justify-center rounded-full text-sage-400 hover:text-clay-600 hover:bg-clay-50 transition-all duration-300"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Class Tools (Only for Class Groups) */}
                {isClassGroup && (
                    <div className="px-5 py-4 flex gap-2">
                        <button
                            onClick={handleOpenSchedule}
                            className="flex-1 bg-white border border-sage-200 hover:border-sage-400 text-ink-700 hover:text-sage-700 text-xs py-2.5 rounded-xl transition-all font-bold shadow-sm hover:shadow-md hover:-translate-y-0.5"
                        >
                            📅 班级课表
                        </button>
                        <button
                            onClick={() => setShowDutyRoster(true)}
                            className="flex-1 bg-white border border-sage-200 hover:border-sage-400 text-ink-700 hover:text-sage-700 text-xs py-2.5 rounded-xl transition-all font-bold shadow-sm hover:shadow-md hover:-translate-y-0.5"
                        >
                            🧹 值日表
                        </button>
                        <button
                            onClick={() => setShowWallpaper(true)}
                            className="flex-1 bg-white border border-sage-200 hover:border-sage-400 text-ink-700 hover:text-sage-700 text-xs py-2.5 rounded-xl transition-all font-bold shadow-sm hover:shadow-md hover:-translate-y-0.5"
                        >
                            🖼️ 壁纸
                        </button>
                    </div>
                )}

                {/* Member List Header */}
                <div className="px-5 py-2 bg-sage-50/50 border-y border-sage-100/50 flex justify-between items-center text-xs font-bold text-sage-500 uppercase tracking-wide">
                    <span>成员列表 ({members.length})</span>
                    <button className="text-sage-600 hover:text-sage-800 hover:underline decoration-sage-300 underline-offset-2 transition-all">+ 添加好友</button>
                </div>

                {/* Member List (Grid/List) */}
                <div className="flex-1 overflow-y-auto p-5 bg-white/30 custom-scrollbar">
                    {/* Search - simplified */}
                    <div className="mb-4 relative group">
                        <Search className="absolute left-3 top-1/2 -translate-y-1/2 text-sage-400 group-focus-within:text-sage-600 transition-colors" size={16} />
                        <input
                            type="text"
                            placeholder="搜索成员..."
                            value={searchTerm}
                            onChange={e => setSearchTerm(e.target.value)}
                            className="w-full pl-10 pr-4 py-2 bg-white/80 border border-sage-200 rounded-xl text-sm text-ink-700 placeholder:text-sage-300 focus:bg-white focus:ring-2 focus:ring-sage-100 focus:border-sage-400 transition-all outline-none shadow-sm"
                        />
                    </div>

                    {/* Error Message */}
                    {error && (
                        <div className="mb-4 px-3 py-2 bg-clay-50 text-clay-600 text-xs rounded-xl border border-clay-100">
                            {error}
                        </div>
                    )}

                    {loading ? (
                        <div className="flex justify-center py-8"><span className="loading loading-spinner text-sage-400 loading-md"></span></div>
                    ) : (
                        <div className="grid grid-cols-5 gap-3">
                            {filteredMembers.map(member => (
                                <div key={member.user_id} className="flex flex-col items-center gap-1 group cursor-pointer" title={member.name}>
                                    <div className="w-11 h-11 rounded-2xl bg-white flex items-center justify-center text-xs font-bold overflow-hidden border-2 border-transparent group-hover:border-sage-300 shadow-sm group-hover:shadow-md transition-all duration-300 relative">
                                        {member.avatar ? (
                                            <img src={member.avatar} alt="" className="w-full h-full object-cover" />
                                        ) : (
                                            <span className="text-sage-400 text-lg">{member.name[0]}</span>
                                        )}
                                    </div>
                                    <span className="text-[10px] text-ink-500 font-medium truncate w-full text-center group-hover:text-sage-600 transition-colors">{member.name}</span>
                                </div>
                            ))}

                            <button className="flex flex-col items-center gap-1 group">
                                <div className="w-11 h-11 rounded-2xl border-2 border-dashed border-sage-200 bg-white/50 flex items-center justify-center text-sage-300 group-hover:border-sage-400 group-hover:text-sage-500 group-hover:bg-sage-50 transition-all shadow-sm">
                                    <Plus size={20} strokeWidth={2.5} />
                                </div>
                                <span className="text-[10px] text-sage-300 group-hover:text-sage-500 font-bold transition-colors">添加</span>
                            </button>
                            <button className="flex flex-col items-center gap-1 group">
                                <div className="w-11 h-11 rounded-2xl border-2 border-dashed border-sage-200 bg-white/50 flex items-center justify-center text-sage-300 group-hover:border-clay-300 group-hover:text-clay-500 group-hover:bg-clay-50 transition-all shadow-sm">
                                    <Minus size={20} strokeWidth={2.5} />
                                </div>
                                <span className="text-[10px] text-sage-300 group-hover:text-clay-500 font-bold transition-colors">删除</span>
                            </button>

                        </div>
                    )}
                </div>

                {/* Teach Subjects (Class Groups Only) */}
                {isClassGroup && (
                    <div className="p-5 border-t border-sage-100/50 bg-white/40 backdrop-blur-md">
                        <div className="flex items-center justify-between mb-3">
                            <h4 className="text-xs font-bold text-sage-500 uppercase tracking-wide">任教科目</h4>
                            <button
                                onClick={() => setShowAddSubject(true)}
                                className="text-[10px] font-bold bg-white border border-sage-200 px-2 py-1 rounded-lg hover:bg-sage-50 hover:border-sage-300 hover:text-sage-700 transition-all shadow-sm"
                            >
                                + 添加
                            </button>
                        </div>
                        <div className="flex flex-wrap gap-2 min-h-[32px]">
                            {teachSubjects.map((subject, idx) => (
                                <div key={idx} className="flex items-center gap-1 bg-white border border-sage-200 px-3 py-1 rounded-full text-xs text-ink-600 font-medium shadow-sm hover:border-sage-300 hover:shadow-md transition-all cursor-default">
                                    <span>{subject}</span>
                                    <button onClick={() => removeSubject(idx)} className="text-sage-300 hover:text-clay-500 transition-colors ml-1"><X size={12} /></button>
                                </div>
                            ))}
                            {teachSubjects.length === 0 && !showAddSubject && (
                                <span className="text-xs text-sage-400 italic">暂无科目</span>
                            )}
                        </div>

                        {/* Add Subject Input */}
                        {showAddSubject && (
                            <div className="mt-3 flex gap-2 animate-in slide-in-from-top-2 duration-200">
                                <input
                                    autoFocus
                                    className="flex-1 text-xs border border-sage-300 rounded-lg px-3 py-1.5 outline-none focus:ring-2 focus:ring-sage-200 focus:border-sage-400 bg-white shadow-sm"
                                    placeholder="输入科目..."
                                    value={newSubject}
                                    onChange={e => setNewSubject(e.target.value)}
                                    onKeyDown={e => e.key === 'Enter' && handleAddSubject()}
                                />
                                <button onClick={handleAddSubject} className="bg-sage-600 text-white text-xs px-3 py-1.5 rounded-lg hover:bg-sage-700 shadow-sage-200 shadow-lg font-bold transition-all">确定</button>
                                <button onClick={() => setShowAddSubject(false)} className="text-sage-500 text-xs px-2 hover:bg-sage-100 rounded-lg transition-colors font-medium">取消</button>
                            </div>
                        )}
                    </div>
                )}

                {/* Intercom Toggle (Class Groups Only) - Qt Style */}
                {isClassGroup && (
                    <div className="px-5 py-4 border-t border-sage-100/50 bg-white/40 flex items-center justify-between backdrop-blur-md">
                        <div className="flex flex-col">
                            <span className="text-sm font-bold text-ink-800">开启对讲</span>
                            <span className="text-[10px] text-sage-400 font-medium mt-0.5">允许老师使用对讲功能</span>
                        </div>
                        <button
                            onClick={handleToggleIntercom}
                            className={`w-11 h-6 rounded-full flex items-center px-0.5 transition-all duration-300 shadow-inner ${intercomEnabled ? 'bg-sage-500' : 'bg-gray-200'}`}
                        >
                            <div className={`w-5 h-5 bg-white rounded-full shadow-md transform transition-transform duration-300 ${intercomEnabled ? 'translate-x-[20px]' : 'translate-x-0'}`} />
                        </button>
                    </div>
                )}

                {/* Exit / Disband Group Buttons (Class Groups Only) */}
                {isClassGroup && (
                    <div className="px-5 py-4 border-t border-sage-100/50 bg-sage-50/50 space-y-2 backdrop-blur-md">
                        <button
                            onClick={handleExitGroup}
                            className="w-full py-2.5 text-sm rounded-xl border border-gray-200 text-ink-600 hover:bg-white hover:border-gray-300 hover:text-ink-800 flex items-center justify-center gap-2 transition-all font-bold shadow-sm"
                        >
                            <LogOut size={16} />
                            退出群聊
                        </button>
                        {isOwner && (
                            <button
                                onClick={handleDisbandGroup}
                                className="w-full py-2.5 text-sm rounded-xl border border-clay-200/50 text-clay-600 bg-clay-50/50 hover:bg-clay-100 hover:border-clay-300 flex items-center justify-center gap-2 transition-all font-bold shadow-sm"
                            >
                                <Trash2 size={16} />
                                解散群聊
                            </button>
                        )}
                    </div>
                )}
            </div>

            {/* Sub Modals */}
            <DutyRosterModal
                isOpen={showDutyRoster}
                onClose={() => setShowDutyRoster(false)}
                groupId={groupId}
            />
            <WallpaperModal
                isOpen={showWallpaper}
                onClose={() => setShowWallpaper(false)}
                groupId={groupId}
            />
            <CourseScheduleModal
                isOpen={showCourseSchedule}
                onClose={() => setShowCourseSchedule(false)}
                classId={groupId}
            />

            {/* Transfer Ownership Modal (for owner exit) */}
            {showTransferModal && (
                <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/60 backdrop-blur-md animate-in fade-in duration-300">
                    <div className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[350px] p-6 border border-white/60">
                        <h2 className="text-lg font-bold text-ink-800 mb-2 flex items-center gap-2">
                            <UserCheck size={20} className="text-sage-500" />
                            选择新群主
                        </h2>
                        <p className="text-xs text-ink-400 font-medium mb-4">您是群主，退出前需先转让群主身份给其他成员。</p>

                        <div className="max-h-[250px] overflow-y-auto space-y-2 mb-4 custom-scrollbar pr-2">
                            {members.filter(m => String(m.user_id) !== currentUserId && m.name !== '班级' && m.user_id !== 0).map((member) => (
                                <button
                                    key={member.user_id}
                                    onClick={() => handleTransferAndQuit(String(member.user_id))}
                                    className="w-full flex items-center gap-3 p-3 rounded-xl hover:bg-white hover:shadow-md border border-transparent hover:border-sage-100 transition-all text-left group"
                                >
                                    <div className="w-9 h-9 rounded-full bg-white flex items-center justify-center text-xs text-sage-400 border border-sage-100 group-hover:border-sage-300 transition-colors">
                                        {member.avatar ? <img src={member.avatar} className="w-full h-full rounded-full object-cover" /> : member.name[0]}
                                    </div>
                                    <span className="text-sm font-bold text-ink-700 group-hover:text-sage-700 transition-colors">{member.name}</span>
                                    {member.role === 'manager' && <span className="text-[10px] bg-sage-100 text-sage-600 px-2 py-0.5 rounded-full font-bold ml-auto">管理员</span>}
                                </button>
                            ))}
                        </div>
                        <button
                            onClick={() => setShowTransferModal(false)}
                            className="w-full py-2.5 text-xs font-bold text-ink-500 hover:bg-white hover:text-ink-700 rounded-xl border border-transparent hover:border-sage-200 transition-all shadow-sm hover:shadow-md"
                        >
                            取消
                        </button>
                    </div>
                </div>
            )}
        </div>
    ); {/* CustomListModal Removed */ }
};

export default ClassInfoModal;
