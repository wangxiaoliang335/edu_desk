import { useState, useEffect } from 'react';
import { createPortal } from 'react-dom';
import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import { ChevronRight, ChevronDown, User, Users, School, MessageCircle, LogOut, UserMinus } from 'lucide-react';
import { loginTIM, getTIMGroups, setCachedTIMGroups } from '../utils/tim';

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

const LeafNode = ({ label, subLabel, icon, onClick, onDoubleClick, onContextMenu, avatarUrl }: { label: string, subLabel?: string, icon?: React.ReactNode, onClick?: () => void, onDoubleClick?: () => void, onContextMenu?: (e: React.MouseEvent) => void, avatarUrl?: string }) => {
    return (
        <div
            onClick={onClick}
            onDoubleClick={onDoubleClick}
            onContextMenu={onContextMenu}
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

    // Context Menu State
    const [contextMenu, setContextMenu] = useState<{
        visible: boolean;
        x: number;
        y: number;
        type: 'class' | 'friend' | null;
        data: any;
    }>({ visible: false, x: 0, y: 0, type: null, data: null });

    // Handle closing context menu (Resize/Scroll)
    useEffect(() => {
        const handleClose = () => setContextMenu(prev => ({ ...prev, visible: false }));
        window.addEventListener('resize', handleClose);
        window.addEventListener('scroll', handleClose, true); // Capture scroll
        return () => {
            window.removeEventListener('resize', handleClose);
            window.removeEventListener('scroll', handleClose, true);
        };
    }, []);

    const handleContextMenu = (e: React.MouseEvent, type: 'class' | 'friend', data: any) => {
        e.preventDefault();
        e.stopPropagation();
        setContextMenu({
            visible: true,
            x: e.clientX,
            y: e.clientY,
            type,
            data
        });
    };

    const handleLeaveClass = async (e: React.MouseEvent) => {
        e.stopPropagation();
        setContextMenu(prev => ({ ...prev, visible: false }));

        if (!contextMenu.data || !userInfo?.teacher_unique_id) return;
        const classCode = contextMenu.data.class_code;
        if (confirm(`确定要退出班级 "${contextMenu.data.class_name || classCode}" 吗？`)) {
            try {
                await invoke('leave_class', {
                    teacherUniqueId: userInfo.teacher_unique_id,
                    classCode: classCode
                });
                alert('已退出班级');
                fetchData(); // Refresh list
            } catch (e) {
                console.error(e);
                alert('退出班级失败: ' + String(e));
            }
        }
    };

    const handleUnfriend = async (e: React.MouseEvent) => {
        e.stopPropagation();
        setContextMenu(prev => ({ ...prev, visible: false }));

        if (!contextMenu.data || !userInfo?.teacher_unique_id) return;
        const friendId = contextMenu.data.teacher_info?.teacher_unique_id;
        const friendName = contextMenu.data.teacher_info?.name || '该好友';

        if (!friendId) {
            alert('无法获取好友ID');
            return;
        }

        if (confirm(`确定要解除与 "${friendName}" 的好友关系吗？`)) {
            try {
                await invoke('remove_friend', {

                    teacherUniqueId: userInfo.teacher_unique_id,
                    friendTeacherUniqueId: friendId
                });
                alert('已解除好友关系');
                fetchData(); // Refresh list
            } catch (e) {
                console.error(e);
                alert('解除好友失败: ' + String(e));
            }
        }
    };


    useEffect(() => {
        fetchData();

        const handleRefresh = () => {
            console.log('Received refresh-class-list event (browser)');
            fetchData();
        };
        window.addEventListener('refresh-class-list', handleRefresh);

        // Also listen for Tauri events from other windows
        let unlisten: (() => void) | undefined;
        listen<void>('refresh-class-list', () => {
            console.log('Received refresh-class-list event (Tauri)');
            fetchData();
        }).then(fn => { unlisten = fn; });

        return () => {
            window.removeEventListener('refresh-class-list', handleRefresh);
            if (unlisten) unlisten();
        };
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
                        // Cache the groups for CreateClassGroupModal to use
                        setCachedTIMGroups(timGroups);

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
                                    onContextMenu={(e) => handleContextMenu(e, 'class', cls)}
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
                                        onContextMenu={(e) => handleContextMenu(e, 'friend', friend)}
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
                        状态: {debugMsg === 'UserInfo is null' ? '用户信息为空' : debugMsg} <br />
                        教师ID: {userInfo?.teacher_unique_id ? '已获取' : '未获取'} <br />
                        身份证号: {userInfo?.id_number ? '已获取' : '未获取'}
                    </div>
                )}
            </div>

            {/* Context Menu - Portaled to body to avoid transform issues */}
            {contextMenu.visible && createPortal(
                <>
                    <div className="fixed inset-0 z-50 bg-transparent" onClick={() => setContextMenu(prev => ({ ...prev, visible: false }))} />
                    <div
                        className="fixed bg-white/95 backdrop-blur-sm shadow-xl rounded-xl py-1.5 z-50 border border-gray-100 min-w-[160px] animate-in fade-in zoom-in-95 duration-100"
                        style={{ top: contextMenu.y, left: contextMenu.x }}
                    >
                        {contextMenu.type === 'class' && (
                            <button
                                onClick={handleLeaveClass}
                                className="w-full text-left px-3 py-2 text-sm text-red-600 hover:bg-red-50 flex items-center gap-2.5 transition-colors group"
                            >
                                <LogOut size={16} className="text-red-500 group-hover:text-red-600" />
                                <span className="font-medium">退出班级</span>
                            </button>
                        )}
                        {contextMenu.type === 'friend' && (
                            <button
                                onClick={handleUnfriend}
                                className="w-full text-left px-3 py-2 text-sm text-red-600 hover:bg-red-50 flex items-center gap-2.5 transition-colors group"
                            >
                                <UserMinus size={16} className="text-red-500 group-hover:text-red-600" />
                                <span className="font-medium">解除好友</span>
                            </button>
                        )}
                    </div>
                </>,
                document.body
            )}
        </div>
    );
};

export default ClassManagement;
