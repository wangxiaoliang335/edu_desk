import { useState, useEffect } from 'react';
import { X, Search, Users, User, Check } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { useWebSocket } from '../../context/WebSocketContext';
import { getTIMGroups, checkGroupsExist, invalidateTIMGroupsCache } from '../../utils/tim';

interface CreateClassGroupModalProps {
    isOpen: boolean;
    onClose: () => void;
    onSuccess?: () => void;
    userInfo: any;
}

interface ClassItem {
    class_id: string;
    class_name: string;
    grade: string;
}

interface FriendItem {
    teacher_info: {
        id: number;
        name: string;
        teacher_unique_id: string;
    };
    user_details: {
        avatar: string;
        phone: string;
    };
}

const CreateClassGroupModal = ({ isOpen, onClose, onSuccess, userInfo }: CreateClassGroupModalProps) => {
    const { sendMessage } = useWebSocket();
    const [searchTerm, setSearchTerm] = useState('');

    const [classes, setClasses] = useState<ClassItem[]>([]);
    const [friends, setFriends] = useState<FriendItem[]>([]);

    const [selectedClass, setSelectedClass] = useState<ClassItem | null>(null);
    const [selectedFriend, setSelectedFriend] = useState<FriendItem | null>(null);

    const [isLoading, setIsLoading] = useState(false);
    const [isSearching, setIsSearching] = useState(false);
    const [isFetchingFriends, setIsFetchingFriends] = useState(false);

    useEffect(() => {
        if (isOpen) {
            fetchClasses();  // Fetch teacher's classes
            fetchFriends();
            setSelectedClass(null);
            setSelectedFriend(null);
        }
    }, [isOpen]);

    const fetchFriends = async () => {
        if (!userInfo) return;
        setIsFetchingFriends(true);
        try {
            const idCard = userInfo.id_number || userInfo.strIdNumber || localStorage.getItem('id_number');
            const token = localStorage.getItem('token') || '';
            const res = await invoke<string>('get_user_friends', { idCard, token });
            const json = JSON.parse(res);
            if (json.friends && Array.isArray(json.friends)) {
                setFriends(json.friends);
            }
        } catch (e) {
            console.error("Failed to fetch friends:", e);
        } finally {
            setIsFetchingFriends(false);
        }
    };

    const fetchClasses = async () => {
        if (!userInfo) return;
        setIsSearching(true);
        try {
            const teacherId = userInfo.teacher_unique_id || localStorage.getItem('teacher_unique_id');
            const token = localStorage.getItem('token') || '';
            console.log('[CreateClassGroup] Fetching classes for teacher:', teacherId);

            // Fetch existing TIM groups to filter out classes with existing groups
            let existingGroupClassIds: Set<string> = new Set();
            try {
                const groups = await getTIMGroups();
                console.log('[CreateClassGroup] Existing TIM groups:', groups);
                // Class group IDs are formatted as: classId + "01"
                // So extract class IDs by removing trailing "01" from ChatRoom type groups
                groups.forEach((g: any) => {
                    if (g.type === 'ChatRoom' && g.groupID) {
                        // Class group ID ends with "01", extract the class_id (remove last 2 chars)
                        const groupId = String(g.groupID);
                        if (groupId.length > 2) {
                            const classId = groupId.slice(0, -2); // Remove "01" suffix
                            existingGroupClassIds.add(classId);
                            console.log('[CreateClassGroup] Found existing group for class:', classId);
                        }
                    }
                });
            } catch (e) {
                console.log('[CreateClassGroup] Could not fetch TIM groups:', e);
            }

            const res = await invoke<string>('get_teacher_classes', { teacherUniqueId: teacherId, token });
            console.log('[CreateClassGroup] Raw response:', res);
            const json = JSON.parse(res);
            console.log('[CreateClassGroup] Parsed JSON:', json);

            let classList: ClassItem[] = [];
            if (json.data) {
                if (Array.isArray(json.data)) {
                    // Map class_code to class_id
                    classList = json.data.map((c: any) => ({
                        class_id: c.class_code || c.class_id,
                        class_name: c.class_name,
                        grade: c.grade || ''
                    }));
                } else if (json.data.classes && Array.isArray(json.data.classes)) {
                    classList = json.data.classes.map((c: any) => ({
                        class_id: c.class_code || c.class_id,
                        class_name: c.class_name,
                        grade: c.grade || ''
                    }));
                }
            }

            // Check globally if duplicate groups exist (even if I am not in them)
            // Rule: GroupID = class_id + "01"
            if (classList.length > 0) {
                try {
                    const candidateIDs = classList.map(c => String(c.class_id) + "01");
                    console.log('[CreateClassGroup] Checking global existence for:', candidateIDs);
                    // Batch check if needed, but for typical use case (few classes), one call is fine.
                    // TIM limit usually 20-50.
                    const existingGlobals = await checkGroupsExist(candidateIDs);
                    console.log('[CreateClassGroup] Found existing global groups:', existingGlobals);

                    existingGlobals.forEach((gid: any) => {
                        const groupId = String(gid);
                        if (groupId.length > 2) {
                            const classId = groupId.slice(0, -2);
                            existingGroupClassIds.add(classId);
                        }
                    });
                } catch (e) {
                    console.error('[CreateClassGroup] Global group check failed', e);
                }
            }

            // Filter out classes that already have a class group
            const beforeFilter = classList.length;
            classList = classList.filter(c => !existingGroupClassIds.has(String(c.class_id)));
            console.log(`[CreateClassGroup] Filtered classes: ${beforeFilter} -> ${classList.length} (removed ${beforeFilter - classList.length} with existing groups)`);

            // Filter by search term if provided
            if (searchTerm.trim()) {
                classList = classList.filter(c =>
                    c.class_name.toLowerCase().includes(searchTerm.toLowerCase()) ||
                    c.class_id.includes(searchTerm) ||
                    c.grade.toLowerCase().includes(searchTerm.toLowerCase())
                );
            }

            console.log('[CreateClassGroup] Final class list:', classList);
            setClasses(classList);
        } catch (e) {
            console.error('[CreateClassGroup] Error fetching classes:', e);
            setClasses([]);
        } finally {
            setIsSearching(false);
        }
    };

    // Re-filter when searchTerm changes
    useEffect(() => {
        if (isOpen && classes.length > 0) {
            // Re-fetch to apply filter
            fetchClasses();
        }
    }, [searchTerm]);

    const handleCreate = async () => {
        if (!selectedClass || !userInfo) return;

        setIsLoading(true);
        try {
            const uniqueId = userInfo.teacher_unique_id || userInfo.id;
            const schoolId = userInfo.schoolId || userInfo.school_id;
            const classUniqueId = selectedClass.class_id;

            // Create group name: grade + class_name + "的班级群"
            const fullClassName = selectedClass.grade ? selectedClass.grade + selectedClass.class_name : selectedClass.class_name;
            const groupName = fullClassName + "的班级群";
            const classGroupUniqueId = classUniqueId + "01";

            // Load default avatar and convert to base64
            let avatarBase64 = "";
            try {
                const response = await fetch('/default_group_avatar.png');
                if (response.ok) {
                    const blob = await response.blob();
                    const reader = new FileReader();
                    avatarBase64 = await new Promise<string>((resolve) => {
                        reader.onloadend = () => {
                            // Remove the data:image/png;base64, prefix if present
                            const result = reader.result as string;
                            const base64 = result.includes('base64,')
                                ? result.split('base64,')[1]
                                : result;
                            resolve(base64);
                        };
                        reader.readAsDataURL(blob);
                    });
                    console.log('[CreateClassGroup] Default avatar loaded, base64 length:', avatarBase64.length);
                }
            } catch (e) {
                console.warn('[CreateClassGroup] Could not load default avatar:', e);
            }

            let members: any[] = [];

            if (selectedFriend) {
                members.push({
                    user_id: selectedFriend.teacher_info.teacher_unique_id,
                    user_name: selectedFriend.teacher_info.name,
                    group_role: 300, // Admin
                    join_time: Math.floor(Date.now() / 1000),
                    msg_flag: 0,
                    self_msg_flag: 0,
                    readed_seq: 0,
                    unread_num: 0
                });
            }

            const wsMessage = {
                type: "3",
                group_name: groupName,
                group_type: "Meeting",
                owner_identifier: uniqueId,
                classid: classUniqueId,
                schoolid: schoolId,
                is_class_group: 1,
                group_id: classGroupUniqueId,
                avatar_base64: avatarBase64,
                face_url: "",
                introduction: `班级群：${groupName}`,
                notification: `欢迎加入${groupName}`,
                max_member_num: 2000,
                searchable: 1,
                visible: 1,
                add_option: 0,
                members: members,
                member_info: {
                    user_id: uniqueId,
                    user_name: userInfo.name,
                    self_role: 400,
                    join_time: Math.floor(Date.now() / 1000),
                    msg_flag: 0,
                    self_msg_flag: 0,
                    readed_seq: 0,
                    unread_num: 0
                }
            };

            console.log("[CreateClassGroup] Sending WS:", wsMessage);
            const msgStr = `to::${JSON.stringify(wsMessage)}`;
            sendMessage(msgStr);

            const handleResponse = (e: any) => {
                try {
                    const data = JSON.parse(e.detail);
                    if (data.type === "3") {
                        window.removeEventListener('ws-message', handleResponse);
                        setIsLoading(false);

                        if (data.group_id) {
                            // Use the detailed message from server if available
                            alert(data.message || `班级群 "${data.groupname || groupName}" 创建成功`);

                            // Store the newly created group ID for immediate ownership detection
                            // This helps before TIM SDK syncs the owner info
                            const newGroupId = data.group_id;
                            if (newGroupId) {
                                const recentlyCreatedGroups = JSON.parse(sessionStorage.getItem('recentlyCreatedGroups') || '[]');
                                recentlyCreatedGroups.push(newGroupId);
                                sessionStorage.setItem('recentlyCreatedGroups', JSON.stringify(recentlyCreatedGroups));
                                console.log('[CreateClassGroup] Stored newly created group ID:', newGroupId);
                            }

                            // Invalidate cache and delay to allow TIM server to sync
                            // TIM cloud needs more time to propagate the new group
                            invalidateTIMGroupsCache();

                            // First refresh after 2 seconds
                            setTimeout(() => {
                                console.log('[CreateClassGroup] First refresh attempt');
                                onSuccess?.();

                                // Second refresh after 4 more seconds (total 6s)
                                // This catches cases where TIM sync is slow
                                setTimeout(() => {
                                    console.log('[CreateClassGroup] Second refresh attempt');
                                    invalidateTIMGroupsCache();
                                    onSuccess?.();
                                }, 4000);
                            }, 2000);

                            onClose();
                        } else {
                            alert(`创建失败: ${data.message || data.error || '未知错误'}`);
                        }
                    }
                } catch (err) {
                    console.error("Error parsing WS response:", err);
                }
            };

            window.addEventListener('ws-message', handleResponse);

            setTimeout(() => {
                window.removeEventListener('ws-message', handleResponse);
                if (isLoading) {
                    setIsLoading(false);
                }
            }, 10000);

        } catch (error) {
            console.error("Failed to create group:", error);
            alert(`创建失败: ${error}`);
            setIsLoading(false);
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[600px] max-h-[85vh] flex flex-col overflow-hidden transform scale-100 transition-all border border-white/50 ring-1 ring-sage-100/50">

                {/* Header */}
                <div className="flex items-center justify-between px-8 py-6 border-b border-sage-100/50 bg-white/30 backdrop-blur-sm">
                    <div>
                        <h2 className="text-2xl font-bold text-ink-800 tracking-tight">创建班级群</h2>
                        <p className="text-sm font-medium text-ink-400 mt-1">选择班级和协同教师，快速建立沟通群组。</p>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-10 h-10 flex items-center justify-center text-sage-400 hover:text-clay-600 hover:bg-clay-50 rounded-full transition-all duration-300"
                    >
                        <X size={24} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 p-8 flex flex-col gap-8 overflow-hidden bg-white/40 relative">

                    {/* Class Section */}
                    <div className="flex flex-col gap-4">
                        <div className="flex items-center justify-between">
                            <div className="flex items-center gap-2 text-sm font-bold text-ink-700 uppercase tracking-wide">
                                <Users size={18} className="text-sage-500" />
                                <span>选择班级</span>
                                {selectedClass && <span className="text-xs font-bold text-sage-600 bg-sage-50 border border-sage-100 px-2 py-0.5 rounded-lg ml-2 animate-in fade-in slide-in-from-left-2">已选: {selectedClass.class_name}</span>}
                            </div>

                            {/* Search within header line */}
                            <div className="flex items-center gap-2 bg-white/80 border border-sage-200 rounded-xl px-3 py-1.5 shadow-sm focus-within:ring-2 focus-within:ring-sage-100 focus-within:border-sage-400 transition-all group">
                                <Search size={14} className="text-sage-300 group-focus-within:text-sage-500 transition-colors" />
                                <input
                                    type="text"
                                    value={searchTerm}
                                    onChange={(e) => setSearchTerm(e.target.value)}
                                    placeholder="搜索班级..."
                                    className="w-24 text-sm font-medium text-ink-700 outline-none bg-transparent placeholder:text-sage-300"
                                />
                            </div>
                        </div>

                        <div className="bg-white/60 rounded-2xl border border-white/50 ring-1 ring-sage-50 h-[200px] overflow-y-auto shadow-sm p-3 custom-scrollbar backdrop-blur-sm">
                            <div className="flex flex-col gap-1.5">
                                {isSearching ? (
                                    <div className="flex flex-col items-center justify-center h-full text-sage-400 gap-2 font-bold animate-pulse">
                                        <div className="w-6 h-6 border-2 border-sage-200 border-t-sage-500 rounded-full animate-spin" />
                                        <div className="text-xs">搜索中...</div>
                                    </div>
                                ) : classes.length === 0 ? (
                                    <div className="flex flex-col items-center justify-center h-full text-sage-300 text-xs font-bold">
                                        <span>未找到班级</span>
                                    </div>
                                ) : (
                                    classes.map(cls => (
                                        <div
                                            key={cls.class_id}
                                            onClick={() => setSelectedClass(cls)}
                                            className={`flex items-center justify-between p-3 rounded-xl cursor-pointer transition-all border group ${selectedClass?.class_id === cls.class_id
                                                ? 'bg-sage-50 border-sage-200 shadow-sm'
                                                : 'bg-transparent border-transparent hover:bg-white/80 hover:shadow-sm hover:border-sage-50'
                                                }`}
                                        >
                                            <div className="flex items-center gap-3 overflow-hidden">
                                                <div className={`w-10 h-10 rounded-xl flex items-center justify-center shrink-0 text-sm font-bold transition-all shadow-sm ${selectedClass?.class_id === cls.class_id ? 'bg-sage-500 text-white' : 'bg-white text-sage-400 group-hover:text-sage-600'}`}>
                                                    {cls.class_name.substring(0, 1)}
                                                </div>
                                                <div className="flex flex-col min-w-0">
                                                    <div className={`text-sm font-bold truncate transition-colors ${selectedClass?.class_id === cls.class_id ? 'text-sage-800' : 'text-ink-700'}`}>
                                                        {cls.grade ? cls.grade + cls.class_name : cls.class_name}
                                                    </div>
                                                    <div className="text-[10px] font-mono text-ink-300 truncate tracking-wide">{cls.class_id}</div>
                                                </div>
                                            </div>

                                            {selectedClass?.class_id === cls.class_id && (
                                                <div className="w-6 h-6 bg-sage-500 rounded-full flex items-center justify-center shadow-md shrink-0 animate-in zoom-in-75">
                                                    <Check size={14} className="text-white" />
                                                </div>
                                            )}
                                        </div>
                                    ))
                                )}
                            </div>
                        </div>
                    </div>

                    {/* Teacher (Friend) Section */}
                    <div className="flex flex-col gap-4">
                        <div className="flex items-center justify-between">
                            <div className="flex items-center gap-2 text-sm font-bold text-ink-700 uppercase tracking-wide">
                                <User size={18} className="text-clay-500" />
                                <span>添加管理员 (可选)</span>
                                {selectedFriend && <span className="text-xs font-bold text-clay-600 bg-clay-50 border border-clay-100 px-2 py-0.5 rounded-lg ml-2 animate-in fade-in slide-in-from-left-2">已选: {selectedFriend.teacher_info.name}</span>}
                            </div>
                        </div>

                        <div className="bg-white/60 rounded-2xl border border-white/50 ring-1 ring-sage-50 h-[120px] overflow-y-auto shadow-sm p-3 custom-scrollbar backdrop-blur-sm">
                            {isFetchingFriends ? (
                                <div className="flex flex-col items-center justify-center h-full text-sage-400 gap-2 font-bold animate-pulse">
                                    <div className="w-5 h-5 border-2 border-clay-200 border-t-clay-500 rounded-full animate-spin" />
                                    <div className="text-xs">加载好友列表...</div>
                                </div>
                            ) : friends.length === 0 ? (
                                <div className="flex flex-col items-center justify-center h-full text-sage-300 text-xs font-bold">
                                    暂无教师好友
                                </div>
                            ) : (
                                <div className="grid grid-cols-2 gap-3">
                                    {friends.map(friend => (
                                        <div
                                            key={friend.teacher_info.teacher_unique_id}
                                            onClick={() => setSelectedFriend(selectedFriend?.teacher_info.teacher_unique_id === friend.teacher_info.teacher_unique_id ? null : friend)}
                                            className={`flex items-center gap-3 p-2.5 rounded-xl cursor-pointer transition-all border group ${selectedFriend?.teacher_info.teacher_unique_id === friend.teacher_info.teacher_unique_id
                                                ? 'bg-clay-50 border-clay-200 shadow-sm'
                                                : 'bg-transparent border-transparent hover:bg-white/80 hover:shadow-sm hover:border-clay-50'
                                                }`}
                                        >
                                            <div className="relative shrink-0">
                                                <div className="w-10 h-10 rounded-full bg-white shadow-sm overflow-hidden border border-white ring-1 ring-sage-50">
                                                    {friend.user_details?.avatar ? (
                                                        <img src={friend.user_details.avatar} className="w-full h-full object-cover" onError={(e) => e.currentTarget.style.display = 'none'} />
                                                    ) : (
                                                        <div className="w-full h-full flex items-center justify-center text-sm font-bold text-clay-400 bg-clay-50">
                                                            {friend.teacher_info.name.substring(0, 1)}
                                                        </div>
                                                    )}
                                                </div>
                                                {selectedFriend?.teacher_info.teacher_unique_id === friend.teacher_info.teacher_unique_id && (
                                                    <div className="absolute -bottom-1 -right-1 w-5 h-5 bg-clay-500 rounded-full flex items-center justify-center border-2 border-white shadow-sm animate-in zoom-in-75">
                                                        <Check size={10} className="text-white" />
                                                    </div>
                                                )}
                                            </div>
                                            <div className="flex flex-col min-w-0">
                                                <div className={`text-xs font-bold truncate transition-colors ${selectedFriend?.teacher_info.teacher_unique_id === friend.teacher_info.teacher_unique_id ? 'text-clay-700' : 'text-ink-700'}`}>
                                                    {friend.teacher_info.name}
                                                </div>
                                                <div className="text-[10px] font-mono text-ink-300 truncate">{friend.user_details?.phone || '无电话'}</div>
                                            </div>
                                        </div>
                                    ))}
                                </div>
                            )}
                        </div>
                    </div>

                </div>

                {/* Footer */}
                <div className="p-6 bg-white/50 backdrop-blur-md flex justify-end gap-3 border-t border-sage-100/50">
                    <button
                        onClick={onClose}
                        className="px-6 py-3 bg-white text-ink-500 rounded-2xl hover:bg-sage-50 font-bold text-sm transition-all border border-sage-200 hover:border-sage-300 shadow-sm hover:shadow active:scale-95"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleCreate}
                        disabled={isLoading || !selectedClass}
                        className="px-8 py-3 bg-sage-600 text-white rounded-2xl hover:bg-sage-700 font-bold text-sm transition-all shadow-md shadow-sage-500/20 hover:shadow-lg hover:shadow-sage-500/30 disabled:opacity-50 disabled:cursor-not-allowed disabled:shadow-none flex items-center gap-2 active:scale-95"
                    >
                        {isLoading ? (
                            <>
                                <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin" />
                                <span>创建中...</span>
                            </>
                        ) : '确认创建'}
                    </button>
                </div>

            </div>
        </div>
    );
};

export default CreateClassGroupModal;
