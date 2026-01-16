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
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm p-4">
            <div
                className="bg-white rounded-xl shadow-2xl w-full max-w-sm flex flex-col overflow-hidden animate-in fade-in zoom-in-95 duration-200"
                style={{ height: '700px' }}
            >
                {/* Header */}
                <div className="flex items-center justify-between p-4 border-b border-gray-100 bg-gray-50/50">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-full bg-blue-100 text-blue-600 flex items-center justify-center font-bold text-sm">
                            {groupName ? groupName[0] : <Users size={20} />}
                        </div>
                        <div>
                            <h3 className="font-bold text-gray-800 text-lg">
                                {groupName || "班级信息"}
                            </h3>
                            <p className="text-xs text-gray-400">ID: {groupId}</p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="p-1.5 hover:bg-gray-200 rounded-full text-gray-500 transition-colors"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Class Tools (Only for Class Groups) */}
                {isClassGroup && (
                    <div className="px-4 py-3 flex gap-2">
                        <button
                            onClick={handleOpenSchedule}
                            className="flex-1 bg-gray-800 hover:bg-gray-700 text-white text-xs py-2 rounded-md transition-colors font-medium"
                        >
                            班级课程表
                        </button>
                        <button
                            onClick={() => setShowDutyRoster(true)}
                            className="flex-1 bg-gray-800 hover:bg-gray-700 text-white text-xs py-2 rounded-md transition-colors font-medium"
                        >
                            值日表
                        </button>
                        <button
                            onClick={() => setShowWallpaper(true)}
                            className="flex-1 bg-gray-800 hover:bg-gray-700 text-white text-xs py-2 rounded-md transition-colors font-medium"
                        >
                            壁纸
                        </button>
                        {/* More Import Removed */}
                    </div>
                )}

                {/* Member List Header */}
                <div className="px-4 py-2 bg-gray-50 border-y border-gray-100 flex justify-between items-center text-xs font-medium text-gray-500">
                    <span>成员列表 ({members.length})</span>
                    <button className="text-blue-600 hover:underline">+ 添加好友</button>
                </div>

                {/* Member List (Grid/List) */}
                <div className="flex-1 overflow-y-auto p-4 bg-white">
                    {/* Search - simplified */}
                    <div className="mb-3 relative">
                        <Search className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-300" size={14} />
                        <input
                            type="text"
                            placeholder="搜索..."
                            value={searchTerm}
                            onChange={e => setSearchTerm(e.target.value)}
                            className="w-full pl-8 pr-3 py-1.5 bg-gray-100 border-none rounded-md text-sm focus:bg-white focus:ring-1 focus:ring-blue-200 transition-all outline-none"
                        />
                    </div>

                    {/* Error Message */}
                    {error && (
                        <div className="mb-3 px-3 py-2 bg-red-50 text-red-600 text-xs rounded-md border border-red-100">
                            {error}
                        </div>
                    )}

                    {loading ? (
                        <div className="flex justify-center py-4"><span className="loading loading-spinner loading-sm"></span></div>
                    ) : (
                        <div className="grid grid-cols-6 gap-2">
                            {filteredMembers.map(member => (
                                <div key={member.user_id} className="flex flex-col items-center gap-1 group cursor-pointer" title={member.name}>
                                    <div className="w-10 h-10 rounded-full bg-gray-200 flex items-center justify-center text-xs font-bold overflow-hidden border border-gray-100 group-hover:border-blue-300 transition-colors">
                                        {member.avatar ? (
                                            <img src={member.avatar} alt="" className="w-full h-full object-cover" />
                                        ) : (
                                            <span className="text-gray-500">{member.name[0]}</span>
                                        )}
                                    </div>
                                    <span className="text-[10px] text-gray-500 truncate w-full text-center scale-90">{member.name}</span>
                                </div>
                            ))}

                            <button className="flex flex-col items-center gap-1 group">
                                <div className="w-10 h-10 rounded-full border border-dashed border-gray-300 flex items-center justify-center text-gray-400 hover:border-blue-400 hover:text-blue-500 hover:bg-blue-50 transition-colors">
                                    <Plus size={16} />
                                </div>
                                <span className="text-[10px] text-gray-400">添加</span>
                            </button>
                            <button className="flex flex-col items-center gap-1 group">
                                <div className="w-10 h-10 rounded-full border border-dashed border-gray-300 flex items-center justify-center text-gray-400 hover:border-red-400 hover:text-red-500 hover:bg-red-50 transition-colors">
                                    <Minus size={16} />
                                </div>
                                <span className="text-[10px] text-gray-400">删除</span>
                            </button>

                        </div>
                    )}
                </div>

                {/* Teach Subjects (Class Groups Only) */}
                {isClassGroup && (
                    <div className="p-4 border-t border-gray-100 bg-gray-50">
                        <div className="flex items-center justify-between mb-2">
                            <h4 className="text-sm font-bold text-gray-700">任教科目</h4>
                            <button
                                onClick={() => setShowAddSubject(true)}
                                className="text-xs bg-white border border-gray-200 px-2 py-0.5 rounded hover:bg-blue-50 hover:border-blue-200 hover:text-blue-600 transition-colors"
                            >
                                + 添加
                            </button>
                        </div>
                        <div className="flex flex-wrap gap-2 min-h-[32px]">
                            {teachSubjects.map((subject, idx) => (
                                <div key={idx} className="flex items-center gap-1 bg-white border border-gray-200 px-2 py-1 rounded-full text-xs text-gray-600 shadow-sm">
                                    <span>{subject}</span>
                                    <button onClick={() => removeSubject(idx)} className="hover:text-red-500"><X size={12} /></button>
                                </div>
                            ))}
                            {teachSubjects.length === 0 && !showAddSubject && (
                                <span className="text-xs text-gray-400 italic">暂无科目</span>
                            )}
                        </div>

                        {/* Add Subject Input */}
                        {showAddSubject && (
                            <div className="mt-2 flex gap-2">
                                <input
                                    autoFocus
                                    className="flex-1 text-xs border border-gray-300 rounded px-2 py-1 outline-none focus:border-blue-500"
                                    placeholder="输入科目..."
                                    value={newSubject}
                                    onChange={e => setNewSubject(e.target.value)}
                                    onKeyDown={e => e.key === 'Enter' && handleAddSubject()}
                                />
                                <button onClick={handleAddSubject} className="bg-blue-600 text-white text-xs px-3 rounded hover:bg-blue-700">确定</button>
                                <button onClick={() => setShowAddSubject(false)} className="text-gray-500 text-xs px-2 hover:bg-gray-200 rounded">取消</button>
                            </div>
                        )}
                    </div>
                )}

                {/* Intercom Toggle (Class Groups Only) - Qt Style */}
                {isClassGroup && (
                    <div className="px-4 py-3 border-t border-gray-100 bg-gray-50 flex items-center justify-between">
                        <div className="flex flex-col">
                            <span className="text-sm font-bold text-gray-700">开启对讲</span>
                            <span className="text-[10px] text-gray-400">允许老师使用对讲功能</span>
                        </div>
                        <button
                            onClick={handleToggleIntercom}
                            className={`w-10 h-5 rounded-full flex items-center px-0.5 transition-colors duration-300 ${intercomEnabled ? 'bg-blue-500' : 'bg-gray-300'}`}
                        >
                            <div className={`w-4 h-4 bg-white rounded-full shadow-sm transform transition-transform duration-300 ${intercomEnabled ? 'translate-x-5' : 'translate-x-0'}`} />
                        </button>
                    </div>
                )}

                {/* Exit / Disband Group Buttons (Class Groups Only) */}
                {isClassGroup && (
                    <div className="px-4 py-3 border-t border-gray-100 bg-gray-50 space-y-2">
                        <button
                            onClick={handleExitGroup}
                            className="w-full py-2 text-sm rounded-lg border border-gray-300 text-gray-700 hover:bg-gray-100 flex items-center justify-center gap-2 transition-colors"
                        >
                            <LogOut size={14} />
                            退出群聊
                        </button>
                        {isOwner && (
                            <button
                                onClick={handleDisbandGroup}
                                className="w-full py-2 text-sm rounded-lg border border-red-200 text-red-600 hover:bg-red-50 flex items-center justify-center gap-2 transition-colors"
                            >
                                <Trash2 size={14} />
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
                <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/50 backdrop-blur-sm">
                    <div className="bg-white rounded-xl shadow-2xl w-[350px] p-5">
                        <h2 className="text-lg font-bold text-gray-800 mb-3 flex items-center gap-2">
                            <UserCheck size={18} className="text-blue-500" />
                            选择新群主
                        </h2>
                        <p className="text-xs text-gray-500 mb-3">您是群主，退出前需先转让群主身份给其他成员。</p>
                        <div className="max-h-[200px] overflow-y-auto space-y-1.5 mb-3">
                            {members.filter(m => String(m.user_id) !== currentUserId && m.name !== '班级' && m.user_id !== 0).map((member) => (
                                <button
                                    key={member.user_id}
                                    onClick={() => handleTransferAndQuit(String(member.user_id))}
                                    className="w-full flex items-center gap-2 p-2 rounded-lg hover:bg-blue-50 border border-gray-200 transition-colors text-left"
                                >
                                    <div className="w-7 h-7 rounded-full bg-gray-100 flex items-center justify-center text-xs text-gray-500 border border-gray-200">
                                        {member.avatar ? <img src={member.avatar} className="w-full h-full rounded-full" /> : member.name[0]}
                                    </div>
                                    <span className="text-sm text-gray-700">{member.name}</span>
                                    {member.role === 'manager' && <span className="text-[10px] bg-blue-100 text-blue-600 px-1 py-0.5 rounded">管理员</span>}
                                </button>
                            ))}
                        </div>
                        <button
                            onClick={() => setShowTransferModal(false)}
                            className="w-full py-1.5 text-xs text-gray-500 hover:bg-gray-100 rounded-lg transition-colors"
                        >
                            取消
                        </button>
                    </div>
                </div>
            )}
            {/* CustomListModal Removed */}
        </div>
    );
};

export default ClassInfoModal;
