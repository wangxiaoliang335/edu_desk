import { useState, useEffect } from 'react';
import { createPortal } from 'react-dom';
import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import { WebviewWindow } from '@tauri-apps/api/webviewWindow';
import { ChevronRight, ChevronDown, User, Users, School, MessageCircle, LogOut, UserMinus, Bell } from 'lucide-react';
import { loginTIM, getTIMGroups, setCachedTIMGroups } from '../utils/tim';
import { getLatestNotifications } from '../utils/websocket';
import NotificationCenterModal, { NotificationItem } from './modals/NotificationCenterModal';

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
                className="flex items-center gap-2 p-2 hover:bg-sage-50 rounded-xl cursor-pointer text-ink-600 select-none transition-colors group"
                onClick={() => setIsOpen(!isOpen)}
            >
                <div className="text-sage-300 group-hover:text-sage-500 transition-colors">
                    {children ? (isOpen ? <ChevronDown size={16} /> : <ChevronRight size={16} />) : <span className="w-4" />}
                </div>
                <div className="text-sage-500">{icon}</div>
                <span className="font-bold text-sm">{label}</span>
                {count > 0 && <span className="text-xs text-sage-400 ml-auto bg-sage-50 px-2 py-0.5 rounded-full font-bold">({count})</span>}
            </div>
            {isOpen && children && (
                <div className="ml-6 pl-2 border-l border-sage-100 mt-1 space-y-0.5">
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
            className="flex items-center gap-3 p-2 hover:bg-white hover:shadow-sm rounded-xl cursor-pointer transition-all group border border-transparent hover:border-sage-100 bg-transparent"
        >
            {avatarUrl ? (
                <img src={avatarUrl} alt="" className="w-9 h-9 rounded-full bg-sage-100 object-cover shadow-sm ring-2 ring-white" />
            ) : (
                <div className="w-9 h-9 rounded-full bg-sage-50 flex items-center justify-center text-sage-400 group-hover:bg-sage-100 group-hover:text-sage-600 transition-all shadow-sm ring-2 ring-transparent group-hover:ring-white">
                    {icon || <User size={16} />}
                </div>
            )}
            <div className="flex flex-col">
                <span className="text-sm font-bold text-ink-600 group-hover:text-sage-700 transition-colors">{label}</span>
                {subLabel && <span className="text-xs text-ink-300 group-hover:text-sage-400 transition-colors">{subLabel}</span>}
            </div>
        </div>
    );
};

