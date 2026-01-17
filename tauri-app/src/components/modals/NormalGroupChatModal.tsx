import { useState, useEffect, useRef, useCallback } from 'react';
import { createPortal } from 'react-dom';
import { X, Send, Image, Paperclip, Users, Settings, UserPlus, LogOut, Crown, Shield, Check } from 'lucide-react';
import { sendMessage, sendImageMessage, sendFileMessage, addMessageListener, getGroupMemberList, quitGroup, dismissGroup, addGroupMember, getMessageList } from '../../utils/tim';
import TencentCloudChat from '@tencentcloud/chat';

interface NormalGroupChatModalProps {
    isOpen: boolean;
    onClose: () => void;
    groupId: string;
    groupName: string;
    isOwner: boolean;
    userInfo?: any;
    friends?: FriendInfo[];
}

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

const NormalGroupChatModal = ({ isOpen, onClose, groupId, groupName, isOwner, userInfo, friends = [] }: NormalGroupChatModalProps) => {
    const [messages, setMessages] = useState<ChatMessage[]>([]);
    const [inputText, setInputText] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const [showGroupInfo, setShowGroupInfo] = useState(false);
    const [members, setMembers] = useState<GroupMember[]>([]);
    const [loadingMembers, setLoadingMembers] = useState(false);

    // Add member dialog state
    const [showAddMember, setShowAddMember] = useState(false);
    const [selectedFriends, setSelectedFriends] = useState<string[]>([]);

    // Draggable state
    const [position, setPosition] = useState({ x: 0, y: 0 });
    const [isDragging, setIsDragging] = useState(false);
    const dragOffset = useRef({ x: 0, y: 0 });
    const modalRef = useRef<HTMLDivElement>(null);

    const messagesEndRef = useRef<HTMLDivElement>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const imageInputRef = useRef<HTMLInputElement>(null);

    // Get current user ID
    const currentUserId = userInfo?.teacher_unique_id || userInfo?.id || localStorage.getItem('unique_id') || '';

    // Center modal on open
    useEffect(() => {
        if (isOpen) {
            setPosition({ x: 0, y: 0 });
        }
    }, [isOpen]);

    // Scroll to bottom when messages change
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages]);

    // Load message history when modal opens
    useEffect(() => {
        if (isOpen && groupId) {
            loadMessageHistory();
        }
    }, [isOpen, groupId]);

    // Listen for incoming messages
    useEffect(() => {
        if (!isOpen || !groupId) return;

        const removeListener = addMessageListener((event: any) => {
            const messageList = event.data || [];
            messageList.forEach((msg: any) => {
                if (msg.conversationID === `GROUP${groupId}`) {
                    const newMsg = parseMessage(msg);
                    if (newMsg) {
                        setMessages(prev => {
                            if (prev.some(m => m.id === newMsg.id)) return prev;
                            return [...prev, newMsg];
                        });
                    }
                }
            });
        });

        return () => removeListener();
    }, [isOpen, groupId, currentUserId]);

    // Drag handlers
    const handleMouseDown = useCallback((e: React.MouseEvent) => {
        if ((e.target as HTMLElement).closest('.no-drag')) return;
        setIsDragging(true);
        const rect = modalRef.current?.getBoundingClientRect();
        if (rect) {
            dragOffset.current = {
                x: e.clientX - rect.left - rect.width / 2,
                y: e.clientY - rect.top - rect.height / 2
            };
        }
    }, []);

    useEffect(() => {
        if (!isDragging) return;

        const handleMouseMove = (e: MouseEvent) => {
            const newX = e.clientX - window.innerWidth / 2 - dragOffset.current.x;
            const newY = e.clientY - window.innerHeight / 2 - dragOffset.current.y;
            setPosition({ x: newX, y: newY });
        };

        const handleMouseUp = () => setIsDragging(false);

        document.addEventListener('mousemove', handleMouseMove);
        document.addEventListener('mouseup', handleMouseUp);
        return () => {
            document.removeEventListener('mousemove', handleMouseMove);
            document.removeEventListener('mouseup', handleMouseUp);
        };
    }, [isDragging]);

    const parseMessage = (msg: any): ChatMessage | null => {
        const isMine = msg.from === currentUserId;
        const baseMsg = {
            id: msg.ID || msg.clientSequence || `${Date.now()}-${Math.random()}`,
            senderName: msg.nick || msg.from || '未知用户',
            senderAvatar: msg.avatar || '',
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

    const loadMessageHistory = async () => {
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

        try {
            if (isOwner) {
                await dismissGroup(groupId);
                alert('群已解散');
            } else {
                await quitGroup(groupId);
                alert('已退出群聊');
            }
            onClose();
        } catch (error) {
            console.error('Failed to quit/dismiss group:', error);
            alert(isOwner ? '解散群失败' : '退出群失败');
        }
    };

    const handleAddMember = async () => {
        if (selectedFriends.length === 0) return;
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

    // Filter out friends who are already members
    const availableFriends = friends.filter(
        f => !members.some(m => m.userID === f.teacher_unique_id)
    );

    const getRoleIcon = (role: string) => {
        if (role === 'Owner') return <Crown size={14} className="text-amber-500" />;
        if (role === 'Admin') return <Shield size={14} className="text-blue-500" />;
        return null;
    };

    if (!isOpen) return null;

    return createPortal(
        <div className="fixed inset-0 z-[9999] pointer-events-none font-sans">
            <div
                ref={modalRef}
                style={{
                    position: 'fixed',
                    left: `calc(50% + ${position.x}px)`,
                    top: `calc(50% + ${position.y}px)`,
                    transform: 'translate(-50%, -50%)'
                }}
                className={`bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl flex overflow-hidden pointer-events-auto border border-white/60 ring-1 ring-sage-100/50 ${isDragging ? 'shadow-3xl cursor-grabbing scale-[1.01]' : 'transition-transform duration-200'}`}
            >
                {/* Main Chat Area */}
                <div className="w-[600px] h-[700px] flex flex-col">
                    {/* Header - Draggable */}
                    <div
                        onMouseDown={handleMouseDown}
                        className="flex items-center justify-between px-6 py-4 border-b border-sage-100/50 bg-white/40 backdrop-blur-md cursor-grab active:cursor-grabbing select-none"
                    >
                        <div className="flex items-center gap-4">
                            <div className="w-12 h-12 rounded-2xl bg-gradient-to-br from-indigo-400 to-indigo-600 flex items-center justify-center text-white shadow-lg shadow-indigo-500/20">
                                <Users size={24} />
                            </div>
                            <div>
                                <h2 className="text-lg font-bold text-ink-800 tracking-tight">{groupName || '群聊'}</h2>
                                <p className="text-xs font-medium text-sage-500 flex items-center gap-1">
                                    <span className={`w-1.5 h-1.5 rounded-full ${members.length > 0 ? 'bg-emerald-400' : 'bg-sage-300'}`}></span>
                                    {members.length > 0 ? `${members.length} 位成员` : (isOwner ? '群主' : '成员')}
                                </p>
                            </div>
                        </div>
                        <div className="flex items-center gap-2 no-drag">
                            <button
                                onClick={handleToggleGroupInfo}
                                className={`w-10 h-10 rounded-full flex items-center justify-center transition-all ${showGroupInfo
                                    ? 'bg-indigo-100 text-indigo-600'
                                    : 'text-sage-400 hover:text-indigo-500 hover:bg-indigo-50'
                                    }`}
                                title="群信息"
                            >
                                <Settings size={20} />
                            </button>
                            <button
                                onClick={onClose}
                                className="w-10 h-10 rounded-full flex items-center justify-center text-sage-400 hover:text-clay-600 hover:bg-clay-50 transition-all"
                                title="关闭"
                            >
                                <X size={20} />
                            </button>
                        </div>
                    </div>

                    {/* Messages Area */}
                    <div className="flex-1 overflow-y-auto px-6 py-4 space-y-4 bg-white/30 custom-scrollbar">
                        {isLoading && (
                            <div className="text-center text-sage-400 py-6">
                                <div className="inline-block w-6 h-6 border-3 border-sage-200 border-t-indigo-500 rounded-full animate-spin mb-2"></div>
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
                                            <img src={msg.senderAvatar} alt="" className="w-10 h-10 rounded-xl object-cover shadow-sm ring-2 ring-white" />
                                        ) : (
                                            <div className={`w-10 h-10 rounded-xl flex items-center justify-center text-white text-sm font-bold shadow-sm ring-2 ring-white ${msg.isMine ? 'bg-gradient-to-br from-indigo-400 to-indigo-600' : 'bg-gradient-to-br from-sage-400 to-sage-500'}`}>
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
                                                        <Paperclip size={20} />
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
                            <button onClick={() => imageInputRef.current?.click()} className="p-3 text-sage-400 hover:text-violet-600 hover:bg-violet-50 rounded-xl transition-all" title="发送图片">
                                <Image size={22} />
                            </button>
                            <input type="file" ref={fileInputRef} onChange={handleSendFile} className="hidden" />
                            <button onClick={() => fileInputRef.current?.click()} className="p-3 text-sage-400 hover:text-violet-600 hover:bg-violet-50 rounded-xl transition-all" title="发送文件">
                                <Paperclip size={22} />
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
                                className="p-3 bg-gradient-to-r from-indigo-500 to-violet-600 text-white rounded-xl hover:from-indigo-600 hover:to-violet-700 disabled:opacity-50 disabled:cursor-not-allowed transition-all shadow-lg shadow-indigo-500/20 hover:-translate-y-0.5 hover:shadow-indigo-500/30 active:scale-95 disabled:shadow-none"
                            >
                                <Send size={22} />
                            </button>
                        </div>
                    </div>
                </div>

                {/* Group Info Panel */}
                {showGroupInfo && (
                    <div className="w-[280px] h-[700px] border-l border-sage-100/50 bg-sage-50/50 backdrop-blur-md flex flex-col">
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
                                            onClick={() => setShowAddMember(true)}
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
                                        {members.map((member) => (
                                            <div key={member.userID} className="flex items-center gap-3 p-2.5 rounded-xl hover:bg-white/80 border border-transparent hover:border-sage-100 transition-all group shadow-sm hover:shadow-md cursor-default bg-white/40">
                                                {member.avatar ? (
                                                    <img src={member.avatar} alt="" className="w-9 h-9 rounded-xl object-cover" />
                                                ) : (
                                                    <div className="w-9 h-9 rounded-xl bg-gradient-to-br from-indigo-100 to-violet-100 text-indigo-600 border border-indigo-200 flex items-center justify-center text-sm font-bold">
                                                        {member.nick.charAt(0)}
                                                    </div>
                                                )}
                                                <div className="flex-1 min-w-0">
                                                    <div className="flex items-center gap-1.5">
                                                        <span className="text-sm font-bold text-ink-700 truncate">{member.nick}</span>
                                                        {getRoleIcon(member.role)}
                                                    </div>
                                                    {member.userID === currentUserId && (
                                                        <span className="text-[10px] font-bold text-white bg-indigo-400 px-1.5 py-0.5 rounded-md mt-0.5 inline-block">我</span>
                                                    )}
                                                </div>
                                            </div>
                                        ))}
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
                                <LogOut size={18} />
                                {isOwner ? '解散群聊' : '退出群聊'}
                            </button>
                        </div>
                    </div>
                )}
            </div>

            {/* Add Member Dialog */}
            {showAddMember && (
                <div className="fixed inset-0 z-[10000] flex items-center justify-center bg-black/40 backdrop-blur-sm pointer-events-auto font-sans">
                    <div className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[400px] max-h-[550px] flex flex-col animate-in zoom-in-95 border border-white/60">
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
                            {availableFriends.length === 0 ? (
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
                                className="flex-1 px-4 py-2.5 bg-indigo-500 text-white rounded-xl hover:bg-indigo-600 disabled:opacity-50 disabled:cursor-not-allowed transition-all font-bold text-sm shadow-lg shadow-indigo-500/20 hover:-translate-y-0.5"
                            >
                                邀请 ({selectedFriends.length})
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>,
        document.body
    );
};

export default NormalGroupChatModal;
