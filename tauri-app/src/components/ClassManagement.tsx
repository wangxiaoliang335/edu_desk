import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { ChevronRight, ChevronDown, User, Users, School, MessageCircle } from 'lucide-react';
import { loginTIM, getTIMGroups } from '../utils/tim';

interface ClassInfo {
    class_code: string;
    class_name: string;
    school_stage: string;
    grade: string;
}

interface TeacherInfo {
    id: number;
    name: string;
    subject: string;
    teacher_unique_id: string;
    avatar?: string;
}

interface GroupInfo {
    group_id: string;
    group_name: string;
    face_url: string;
    is_class_group: number | boolean;
    classid?: string;
}

interface FriendData {
    teacher_info: TeacherInfo;
    user_details: {
        avatar: string;
        name: string;
        id_number: string;
    };
}

interface FriendsResponse {
    code: number;
    message: string;
    data: {
        owner_groups: GroupInfo[];
        member_groups: GroupInfo[];
    };
    friends: FriendData[];
}

interface ClassesResponse {
    data: {
        classes: ClassInfo[];
    };
}

const TreeNode = ({ label, icon, children, defaultOpen = false, count = 0 }: { label: string, icon: React.ReactNode, children?: React.ReactNode, defaultOpen?: boolean, count?: number }) => {
    const [isOpen, setIsOpen] = useState(defaultOpen);

    return (
        <div className="mb-1">
            <div
                className="flex items-center gap-2 p-2 hover:bg-gray-100 rounded-lg cursor-pointer text-gray-700 select-none transition-colors"
                onClick={() => setIsOpen(!isOpen)}
            >
                <div className="text-gray-400">
                    {children ? (isOpen ? <ChevronDown size={16} /> : <ChevronRight size={16} />) : <span className="w-4" />}
                </div>
                <div className="text-blue-500/80">{icon}</div>
                <span className="font-medium text-sm">{label}</span>
                {count > 0 && <span className="text-xs text-gray-400 ml-auto bg-gray-50 px-2 py-0.5 rounded-full">({count})</span>}
            </div>
            {isOpen && children && (
                <div className="ml-6 pl-2 border-l border-gray-100 mt-1 space-y-0.5">
                    {children}
                </div>
            )}
        </div>
    );
};

const LeafNode = ({ label, subLabel, icon, onClick, onDoubleClick, avatarUrl }: { label: string, subLabel?: string, icon?: React.ReactNode, onClick?: () => void, onDoubleClick?: () => void, avatarUrl?: string }) => {
    return (
        <div
            onClick={onClick}
            onDoubleClick={onDoubleClick}
            className="flex items-center gap-3 p-2 hover:bg-blue-50 hover:text-blue-600 rounded-lg cursor-pointer transition-all group"
        >
            {avatarUrl ? (
                <img src={avatarUrl} alt="" className="w-8 h-8 rounded-full bg-gray-200 object-cover shadow-sm" />
            ) : (
                <div className="w-8 h-8 rounded-full bg-gray-100 flex items-center justify-center text-gray-500 group-hover:bg-blue-100 group-hover:text-blue-600 transition-colors">
                    {icon || <User size={16} />}
                </div>
            )}
            <div className="flex flex-col">
                <span className="text-sm font-medium text-gray-700 group-hover:text-blue-700">{label}</span>
                {subLabel && <span className="text-xs text-gray-400 group-hover:text-blue-400">{subLabel}</span>}
            </div>
        </div>
    );
};