const ClassManagement = ({ userInfo }: { userInfo: any }) => {
    // console.log('ClassManagement Rendered. userInfo:', JSON.stringify(userInfo));
    const [activeTab, setActiveTab] = useState<'contacts' | 'groups'>('contacts');
    const [classes, setClasses] = useState<ClassInfo[]>([]);
    const [friends, setFriends] = useState<FriendData[]>([]);
    const [managedClassGroups, setManagedClassGroups] = useState<GroupInfo[]>([]);
    const [joinedClassGroups, setJoinedClassGroups] = useState<GroupInfo[]>([]);
    const [managedNormalGroups, setManagedNormalGroups] = useState<GroupInfo[]>([]);
    const [joinedNormalGroups, setJoinedNormalGroups] = useState<GroupInfo[]>([]);
    const [loading, setLoading] = useState(false);
    const [debugMsg, setDebugMsg] = useState('');

    // Notification Center State
    const [isNotificationCenterOpen, setIsNotificationCenterOpen] = useState(false);
    const [notifications, setNotifications] = useState<NotificationItem[]>([]);

    // Normal Group Chat State - REMOVED for Independent Window
    // const [selectedNormalGroup, setSelectedNormalGroup] = useState<{ groupId: string; groupName: string; isOwner: boolean } | null>(null);

    const openNormalChatWindow = async (groupId: string, groupName: string) => {
        try {
            const label = `chat-${groupId.replace(/[^a-zA-Z0-9-/:_]/g, '_')}`;
            const existingWin = await WebviewWindow.getByLabel(label);
            if (existingWin) {
                await existingWin.setFocus();
                return;
            }

            const webview = new WebviewWindow(label, {
                url: `/chat/normal/${encodeURIComponent(groupId)}`,
                title: groupName || "群聊",
                width: 800,
                height: 600,
                center: true,
                resizable: true,
                decorations: false, // We have custom header
                transparent: true
            });

            webview.once('tauri://created', function () {
                // webview window successfully created
            });

            webview.once('tauri://error', function (e) {
                console.error("Failed to create chat window", e);
            });
        } catch (e) {
            console.error("Error opening chat window:", e);
        }
    };

    // Websocket Listener for Notifications
    useEffect(() => {
        // Load initial cached notifications
        const cached = getLatestNotifications();
        if (cached && cached.length > 0) {
            console.log("[Dashboard] Loaded cached notifications:", cached.length);
            setNotifications(cached as NotificationItem[]);
        }

        const handleWSMessage = (event: CustomEvent) => {
            try {
                const msg = JSON.parse(event.detail);
                if (msg.type === "unread_notifications") {
                    console.log("[Dashboard] Received Notifications Payload:", msg.data);
                    const newItems = (msg.data || []) as NotificationItem[];
                    setNotifications(prev => {
                        const existingMap = new Map(prev.map(item => [item.id, item]));
                        newItems.forEach(item => existingMap.set(item.id, item));
                        return Array.from(existingMap.values());
                    });
                }
            } catch (e) {
                console.error("[Dashboard] Error parsing WS message:", e);
            }
        };

        window.addEventListener('ws-message', handleWSMessage as EventListener);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage as EventListener);
        };
    }, []);

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
        const className = contextMenu.data.class_name || classCode;

        if (confirm(`确定要退出班级 "${className}" 吗？`)) {
            try {
                // Class List "Leave" only calls /teachers/classes/remove
                // It does NOT interact with TIM SDK directly.
                await invoke('leave_class', {
                    teacherUniqueId: userInfo.teacher_unique_id,
                    classCode: classCode
                });
                alert('已退出班级');
                fetchData(); // Refresh list
            } catch (e: any) {
                console.error(e);
                alert('退出班级失败: ' + (e.message || String(e)));
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
            let serverOwnerIds = new Set<string>();
            let serverMemberIds = new Set<string>();

            if (idCard) {
                console.log('Fetching friends for:', idCard);
                const friendsResText = await invoke<string>('get_user_friends', { idCard, token });
                const friendsRes = JSON.parse(friendsResText) as FriendsResponse;

                if (friendsRes.friends) {
                    setFriends(friendsRes.friends);
                }

                if (friendsRes.data) {
                    const ownerGroups = friendsRes.data.owner_groups || [];
                    const memberGroups = friendsRes.data.member_groups || [];

                    ownerGroups.forEach(g => {
                        serverGroupsMap.set(g.group_id, g);
                        serverOwnerIds.add(g.group_id);
                    });
                    memberGroups.forEach(g => {
                        serverGroupsMap.set(g.group_id, g);
                        serverMemberIds.add(g.group_id);
                    });
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

                        const mClassGroups: GroupInfo[] = [];
                        const jClassGroups: GroupInfo[] = [];
                        const mNormalGroups: GroupInfo[] = [];
                        const jNormalGroups: GroupInfo[] = [];

                        // Merge TIM groups with Server info
                        timGroups.forEach((timGroup: any) => {
                            const groupId = timGroup.groupID;
                            const groupType = timGroup.type; // "Meeting" (Class) or "Public" (Normal)
                            // Construct merged group info
                            // DEBUG: Log raw group type to check if "Public" is handled
                            // console.log(`[ClassManagement] Processing Group: ${groupId}, Type: ${groupType}, Name: ${timGroup.name}`);

                            const groupInfo: GroupInfo = {
                                group_id: groupId,
                                group_name: timGroup.name || '未命名群聊',
                                face_url: timGroup.avatar || '',
                                // Classification Logic:
                                // 1. "ChatRoom" or "Meeting" -> Class Group (based on C++ Meeting type mapping to ChatRoom in SDK)
                                // 2. "Public" -> Normal Group
                                // 3. Fallback: ID check
                                is_class_group: groupType === 'Meeting' || groupType === 'ChatRoom' || groupId.endsWith('01'),
                                classid: undefined
                            };

                            // Check ownership
                            // Priority: Server Data > TIM Data
                            let isOwner = false;
                            let ownershipSource = "TIM";

                            // Fallback logic preparation
                            const selfRole = timGroup.selfInfo?.role;
                            const recentlyCreatedGroups: string[] = JSON.parse(sessionStorage.getItem('recentlyCreatedGroups') || '[]');
                            const wasCreatedByMe = recentlyCreatedGroups.includes(groupId);

                            if (serverOwnerIds.has(groupId)) {
                                isOwner = true;
                                ownershipSource = "Server(Owner)";
                            } else if (serverMemberIds.has(groupId)) {
                                isOwner = false;
                                ownershipSource = "Server(Member)";
                            } else {
                                // Fallback to TIM logic
                                isOwner = timGroup.ownerID === timUserId ||
                                    ((!timGroup.ownerID || timGroup.ownerID === '') && selfRole === 400) ||
                                    ((!timGroup.ownerID || timGroup.ownerID === '') && wasCreatedByMe);
                            }

                            // console.log(`Group: ${groupInfo.group_id} (${groupInfo.group_name}), Source: ${ownershipSource}, IsOwner: ${isOwner}`);

                            if (groupInfo.is_class_group) {
                                if (isOwner) mClassGroups.push(groupInfo);
                                else jClassGroups.push(groupInfo);
                            } else {
                                if (isOwner) mNormalGroups.push(groupInfo);
                                else jNormalGroups.push(groupInfo);
                            }
                        });

                        // console.log(`TIM Groups Processed. Class(M/J): ${mClassGroups.length}/${jClassGroups.length}, Normal(M/J): ${mNormalGroups.length}/${jNormalGroups.length}`);
                        setManagedClassGroups(mClassGroups);
                        setJoinedClassGroups(jClassGroups);
                        setManagedNormalGroups(mNormalGroups);
                        setJoinedNormalGroups(jNormalGroups);
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
        <div className="flex flex-col h-full bg-paper/50 backdrop-blur-xl rounded-2xl overflow-hidden border border-sage-100 shadow-sm relative transition-all">
            {/* Header Tabs */}
            <div className="flex items-center shrink-0 bg-white/40 p-1.5 m-3 rounded-xl border border-sage-100">
                <button
                    onClick={() => setActiveTab('contacts')}
                    className={`flex-1 py-2 text-sm font-bold rounded-lg transition-all ${activeTab === 'contacts' ? 'bg-white text-sage-600 shadow-sm ring-1 ring-sage-100' : 'text-ink-400 hover:text-ink-600 hover:bg-sage-50/50'}`}
                >
                    好友 / 班级
                </button>
                <div className="w-[1px] h-4 bg-sage-200/50 mx-1"></div>
                <button
                    onClick={() => setActiveTab('groups')}
                    className={`flex-1 py-2 text-sm font-bold rounded-lg transition-all ${activeTab === 'groups' ? 'bg-white text-sage-600 shadow-sm ring-1 ring-sage-100' : 'text-ink-400 hover:text-ink-600 hover:bg-sage-50/50'}`}
                >
                    群聊
                </button>

                {/* Notification Bell */}
                <div className="ml-1 pl-1 border-l border-sage-200/50">
                    <button
                        onClick={() => setIsNotificationCenterOpen(true)}
                        className="p-2 hover:bg-white hover:text-sage-600 rounded-lg text-sage-400 transition-all relative group"
                        title="消息中心"
                    >
                        <Bell size={18} className="group-hover:scale-110 transition-transform" />
                        {notifications.length > 0 && notifications.some(n => n.is_read === 0) && (
                            <span className="absolute top-1.5 right-1.5 w-2.5 h-2.5 bg-clay-500 rounded-full border-2 border-white animate-pulse"></span>
                        )}
                    </button>
                </div>
            </div>

            {/* List Content */}
            <div className="flex-1 overflow-auto p-4 pt-0 custom-scrollbar">
                {loading && (
                    <div className="flex flex-col items-center justify-center py-12 text-sage-400 gap-3">
                        <div className="w-6 h-6 border-2 border-sage-200 border-t-sage-500 rounded-full animate-spin"></div>
                        <span className="text-sm font-bold">加载中...</span>
                    </div>
                )}

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
                        <TreeNode label="班级群" count={managedClassGroups.length + joinedClassGroups.length} icon={<Users size={18} />} defaultOpen={true}>
                            {managedClassGroups.length > 0 && (
                                <TreeNode label="我管理的" count={managedClassGroups.length} icon={<User size={16} className="text-sage-500" />} defaultOpen={true}>
                                    {managedClassGroups.slice(0, 50).map((g, idx) => (
                                        <LeafNode
                                            key={`mgr-c-${idx}`}
                                            label={g?.group_name || '未命名群组'}
                                            avatarUrl={g?.face_url}
                                            icon={<MessageCircle size={16} />}
                                            onDoubleClick={() => invoke('open_class_window', { groupclassId: g.group_id })}
                                        />
                                    ))}
                                </TreeNode>
                            )}
                            {joinedClassGroups.length > 0 && (
                                <TreeNode label="我加入的" count={joinedClassGroups.length} icon={<Users size={16} />} defaultOpen={true}>
                                    {joinedClassGroups.slice(0, 50).map((g, idx) => (
                                        <LeafNode
                                            key={`join-c-${idx}`}
                                            label={g?.group_name || '未命名群组'}
                                            avatarUrl={g?.face_url}
                                            icon={<MessageCircle size={16} />}
                                            onDoubleClick={() => invoke('open_class_window', { groupclassId: g.group_id })}
                                        />
                                    ))}
                                </TreeNode>
                            )}
                        </TreeNode>

                        <TreeNode label="普通群" count={managedNormalGroups.length + joinedNormalGroups.length} icon={<MessageCircle size={18} />} defaultOpen={true}>
                            {managedNormalGroups.length > 0 && (
                                <TreeNode label="我管理的" count={managedNormalGroups.length} icon={<User size={16} className="text-sage-500" />} defaultOpen={true}>
                                    {managedNormalGroups.slice(0, 50).map((g, idx) => (
                                        <LeafNode
                                            key={`mgr-n-${idx}`}
                                            label={g?.group_name || '未命名群组'}
                                            avatarUrl={g?.face_url}
                                            // onDoubleClick={() => setSelectedNormalGroup({ groupId: g.group_id, groupName: g.group_name || '群聊', isOwner: true })}
                                            onDoubleClick={() => openNormalChatWindow(g.group_id, g.group_name)}
                                        />
                                    ))}
                                </TreeNode>
                            )}
                            {joinedNormalGroups.length > 0 && (
                                <TreeNode label="我加入的" count={joinedNormalGroups.length} icon={<Users size={16} />} defaultOpen={true}>
                                    {joinedNormalGroups.slice(0, 50).map((g, idx) => (
                                        <LeafNode
                                            key={`join-n-${idx}`}
                                            label={g?.group_name || '未命名群组'}
                                            avatarUrl={g?.face_url}
                                            // onDoubleClick={() => setSelectedNormalGroup({ groupId: g.group_id, groupName: g.group_name || '群聊', isOwner: false })}
                                            onDoubleClick={() => openNormalChatWindow(g.group_id, g.group_name)}
                                        />
                                    ))}
                                </TreeNode>
                            )}
                        </TreeNode>
                    </div>
                )}

                {/* Debug Info */}
                {classes.length === 0 && friends.length === 0 && managedClassGroups.length === 0 && joinedClassGroups.length === 0 && managedNormalGroups.length === 0 && joinedNormalGroups.length === 0 && (
                    <div className="p-4 text-xs text-ink-300 border-t border-sage-50 mt-4 text-center">
                        状态: {debugMsg === 'UserInfo is null' ? '用户信息为空' : debugMsg} <br />
                        教师ID: {userInfo?.teacher_unique_id ? '已获取' : '未获取'} <br />
                        身份证号: {userInfo?.id_number ? '已获取' : '未获取'}
                    </div>
                )}
            </div>

            {/* Notification Modal */}
            <NotificationCenterModal
                isOpen={isNotificationCenterOpen}
                onClose={() => setIsNotificationCenterOpen(false)}
                notifications={notifications}
            />

            {/* Context Menu - Portaled to body to avoid transform issues */}
            {
                contextMenu.visible && createPortal(
                    <>
                        <div className="fixed inset-0 z-50 bg-transparent" onClick={() => setContextMenu(prev => ({ ...prev, visible: false }))} />
                        <div
                            className="fixed bg-white/95 backdrop-blur-xl shadow-[0_4px_20px_rgba(0,0,0,0.1)] rounded-xl py-1.5 z-50 border border-sage-100 min-w-[160px] animate-in fade-in zoom-in-95 duration-100 ring-1 ring-sage-50"
                            style={{ top: contextMenu.y, left: contextMenu.x }}
                        >
                            {contextMenu.type === 'class' && (
                                <button
                                    onClick={handleLeaveClass}
                                    className="w-full text-left px-3 py-2 text-sm text-clay-600 hover:bg-clay-50 flex items-center gap-2.5 transition-colors group font-bold"
                                >
                                    <LogOut size={16} className="text-clay-500 group-hover:text-clay-600" />
                                    <span className="font-bold">退出班级</span>
                                </button>
                            )}
                            {contextMenu.type === 'friend' && (
                                <button
                                    onClick={handleUnfriend}
                                    className="w-full text-left px-3 py-2 text-sm text-clay-600 hover:bg-clay-50 flex items-center gap-2.5 transition-colors group font-bold"
                                >
                                    <UserMinus size={16} className="text-clay-500 group-hover:text-clay-600" />
                                    <span className="font-bold">解除好友</span>
                                </button>
                            )}
                        </div>
                    </>,
                    document.body
                )
            }
        </div >
    );
};

export default ClassManagement;
