import { useState, useEffect, useRef } from 'react';
import { useParams } from 'react-router-dom';
import { Send, Image, Paperclip, Users, Settings, UserPlus, LogOut, Crown, Shield, Check, X, Loader2 } from 'lucide-react';
import { sendMessage, sendImageMessage, sendFileMessage, addMessageListener, getGroupMemberList, quitGroup, dismissGroup, addGroupMember, loginTIM, isSDKReady, getGroupProfile, getMessageList } from '../utils/tim';
import TencentCloudChat from '@tencentcloud/chat';
import { invoke } from '@tauri-apps/api/core';
import { emit, listen } from '@tauri-apps/api/event';

interface FriendInfo {
    teacher_unique_id: string;
    name: string;
    avatar?: string;
}

interface ChatMessage {
    id: string;
    text: string;
    senderName: string;
    senderAvatar: string;
    isMine: boolean;
    time: Date;
    type: 'text' | 'image' | 'file';
    imageUrl?: string;
    fileName?: string;
    fileUrl?: string;
}

interface GroupMember {
    userID: string;
    nick: string;
    avatar: string;
    role: string; // Owner, Admin, Member
}

const NormalGroupChatWindow = () => {
    const { groupId } = useParams();
    const [messages, setMessages] = useState<ChatMessage[]>([]);
    const [inputText, setInputText] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const [showGroupInfo, setShowGroupInfo] = useState(false);
    const [members, setMembers] = useState<GroupMember[]>([]);
    const [loadingMembers, setLoadingMembers] = useState(false);
    const [groupName, setGroupName] = useState('');
    const [isOwner, setIsOwner] = useState(false);
    const [currentUserId, setCurrentUserId] = useState('');

    // Add member dialog state
    const [showAddMember, setShowAddMember] = useState(false);
    const [friends, setFriends] = useState<FriendInfo[]>([]);
    const [selectedFriends, setSelectedFriends] = useState<string[]>([]);
    const [loadingFriends, setLoadingFriends] = useState(false);

    // Debug & Status
    const [initStatus, setInitStatus] = useState('正在初始化...');
    const [initError, setInitError] = useState('');

    const messagesEndRef = useRef<HTMLDivElement>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const imageInputRef = useRef<HTMLInputElement>(null);

    const initCalled = useRef(false);

    // Initialize: Login TIM and Load Data
    useEffect(() => {
        if (initCalled.current) return;
        initCalled.current = true;

        const init = async () => {
            setInitStatus('读取用户信息...');
            try {
                // Get user info from localStorage
                const storedUserId = localStorage.getItem('teacher_unique_id');
                const storedUserSig = localStorage.getItem('userSig');

                if (storedUserId) {
                    setCurrentUserId(storedUserId);
                    if (storedUserSig) {
                        setInitStatus('登录 IM 服务...');
                        // Always login, let tin.ts handle ready state checking
                        const success = await loginTIM(storedUserId, storedUserSig);
                        if (!success && !isSDKReady) {
                            setInitError('IM 登录失败');
                            return;
                        }
                    }
                } else {
                    setInitError('未找到用户信息');
                    return;
                }

                if (groupId) {
                    setInitStatus('加载群信息...');
                    // Ensure SDK is ready just in case
                    if (!isSDKReady) {
                        console.warn('SDK not ready after loginTIM, waiting...');
                        await new Promise(r => setTimeout(r, 500));
                    }

                    // Fetch group profile
                    try {
                        const res = await getGroupProfile(groupId);
                        if (res && res.group) {
                            setGroupName(res.group.name);
                            setIsOwner(res.group.ownerID === storedUserId);
                        }
                    } catch (e) {
                        console.error("Failed to get group profile", e);
                    }

                    setInitStatus('加载历史消息...');
                    loadMessageHistory();
                    setInitStatus('加载群成员...');
                    loadGroupMembers();
                    setInitStatus('加载好友...');
                    loadFriends();
                    setInitStatus(''); // Done
                }
            } catch (err: any) {
                console.error('Init failed:', err);
                setInitError(`初始化错误: ${err?.message || err}`);
                alert(`初始化错误: ${err?.message || err}`);
            }
        };
        init();

        // Cleanup: Do not explicitly logout to prevent racing with main window or other errors
        return () => {
            // connection dies with window
        };
    }, [groupId]);

    const parseMessage = (msg: any): ChatMessage | null => {
        const isMine = msg.from === currentUserId;
        // Try to find friend info for nickname usually if nick is empty or same as ID
        const friend = friends.find(f => f.teacher_unique_id === msg.from);
        const displayName = friend ? friend.name : (msg.nick && msg.nick !== msg.from ? msg.nick : (msg.from || '未知用户'));

        const baseMsg = {
            id: msg.ID || msg.clientSequence || `${Date.now()}-${Math.random()}`,
            senderName: displayName,
            senderAvatar: friend?.avatar || msg.avatar || '',
            isMine,
            time: new Date(msg.time * 1000 || Date.now()),
        };

        if (msg.type === TencentCloudChat.TYPES.MSG_TEXT) {
            return { ...baseMsg, type: 'text', text: msg.payload?.text || '' };
        } else if (msg.type === TencentCloudChat.TYPES.MSG_IMAGE) {
            return {
                ...baseMsg,
                type: 'image',
                text: '[图片]',
                imageUrl: msg.payload?.imageInfoArray?.[0]?.url || msg.payload?.imageInfoArray?.[1]?.url || ''
            };
        } else if (msg.type === TencentCloudChat.TYPES.MSG_FILE) {
            return {
                ...baseMsg,
                type: 'file',
                text: `[文件] ${msg.payload?.fileName || ''}`,
                fileName: msg.payload?.fileName || '文件',
                fileUrl: msg.payload?.fileUrl || ''
            };
        }
        return null;
    };

    // Scroll to bottom when messages change
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages]);

    // Listen for incoming messages
    useEffect(() => {
        if (!groupId) return;

        const removeListener = addMessageListener((event: any) => {
            const messageList = event.data;
            if (Array.isArray(messageList)) {
                const newMessages = messageList
                    .filter((msg: any) => msg.conversationID === `GROUP${groupId}`)
                    .map(parseMessage)
                    .filter((m: ChatMessage | null): m is ChatMessage => m !== null);

                if (newMessages.length > 0) {
                    setMessages(prev => {
                        // Dedup based on ID
                        const existingIds = new Set(prev.map(p => p.id));
                        const uniqueNew = newMessages.filter(m => !existingIds.has(m.id));
                        if (uniqueNew.length === 0) return prev;
                        return [...prev, ...uniqueNew];
                    });
                }
            }
        });

        // Use broadcast event listener for Kicked Out
        const unlistenKicked = listen('tim:event', (event: any) => {
            if (event.payload?.event === TencentCloudChat.EVENT.KICKED_OUT) {
                setInitError('您的账号已在其他地方登录（或主窗口互斥），请关闭此窗口并重试。');
            }
        });

        return () => {
            removeListener();
            unlistenKicked.then(f => f());
        };
    }, [groupId, currentUserId, friends]); // Added friends dependency as parseMessage uses it

    const loadMessageHistory = async () => {
        if (!groupId) return;
        setIsLoading(true);
        try {
            const messageList = await getMessageList(groupId);
            if (messageList) {
                const parsedMessages = messageList
                    .map(parseMessage)
                    .filter((m: ChatMessage | null): m is ChatMessage => m !== null);
                setMessages(parsedMessages);
            }
        } catch (error) {
            console.error('Failed to load message history:', error);
        } finally {
            setIsLoading(false);
        }
    };

    const loadGroupMembers = async () => {
        if (!groupId) return;
        setLoadingMembers(true);
        try {
            const memberList = await getGroupMemberList(groupId);
            setMembers(memberList.map((m: any) => ({
                userID: m.userID,
                nick: m.nick || m.userID,
                avatar: m.avatar || '',
                role: m.role || 'Member'
            })));
        } catch (error) {
            console.error('Failed to load group members:', error);
        } finally {
            setLoadingMembers(false);
        }
    };

    const loadFriends = async () => {
        const idCard = localStorage.getItem('id_number');
        const token = localStorage.getItem('token');
        if (!idCard) return;

        setLoadingFriends(true);
        try {
            const resText = await invoke<string>('get_user_friends', { idCard, token });
            const res = JSON.parse(resText);
            if (res.friends) {
                setFriends(res.friends.map((f: any) => ({
                    teacher_unique_id: f.teacher_info?.teacher_unique_id || f.teacher_unique_id,
                    name: f.user_details?.name || f.teacher_info?.name || f.name || '未知好友',
                    avatar: f.user_details?.avatar || f.avatar
                })));
            }
        } catch (e) {
            console.error('Failed to load friends:', e);
        } finally {
            setLoadingFriends(false);
        }
    };

    const handleSendText = async () => {
        if (!inputText.trim() || !groupId) return;
        const text = inputText.trim();
        setInputText('');

        try {
            const sentMsg = await sendMessage(groupId, text, 'GROUP');
            if (sentMsg) {
                const newMsg = parseMessage(sentMsg);
                if (newMsg) setMessages(prev => [...prev, newMsg]);
            }
        } catch (error) {
            console.error('Failed to send message:', error);
            alert('发送消息失败');
        }
    };

    const handleSendImage = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !groupId) return;

        try {
            const sentMsg = await sendImageMessage(groupId, file, 'GROUP');
            if (sentMsg) {
                const newMsg = parseMessage(sentMsg);
                if (newMsg) setMessages(prev => [...prev, newMsg]);
            }
        } catch (error) {
            console.error('Failed to send image:', error);
            alert('发送图片失败');
        }
        if (imageInputRef.current) imageInputRef.current.value = '';
    };

    const handleSendFile = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !groupId) return;

        try {
            const sentMsg = await sendFileMessage(groupId, file, 'GROUP');
            if (sentMsg) {
                const newMsg = parseMessage(sentMsg);
                if (newMsg) setMessages(prev => [...prev, newMsg]);
            }
        } catch (error) {
            console.error('Failed to send file:', error);
            alert('发送文件失败');
        }
        if (fileInputRef.current) fileInputRef.current.value = '';
    };

    const handleKeyDown = (e: React.KeyboardEvent) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleSendText();
        }
    };

    const handleToggleGroupInfo = () => {
        if (!showGroupInfo && members.length === 0) {
            loadGroupMembers();
        }
        setShowGroupInfo(!showGroupInfo);
    };

    const handleQuitGroup = async () => {
        if (!confirm(isOwner ? '确定要解散此群吗？此操作不可恢复。' : '确定要退出此群吗？')) return;
        if (!groupId) return;

        try {
            if (isOwner) {
                await dismissGroup(groupId);
                alert('群已解散');
            } else {
                await quitGroup(groupId);
                alert('已退出群聊');
            }
            await emit('refresh-class-list');
            window.close();
        } catch (error) {
            console.error('Failed to quit/dismiss group:', error);
            alert(isOwner ? '解散群失败' : '退出群失败');
        }
    };

    const handleAddMember = async () => {
        if (selectedFriends.length === 0 || !groupId) return;
        try {
            await addGroupMember(groupId, selectedFriends);
            alert(`已邀请 ${selectedFriends.length} 位好友`);
            setSelectedFriends([]);
            setShowAddMember(false);
            loadGroupMembers(); // Refresh member list
        } catch (error: any) {
            console.error('Failed to add member:', error);
            alert(`添加成员失败: ${error?.message || '未知错误'}`);
        }
    };

    const toggleFriendSelection = (friendId: string) => {
        setSelectedFriends(prev =>
            prev.includes(friendId)
                ? prev.filter(id => id !== friendId)
                : [...prev, friendId]
        );
    };

    const getRoleIcon = (role: string) => {
        if (role === 'Owner') return <Crown size={14} className="text-amber-500" />;
        if (role === 'Admin') return <Shield size={14} className="text-blue-500" />;
        return null;
    };

    // Helper to open Add Member Dialog and load friends if needed
    const openAddMemberDialog = () => {
        setShowAddMember(true);
        if (friends.length === 0) {
            loadFriends();
        }
    };

    // Filter available friends
    const availableFriends = friends.filter(
        f => !members.some(m => m.userID === f.teacher_unique_id)
    );

    return (
        <div className="h-screen w-screen flex flex-col bg-paper overflow-hidden relative font-sans selection:bg-indigo-100 selection:text-indigo-700">
            {/* Init Status Overlay */}
            {initStatus && !initError && (
                <div className="absolute inset-0 z-50 flex items-center justify-center bg-white/60 backdrop-blur-sm">
                    <div className="text-center bg-white/80 p-6 rounded-2xl shadow-xl border border-white/50 backdrop-blur-md">
                        <div className="inline-block w-8 h-8 border-4 border-indigo-100 border-t-indigo-500 rounded-full animate-spin mb-4"></div>
                        <p className="text-indigo-600 font-bold tracking-wide">{initStatus}</p>
                    </div>
                </div>
            )}
            {/* Error Overlay */}
            {initError && (
                <div className="absolute inset-0 z-50 flex items-center justify-center bg-white/80 backdrop-blur-md">
                    <div className="text-center p-8 max-w-sm bg-paper rounded-[2rem] shadow-2xl border border-white/60 ring-1 ring-sage-100/50">
                        <div className="w-16 h-16 bg-red-50 text-red-500 rounded-2xl flex items-center justify-center mx-auto mb-4 border border-red-100 shadow-sm">
                            <X size={28} />
                        </div>
                        <h3 className="text-lg font-bold text-ink-800 mb-2">出错了</h3>
                        <p className="text-sage-500 mb-6 font-medium">{initError}</p>
                        <div className="flex gap-3 justify-center">
                            <button
                                onClick={() => window.location.reload()}
                                className="px-6 py-2.5 bg-indigo-500 text-white rounded-xl hover:bg-indigo-600 transition-all font-bold shadow-lg shadow-indigo-500/20 hover:-translate-y-0.5"
                            >
                                重试
                            </button>
                            <button
                                onClick={async () => {
                                    const { getCurrentWebviewWindow } = await import('@tauri-apps/api/webviewWindow');
                                    await getCurrentWebviewWindow().close();
                                }}
                                className="px-6 py-2.5 bg-white text-sage-600 border border-sage-200 rounded-xl hover:bg-sage-50 hover:text-sage-800 transition-all font-bold shadow-sm"
                            >
                                关闭
                            </button>
                        </div>
                    </div>
                </div>
            )}

            {/* Header - Glass Style */}
            <div data-tauri-drag-region className="flex items-center justify-between px-6 py-4 border-b border-sage-100/50 bg-white/60 backdrop-blur-md select-none sticky top-0 z-10 transition-colors">
                <div className="flex items-center gap-4 pointer-events-none">
                    <div className="w-10 h-10 rounded-2xl bg-gradient-to-br from-indigo-400 to-indigo-600 flex items-center justify-center text-white shadow-lg shadow-indigo-500/20">
                        <Users size={20} />
                    </div>
                    <div>
                        <h2 className="text-lg font-bold text-ink-800 tracking-tight">{groupName || '群聊'}</h2>
                        <p className="text-xs font-medium text-sage-500 flex items-center gap-1.5">
                            <span className={`w-1.5 h-1.5 rounded-full ${members.length > 0 ? 'bg-emerald-400' : 'bg-sage-300'}`}></span>
                            {members.length > 0 ? `${members.length} 位成员` : '加载中...'}
                        </p>
                    </div>
                </div>
                <div className="flex items-center gap-2 no-drag">
                    <button
                        onClick={handleToggleGroupInfo}
                        className={`w-9 h-9 flex items-center justify-center rounded-xl transition-all ${showGroupInfo
                            ? 'bg-indigo-100 text-indigo-600'
                            : 'text-sage-400 hover:text-indigo-500 hover:bg-indigo-50'
                            }`}
                        title="群信息"
                    >
                        <Settings size={18} />
                    </button>
                    <button
                        onClick={async () => {
                            const { getCurrentWebviewWindow } = await import('@tauri-apps/api/webviewWindow');
                            await getCurrentWebviewWindow().close();
                        }}
                        className="w-9 h-9 flex items-center justify-center text-sage-400 hover:text-clay-600 hover:bg-clay-50 rounded-xl transition-all"
                        title="关闭"
                    >
                        <X size={18} />
                    </button>
                </div>
            </div>

            <div className="flex-1 flex overflow-hidden relative">
                {/* Main Chat Area */}
                <div className="flex-1 flex flex-col min-w-0 bg-white/30">
                    {/* Messages Area */}
                    <div className="flex-1 overflow-y-auto p-6 space-y-4 custom-scrollbar">
                        {isLoading && (
                            <div className="text-center text-sage-400 py-6">
                                <div className="inline-block w-5 h-5 border-2 border-sage-300 border-t-indigo-500 rounded-full animate-spin mb-2"></div>
                                <p className="text-sm font-medium">加载消息中...</p>
                            </div>
                        )}
                        {!isLoading && messages.length === 0 && (
                            <div className="text-center text-sage-400 py-10 flex flex-col items-center gap-3">
                                <div className="w-16 h-16 bg-sage-50 rounded-2xl flex items-center justify-center">
                                    <Users size={32} className="text-sage-300" />
                                </div>
                                <p className="text-sm font-medium">暂无消息，发送第一条消息开始聊天吧！</p>
                            </div>
                        )}
                        {messages.map((msg) => (
                            <div key={msg.id} className={`flex ${msg.isMine ? 'justify-end' : 'justify-start'} group animate-in slide-in-from-bottom-2 duration-300`}>
                                <div className={`flex gap-3 max-w-[80%] ${msg.isMine ? 'flex-row-reverse' : ''}`}>
                                    <div className="flex-shrink-0 mt-1">
                                        {msg.senderAvatar ? (
                                            <img src={msg.senderAvatar} alt="" className="w-9 h-9 rounded-xl object-cover shadow-sm ring-2 ring-white" />
                                        ) : (
                                            <div className={`w-9 h-9 rounded-xl flex items-center justify-center text-white text-sm font-bold shadow-sm ring-2 ring-white ${msg.isMine ? 'bg-gradient-to-br from-indigo-400 to-indigo-600' : 'bg-gradient-to-br from-sage-400 to-sage-500'}`}>
                                                {msg.senderName.charAt(0)}
                                            </div>
                                        )}
                                    </div>
                                    <div className={`flex flex-col ${msg.isMine ? 'items-end' : 'items-start'}`}>
                                        {!msg.isMine && <span className="text-xs font-bold text-sage-400 mb-1 ml-1">{msg.senderName}</span>}
                                        <div className={`rounded-2xl px-5 py-3 shadow-sm transition-all hover:shadow-md ${msg.isMine
                                            ? 'bg-gradient-to-br from-indigo-500 to-violet-600 text-white rounded-tr-sm shadow-indigo-200'
                                            : 'bg-white text-ink-800 rounded-tl-sm border border-white/50'
                                            }`}>
                                            {msg.type === 'image' && msg.imageUrl ? (
                                                <img src={msg.imageUrl} alt="图片" className="max-w-[280px] rounded-lg cursor-pointer hover:opacity-95 transition-opacity" style={{ maxHeight: 240 }} />
                                            ) : msg.type === 'file' ? (
                                                <a href={msg.fileUrl} target="_blank" rel="noopener noreferrer" className={`flex items-center gap-3 py-1 ${msg.isMine ? 'text-white' : 'text-indigo-600'}`}>
                                                    <div className={`p-2 rounded-lg ${msg.isMine ? 'bg-white/20' : 'bg-indigo-50'}`}>
                                                        <Paperclip size={18} />
                                                    </div>
                                                    <div className="flex flex-col min-w-0">
                                                        <span className="font-bold text-sm truncate max-w-[200px]">{msg.fileName}</span>
                                                        <span className="text-xs opacity-80">点击下载</span>
                                                    </div>
                                                </a>
                                            ) : (
                                                <p className="whitespace-pre-wrap break-words text-sm leading-relaxed font-medium">{msg.text}</p>
                                            )}
                                        </div>
                                        <span className={`text-[10px] font-medium mt-1 mx-1 ${msg.isMine ? 'text-indigo-300' : 'text-sage-300'}`}>
                                            {msg.time.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })}
                                        </span>
                                    </div>
                                </div>
                            </div>
                        ))}
                        <div ref={messagesEndRef} />
                    </div>

                    {/* Input Area */}
                    <div className="border-t border-sage-100/50 p-4 bg-white/60 backdrop-blur-md">
                        <div className="flex items-center gap-3">
                            <input type="file" ref={imageInputRef} onChange={handleSendImage} accept="image/*" className="hidden" />
                            <button onClick={() => imageInputRef.current?.click()} className="p-2.5 text-sage-400 hover:text-violet-600 hover:bg-violet-50 rounded-xl transition-all" title="发送图片">
                                <Image size={20} />
                            </button>
                            <input type="file" ref={fileInputRef} onChange={handleSendFile} className="hidden" />
                            <button onClick={() => fileInputRef.current?.click()} className="p-2.5 text-sage-400 hover:text-violet-600 hover:bg-violet-50 rounded-xl transition-all" title="发送文件">
                                <Paperclip size={20} />
                            </button>
                            <div className="flex-1 relative">
                                <input
                                    type="text"
                                    value={inputText}
                                    onChange={(e) => setInputText(e.target.value)}
                                    onKeyDown={handleKeyDown}
                                    placeholder="输入消息..."
                                    className="w-full px-5 py-3 bg-white/80 border border-sage-200 rounded-2xl text-sm text-ink-800 font-medium focus:outline-none focus:border-indigo-400 focus:ring-4 focus:ring-indigo-100 transition-all placeholder:text-sage-400 shadow-inner"
                                />
                            </div>
                            <button
                                onClick={handleSendText}
                                disabled={!inputText.trim()}
                                className="p-3 bg-gradient-to-r from-indigo-500 to-violet-600 text-white rounded-xl hover:from-indigo-600 hover:to-violet-700 disabled:opacity-50 disabled:cursor-not-allowed transition-all shadow-lg shadow-indigo-500/20 hover:-translate-y-0.5"
                            >
                                <Send size={20} />
                            </button>
                        </div>
                    </div>
                </div>

                {/* Group Info Panel */}
                {showGroupInfo && (
                    <div className="w-[280px] border-l border-sage-100/50 bg-sage-50/80 backdrop-blur-md flex flex-col z-20 shadow-[-5px_0_15px_rgba(0,0,0,0.02)]">
                        <div className="p-5 border-b border-sage-100/50 bg-white/40">
                            <h3 className="font-bold text-ink-800 text-lg">群信息</h3>
                            <p className="text-xs font-mono text-sage-400 mt-1 bg-white/50 px-2 py-1 rounded-lg inline-block border border-sage-100">{groupId}</p>
                        </div>

                        {/* Members List */}
                        <div className="flex-1 overflow-y-auto custom-scrollbar">
                            <div className="p-4">
                                <div className="flex items-center justify-between mb-4">
                                    <span className="text-xs font-bold text-sage-500 uppercase tracking-wider">成员 ({members.length})</span>
                                    {isOwner && (
                                        <button
                                            onClick={openAddMemberDialog}
                                            className="p-1.5 text-indigo-500 hover:bg-indigo-50 rounded-lg transition-colors bg-white shadow-sm border border-sage-100"
                                            title="邀请成员"
                                        >
                                            <UserPlus size={16} />
                                        </button>
                                    )}
                                </div>

                                {loadingMembers ? (
                                    <div className="text-center py-6 text-sage-400 text-sm font-medium">
                                        <div className="w-5 h-5 border-2 border-sage-300 border-t-indigo-500 rounded-full animate-spin mx-auto mb-2"></div>
                                        加载中...
                                    </div>
                                ) : (
                                    <div className="space-y-2">
                                        {members.map((member) => {
                                            const friend = friends.find(f => f.teacher_unique_id === member.userID);
                                            const displayName = friend ? friend.name : (member.nick && member.nick !== member.userID ? member.nick : member.userID);

                                            return (
                                                <div key={member.userID} className="flex items-center gap-3 p-2.5 rounded-xl hover:bg-white/80 border border-transparent hover:border-sage-100 transition-all group shadow-sm hover:shadow-md cursor-default bg-white/40">
                                                    {member.avatar ? (
                                                        <img src={member.avatar} alt="" className="w-9 h-9 rounded-xl object-cover shadow-sm" />
                                                    ) : (
                                                        <div className="w-9 h-9 rounded-xl bg-gradient-to-br from-indigo-100 to-violet-100 text-indigo-600 border border-indigo-200 flex items-center justify-center text-sm font-bold">
                                                            {displayName.charAt(0)}
                                                        </div>
                                                    )}
                                                    <div className="flex-1 min-w-0">
                                                        <div className="flex items-center gap-1.5">
                                                            <span className="text-sm font-bold text-ink-700 truncate">{displayName}</span>
                                                            {getRoleIcon(member.role)}
                                                        </div>
                                                        {member.userID === currentUserId && (
                                                            <span className="text-[10px] font-bold text-white bg-indigo-400 px-1.5 py-0.5 rounded-md mt-0.5 inline-block">我</span>
                                                        )}
                                                    </div>
                                                </div>
                                            )
                                        })}
                                    </div>
                                )}
                            </div>
                        </div>

                        {/* Actions */}
                        <div className="p-5 border-t border-sage-100/50 bg-white/40">
                            <button
                                onClick={handleQuitGroup}
                                className="w-full flex items-center justify-center gap-2 px-4 py-3 text-clay-500 hover:bg-clay-50 hover:text-clay-600 rounded-xl transition-all font-bold text-sm border border-clay-200 hover:border-clay-300 shadow-sm hover:shadow-md"
                            >
                                <LogOut size={16} />
                                {isOwner ? '解散群聊' : '退出群聊'}
                            </button>
                        </div>
                    </div>
                )}
            </div>

            {/* Add Member Dialog (Modal inside Window) */}
            {showAddMember && (
                <div className="fixed inset-0 z-[10000] flex items-center justify-center bg-black/40 backdrop-blur-sm pointer-events-auto font-sans">
                    <div className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[400px] max-h-[550px] flex flex-col animate-in zoom-in-95 border border-white/60 ring-1 ring-sage-100/50">
                        <div className="p-5 border-b border-sage-100/50 flex items-center justify-between bg-white/40">
                            <div>
                                <h3 className="text-lg font-bold text-ink-800">邀请好友</h3>
                                <p className="text-xs font-medium text-sage-500 mt-1">已选择 {selectedFriends.length} 位好友</p>
                            </div>
                            <button
                                onClick={() => { setShowAddMember(false); setSelectedFriends([]); }}
                                className="w-8 h-8 flex items-center justify-center text-sage-400 hover:bg-clay-50 hover:text-clay-600 rounded-full transition-colors"
                            >
                                <X size={20} />
                            </button>
                        </div>

                        <div className="flex-1 overflow-y-auto p-4 max-h-[350px] bg-white/30 custom-scrollbar">
                            {loadingFriends ? (
                                <div className="text-center py-8 text-sage-400 flex flex-col items-center">
                                    <Loader2 className="animate-spin mb-2" />
                                    <p className="font-medium text-sm">加载中...</p>
                                </div>
                            ) : availableFriends.length === 0 ? (
                                <div className="text-center py-10 text-sage-400">
                                    <Users size={40} className="mx-auto mb-3 text-sage-300" />
                                    <p className="font-medium">没有可邀请的好友</p>
                                </div>
                            ) : (
                                <div className="space-y-2">
                                    {availableFriends.map(friend => {
                                        const isSelected = selectedFriends.includes(friend.teacher_unique_id);
                                        return (
                                            <div
                                                key={friend.teacher_unique_id}
                                                onClick={() => toggleFriendSelection(friend.teacher_unique_id)}
                                                className={`flex items-center gap-3 p-3 rounded-xl cursor-pointer transition-all border ${isSelected
                                                    ? 'bg-indigo-50 border-indigo-200 shadow-indigo-100 shadow-inner'
                                                    : 'bg-white/60 border-transparent hover:border-sage-200 hover:bg-white/90 shadow-sm'
                                                    }`}
                                            >
                                                {friend.avatar ? (
                                                    <img src={friend.avatar} alt="" className="w-10 h-10 rounded-xl object-cover shadow-sm" />
                                                ) : (
                                                    <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-indigo-100 to-violet-100 border border-indigo-200 text-indigo-600 flex items-center justify-center text-sm font-bold">
                                                        {friend.name.charAt(0)}
                                                    </div>
                                                )}
                                                <span className="flex-1 text-sm text-ink-700 font-bold">{friend.name}</span>
                                                <div className={`w-6 h-6 rounded-full border-2 flex items-center justify-center transition-all ${isSelected
                                                    ? 'bg-indigo-500 border-indigo-500 scale-110'
                                                    : 'border-sage-300 bg-white'
                                                    }`}>
                                                    {isSelected && <Check size={14} className="text-white" strokeWidth={3} />}
                                                </div>
                                            </div>
                                        );
                                    })}
                                </div>
                            )}
                        </div>

                        <div className="p-5 border-t border-sage-100/50 flex gap-3 bg-white/40">
                            <button
                                onClick={() => { setShowAddMember(false); setSelectedFriends([]); }}
                                className="flex-1 px-4 py-2.5 text-sage-600 bg-white hover:bg-sage-50 border border-sage-200 hover:border-sage-300 rounded-xl transition-all font-bold text-sm shadow-sm"
                            >
                                取消
                            </button>
                            <button
                                onClick={handleAddMember}
                                disabled={selectedFriends.length === 0}
                                className="flex-1 px-4 py-2.5 bg-indigo-500 text-white rounded-xl hover:bg-indigo-600 disabled:opacity-50 disabled:cursor-not-allowed transition-all font-bold text-sm shadow-lg shadow-indigo-500/20"
                            >
                                邀请 ({selectedFriends.length})
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default NormalGroupChatWindow;