const ClassManagement = ({ userInfo }: { userInfo: any }) => {
    // console.log('ClassManagement Rendered. userInfo:', JSON.stringify(userInfo));
    const [activeTab, setActiveTab] = useState<'contacts' | 'groups'>('contacts');
    const [classes, setClasses] = useState<ClassInfo[]>([]);
    const [friends, setFriends] = useState<FriendData[]>([]);
    const [classGroups, setClassGroups] = useState<GroupInfo[]>([]);
    const [normalGroups, setNormalGroups] = useState<GroupInfo[]>([]);
    const [loading, setLoading] = useState(false);
    const [debugMsg, setDebugMsg] = useState('');

    useEffect(() => {
        fetchData();
    }, []);

    const fetchData = async () => {
        if (!userInfo) {
            console.warn('ClassManagement: userInfo is null');
            setDebugMsg('UserInfo is null');
            return;
        }
        setLoading(true);
        const token = localStorage.getItem('token') || '';

        try {
            // Priority: Prop > Data Prop > LocalStorage
            const teacherId = userInfo.teacher_unique_id || userInfo.data?.teacher_unique_id || localStorage.getItem('teacher_unique_id');
            const idCard = userInfo.id_number || userInfo.data?.id_number || userInfo.strIdNumber || localStorage.getItem('id_number');
            // User ID for TIM Login (usually teacher_unique_id or id_number, check C++: Login(teacher_unique_id))
            const timUserId = teacherId;

            const logMsg = `Fetching with TID: ${teacherId}, ID: ${idCard}`;
            console.log(logMsg);
            setDebugMsg(logMsg);

            // 1. Fetch Classes
            if (teacherId) {
                console.log('Fetching classes for:', teacherId);
                const classResText = await invoke<string>('get_teacher_classes', { teacherUniqueId: teacherId, token });
                const classRes = JSON.parse(classResText);

                let classesList: ClassInfo[] = [];
                if (classRes.data) {
                    if (Array.isArray(classRes.data)) {
                        classesList = classRes.data;
                    } else if (classRes.data.classes && Array.isArray(classRes.data.classes)) {
                        classesList = classRes.data.classes;
                    }
                }
                if (classesList.length > 0) setClasses(classesList);
            }

            // 2. Fetch Friends & Server Groups info (for enrichment)
            let serverGroupsMap = new Map<string, GroupInfo>();
            if (idCard) {
                console.log('Fetching friends for:', idCard);
                const friendsResText = await invoke<string>('get_user_friends', { idCard, token });
                const friendsRes = JSON.parse(friendsResText) as FriendsResponse;

                if (friendsRes.friends) {
                    setFriends(friendsRes.friends);
                }

                if (friendsRes.data) {
                    const allGroups = [...(friendsRes.data.owner_groups || []), ...(friendsRes.data.member_groups || [])];
                    allGroups.forEach(g => serverGroupsMap.set(g.group_id, g));
                }
            }

            // 3. TIM Login & Group Fetching
            if (timUserId) {
                try {
                    // Get UserSig
                    console.log('Fetching UserSig for:', timUserId);
                    const userSig = await invoke<string>('get_user_sig', { userId: timUserId });
                    console.log('Got UserSig, Logging into TIM...');

                    const loginSuccess = await loginTIM(timUserId, userSig);
                    if (loginSuccess) {
                        const timGroups = await getTIMGroups();

                        const cGroups: GroupInfo[] = [];
                        const nGroups: GroupInfo[] = [];

                        // Merge TIM groups with Server info
                        timGroups.forEach((timGroup: any) => {
                            const groupId = timGroup.groupID;
                            const groupType = timGroup.type; // "Meeting" (Class) or "Public" (Normal)
                            const serverInfo = serverGroupsMap.get(groupId);

                            console.log(`TIM Group: ${timGroup.name} (ID: ${groupId}, Type: ${groupType}) | On Server: ${!!serverInfo}`);

                            // Construct merged group info
                            const groupInfo: GroupInfo = {
                                group_id: groupId,
                                group_name: timGroup.name || serverInfo?.group_name || '未命名群聊',
                                face_url: timGroup.avatar || serverInfo?.face_url || '',
                                // Classification Logic:
                                // 1. "ChatRoom" or "Meeting" -> Class Group (based on C++ Meeting type mapping to ChatRoom in SDK)
                                // 2. "Public" -> Normal Group
                                // 3. Fallback to Server Info
                                is_class_group: groupType === 'Meeting' || groupType === 'ChatRoom' || (serverInfo ? (serverInfo.is_class_group === 1 || String(serverInfo.is_class_group) === 'true') : false),
                                classid: serverInfo?.classid
                            };

                            console.log(`  -> Classification Result for ${groupInfo.group_id}: is_class_group=${groupInfo.is_class_group}`);

                            if (groupInfo.is_class_group) {
                                cGroups.push(groupInfo);
                            } else {
                                nGroups.push(groupInfo);
                            }
                        });

                        console.log(`TIM Groups Processed. Class: ${cGroups.length}, Normal: ${nGroups.length}`);
                        setClassGroups(cGroups);
                        setNormalGroups(nGroups);
                    }
                } catch (timErr) {
                    console.error('TIM Error:', timErr);
                    setDebugMsg(prev => prev + ' | TIM Error');
                }
            }

        } catch (e) {
            console.error('Failed to fetch contact info', e);
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="flex flex-col h-full bg-white/50 backdrop-blur-sm rounded-xl overflow-hidden border border-white/40 shadow-sm">
            {/* Header Tabs */}
            <div className="flex items-center shrink-0 bg-gray-50/50 p-1 m-2 rounded-lg border border-gray-100">
                <button
                    onClick={() => setActiveTab('contacts')}
                    className={`flex-1 py-1.5 text-sm font-medium rounded-md transition-all ${activeTab === 'contacts' ? 'bg-white text-blue-600 shadow-sm' : 'text-gray-500 hover:text-gray-700'}`}
                >
                    好友 / 班级
                </button>
                <div className="w-[1px] h-4 bg-gray-200 mx-1"></div>
                <button
                    onClick={() => setActiveTab('groups')}
                    className={`flex-1 py-1.5 text-sm font-medium rounded-md transition-all ${activeTab === 'groups' ? 'bg-white text-blue-600 shadow-sm' : 'text-gray-500 hover:text-gray-700'}`}
                >
                    群聊
                </button>
            </div>

            {/* List Content */}
            <div className="flex-1 overflow-auto p-4 pt-0 custom-scrollbar">
                {loading && <div className="text-center text-gray-400 text-sm py-8">加载中...</div>}

                {!loading && activeTab === 'contacts' && (
                    <div className="space-y-1">
                        <TreeNode label="班级" count={classes.length} icon={<School size={18} />} defaultOpen={true}>
                            {classes.slice(0, 100).map((cls, idx) => (
                                <LeafNode
                                    key={idx}
                                    label={cls?.class_name || cls?.class_code || '未知班级'}
                                    subLabel={`${cls?.school_stage || ''}${cls?.grade || ''}`}
                                    icon={<Users size={16} />}
                                />
                            ))}
                        </TreeNode>
                        <TreeNode label="教师" count={friends.length} icon={<User size={18} />} defaultOpen={true}>
                            {friends.slice(0, 100).map((friend, idx) => {
                                const tInfo = friend?.teacher_info || {};
                                const uDetail = friend?.user_details || {};
                                return (
                                    <LeafNode
                                        key={idx}
                                        label={tInfo?.name || uDetail?.name || '未知用户'}
                                        subLabel={tInfo?.subject || ''}
                                        avatarUrl={uDetail?.avatar}
                                    />
                                );
                            })}
                        </TreeNode>
                    </div>
                )}

                {!loading && activeTab === 'groups' && (
                    <div className="space-y-1">
                        <TreeNode label="班级群" count={classGroups.length} icon={<Users size={18} />} defaultOpen={true}>
                            {classGroups.slice(0, 100).map((g, idx) => (
                                <LeafNode
                                    key={idx}
                                    label={g?.group_name || '未命名群组'}
                                    avatarUrl={g?.face_url}
                                    icon={<MessageCircle size={16} />}
                                    onDoubleClick={() => {
                                        console.log('Open Class Group:', g.group_name, g.group_id);
                                        invoke('open_class_window', { groupclassId: g.group_id });
                                    }}
                                />
                            ))}
                        </TreeNode>
                        <TreeNode label="普通群" count={normalGroups.length} icon={<MessageCircle size={18} />} defaultOpen={true}>
                            {normalGroups.slice(0, 100).map((g, idx) => (
                                <LeafNode
                                    key={idx}
                                    label={g?.group_name || '未命名群组'}
                                    avatarUrl={g?.face_url}
                                    onDoubleClick={() => {
                                        console.log('Open Normal Group:', g.group_name, g.group_id);
                                        invoke('open_class_window', { groupclassId: g.group_id });
                                    }}
                                />
                            ))}
                        </TreeNode>
                    </div>
                )}

                {/* Debug Info */}
                {classes.length === 0 && friends.length === 0 && classGroups.length === 0 && normalGroups.length === 0 && (
                    <div className="p-4 text-xs text-gray-400 border-t border-gray-100 mt-4">
                        Status: {debugMsg} <br />
                        TeacherID present: {userInfo?.teacher_unique_id ? 'Yes' : 'No'} <br />
                        IDCard present: {userInfo?.id_number ? 'Yes' : 'No'}
                    </div>
                )}
            </div>
        </div>
    );
};

export default ClassManagement;
