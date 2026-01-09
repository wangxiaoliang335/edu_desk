import { useEffect, useState } from "react";
import { X, Users, Search, Plus, Minus } from "lucide-react"; // Updated imports
import { invoke } from "@tauri-apps/api/core";

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
    // 400: Owner, 300: Admin, 200: Member (based on logs: 400=owner? logs show 王晓露 self_role:400 is owner in Qt logic?)
    // Actually from logs: 
    // "110003" role:400 -> Role Stats: 1 Owner.
    // "110012" role:300 -> Role Stats: 1 Admin.
    // "000011002" role:200 -> Role Stats: 1 Member.
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
            // Token is stored in user_info JSON
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            let currentUserId = "0";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                    currentUserId = u.teacher_unique_id ? String(u.teacher_unique_id) : "0";
                } catch (e) {
                    console.error("Failed to parse user_info", e);
                }
            }

            const resStr = await invoke<string>('get_group_members', {
                groupId: groupId,
                token
            });
            console.log("get_group_members raw response:", resStr);
            const res = JSON.parse(resStr);
            console.log("get_group_members parsed:", res);

            if (res.code === 200 || (res.data && res.data.code === 200)) {
                const list = res.data?.members || res.data || [];
                console.log("Member list to map:", list);

                const mappedMembers: GroupMember[] = Array.isArray(list) ? list.map((m: any) => ({
                    user_id: m.user_id || m.id || m.Member_Account,
                    name: m.user_name || m.name || m.student_name || m.NameCard || m.Nick || "未知用户", // Added user_name
                    role: m.role || getRoleFromSelfRole(m.self_role) || 'member', // added helper if needed, or simple fallback
                    avatar: m.face_url || m.avatar, // Added face_url
                    phone: m.phone,
                    teach_subjects: m.teach_subjects || []
                })) : [];
                // Sort: Owner first, then "Class" (name=='班级'), then others
                mappedMembers.sort((a, b) => {
                    const getScore = (m: GroupMember) => {
                        if (m.role === 'owner') return 4;
                        if (m.name === '班级') return 3;
                        if (m.role === 'manager') return 2;
                        return 1;
                    };
                    return getScore(b) - getScore(a);
                });

                console.log("Mapped members:", mappedMembers);
                setMembers(mappedMembers);

                // Find current user's teach subjects from the list if not locally modified
                // Note: QGroupInfo logic says "if not dirty, use server data"
                // We'll trust the server data on load.
                const currentUser = mappedMembers.find(m => String(m.user_id) === currentUserId);
                if (currentUser && currentUser.teach_subjects && Array.isArray(currentUser.teach_subjects)) {
                    setTeachSubjects(currentUser.teach_subjects);
                    // Update cache
                    localStorage.setItem(`teach_subjects_${groupId}`, JSON.stringify(currentUser.teach_subjects));
                }

            } else {
                // throw new Error(res.message || res.msg || "获取成员失败");
                // Fallback for demo
                setMembers([
                    { user_id: 1, name: "张老师", role: 'owner', phone: "13800000001" },
                    { user_id: 2, name: "李小明", role: 'member', phone: "13800000002" },
                ]);
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
            try {
                await invoke('open_class_window', { groupclassId: groupId });
            } catch (e) {
                console.error("Failed to open schedule", e);
                alert("无法打开课程表窗口");
            }
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
        // Cache locally first
        localStorage.setItem(`teach_subjects_${groupId}`, JSON.stringify(subjects));

        if (!groupId) return;
        try {
            const userInfoStr = localStorage.getItem('user_info');
            let userId = "";
            if (userInfoStr) {
                const u = JSON.parse(userInfoStr);
                userId = u.teacher_unique_id || ""; // Backend expects 'user_id' which is teacher_unique_id
            }

            await invoke('save_teach_subjects', {
                groupId,
                userId,
                teachSubjects: subjects
            });
        } catch (e) {
            console.error("Failed to save subjects", e);
            // Revert state if needed, or just alert
        }
    }

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
                            onClick={() => alert("功能开发中...")}
                            className="flex-1 bg-gray-800 hover:bg-gray-700 text-white text-xs py-2 rounded-md transition-colors font-medium"
                        >
                            值日表
                        </button>
                        <button
                            onClick={() => alert("功能开发中...")}
                            className="flex-1 bg-gray-800 hover:bg-gray-700 text-white text-xs py-2 rounded-md transition-colors font-medium"
                        >
                            壁纸
                        </button>
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

                    {loading ? (
                        <div className="flex justify-center py-4"><span className="loading loading-spinner loading-sm"></span></div>
                    ) : (
                        <div className="grid grid-cols-6 gap-2">
                            {/* Add/Remove buttons mimicking Qt - Always at the end in Qt, but here we can put them first for visibility or last to match. Qt puts them at the end. */}

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

                            {/* Add/Remove buttons at the end like Qt */}
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
            </div>
        </div>
    );
};

export default ClassInfoModal;
