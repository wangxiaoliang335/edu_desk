import { useEffect, useState } from 'react';
import { useParams } from 'react-router-dom';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { Minus, X, Square, Copy, MessageCircle, MoreHorizontal, Image, Paperclip, Smile, Clock, LogOut, Trash2, UserCheck } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { sendMessage, sendImageMessage, sendFileMessage, getMessageList, loginTIM, isSDKReady, getGroupMemberList, dismissGroup, quitGroup, changeGroupOwner } from '../utils/tim';
import CustomListModal from './modals/CustomListModal';
import { useRef } from 'react';

const ClassChatWindow = () => {
    const { groupclassId } = useParams();
    const [isMaximized, setIsMaximized] = useState(false);
    const [messages, setMessages] = useState<any[]>([]);
    const [inputText, setInputText] = useState('');
    const [isReady, setIsReady] = useState(false);

    const [members, setMembers] = useState<any[]>([]);
    const [showCustomList, setShowCustomList] = useState(false);
    const [showGroupMenu, setShowGroupMenu] = useState(false);
    const [isOwner, setIsOwner] = useState(false);
    const [showTransferModal, setShowTransferModal] = useState(false);

    const imageInputRef = useRef<HTMLInputElement>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const groupMenuRef = useRef<HTMLDivElement>(null);

    // ... (useEffect for window and initChat remains same, skipping lines for brevity)

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

        // Initialize TIM if needed
        const initChat = async () => {
            if (!isSDKReady) {
                const teacherId = localStorage.getItem('teacher_unique_id');
                // Ensure Login stores userSig properly, fallback to 'token' might fail if it's not the signature
                const userSig = localStorage.getItem('userSig') || localStorage.getItem('token');

                if (teacherId && userSig) {
                    console.log('ClassChatWindow: Attempting TIM login with', teacherId);
                    await loginTIM(teacherId, userSig);
                } else {
                    console.error('ClassChatWindow: Missing credentials for TIM login');
                }
            }

            // Check readiness in a loop/timeout
            const interval = setInterval(() => {
                if (isSDKReady) {
                    console.log('ClassChatWindow: TIM SDK Ready');
                    setIsReady(true);
                    loadMessages();
                    loadMembers(); // Fetch members
                    clearInterval(interval);
                }
            }, 500);
            setTimeout(() => clearInterval(interval), 10000);
        };
        initChat();

        return () => {
            unlisten.then(f => f());
        }
    }, [groupclassId]);

    const loadMessages = async () => {
        if (!groupclassId) return;
        const list = await getMessageList(groupclassId);
        setMessages(list);
    };

    const loadMembers = async () => {
        if (!groupclassId) {
            console.error("[ClassChatWindow] 未提供 groupclassId");
            return;
        }

        console.log(`[ClassChatWindow] 开始加载成员: ${groupclassId}`);

        const currentUserId = localStorage.getItem('teacher_unique_id') || '';

        // Prepare Token
        let token = localStorage.getItem('token') || "";
        try {
            if (!token) {
                const userInfoStr = localStorage.getItem('user_info');
                if (userInfoStr) {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                }
            }
        } catch (e) {
            console.error("Token parse error", e);
        }

        if (!token) console.warn("[ClassChatWindow] 未找到用于后端请求的 token");

        // 1. Start Backend Fetch (Primary)
        const backendPromise = (async (): Promise<{ members: any[]; groupInfo: any }> => {
            if (!token) return { members: [], groupInfo: null };
            try {
                console.log("[ClassChatWindow] 正在调用 get_group_members...");
                const resStr = await invoke<string>('get_group_members', {
                    groupId: groupclassId,
                    token
                });
                console.log("[ClassChatWindow] get_group_members 原始响应:", resStr);
                const res = JSON.parse(resStr);
                if (res.code === 200 || (res.data && res.data.code === 200)) {
                    return {
                        members: res.data?.members || res.data || [],
                        groupInfo: res.data?.group_info
                    };
                }
            } catch (e) {
                console.error("[ClassChatWindow] 后端拉取失败", e);
            }
            return { members: [], groupInfo: null };
        })();

        // 2. Start TIM Fetch
        const timPromise = (async () => {
            if (isSDKReady) {
                try {
                    return await getGroupMemberList(groupclassId);
                } catch (e) {
                    console.error("[ClassChatWindow] TIM 拉取失败", e);
                }
            }
            return [];
        })();

        // 3. Wait for both
        const [backendData, timList] = await Promise.all([backendPromise, timPromise]);

        const backendMembers = backendData.members || [];
        const groupInfo = backendData.groupInfo;
        const finalTimList: any[] = Array.isArray(timList) ? timList : [];

        console.log(`[ClassChatWindow] 数据获取完毕. 后端: ${backendMembers.length}, TIM: ${finalTimList.length}`);

        // Check ownership
        const selfInTim = finalTimList.find((m: any) => m.userID === currentUserId);
        setIsOwner(selfInTim?.role === 'Owner');

        // 4. Merge Logic
        let mergedList: any[] = [];

        // If we have backend members, use them as base
        if (backendMembers.length > 0) {
            mergedList = backendMembers.map((bm: any) => {
                const userIdStr = String(bm.user_id || bm.id || bm.Member_Account);
                const tm = finalTimList.find((t: any) => t.userID == userIdStr);

                // Avatar Fallback Logic - Prioritize Backend
                let avatarUrl = bm.face_url || bm.avatar || tm?.avatar;

                if (!avatarUrl && groupInfo) {
                    if (String(groupInfo.classid) === userIdStr || bm.user_name === '班级' || bm.name === '班级' || userIdStr.startsWith("0000")) {
                        avatarUrl = groupInfo.face_url;
                        console.log(`[ClassChatWindow] 已为 ${userIdStr} 应用群头像回退`);
                    }
                }

                // Role Mapping
                let role = tm?.role || bm.role;
                if (!role && bm.self_role) {
                    if (bm.self_role === 400) role = 'Owner';
                    else if (bm.self_role === 300) role = 'Admin';
                    else role = 'Member';
                }

                return {
                    userID: userIdStr,
                    nick: bm.user_name || bm.name || bm.student_name || tm?.nick || "未知用户",
                    avatar: avatarUrl,
                    role: role || 'Member',
                };
            });
        } else {
            // Fallback to TIM only if backend failed empty
            mergedList = finalTimList.map((tm: any) => ({
                userID: tm.userID,
                nick: tm.nick || tm.userID,
                avatar: tm.avatar,
                role: tm.role,
            }));
        }

        // Sort: Owner -> Class Bot -> Others
        mergedList.sort((a, b) => {
            const isOwnerA = a.role === 'Owner';
            const isOwnerB = b.role === 'Owner';
            if (isOwnerA && !isOwnerB) return -1;
            if (!isOwnerA && isOwnerB) return 1;

            const isClassA = a.nick === '班级' || a.userID.startsWith('0000');
            const isClassB = b.nick === '班级' || b.userID.startsWith('0000');
            if (isClassA && !isClassB) return -1;
            if (!isClassA && isClassB) return 1;

            return 0;
        });

        setMembers(mergedList);
    };

    // Handler: Exit Group
    const handleExitGroup = async () => {
        if (!groupclassId) return;

        const currentUserId = localStorage.getItem('teacher_unique_id') || '';

        if (isOwner) {
            // Owner needs to transfer ownership first
            // Find other members (excluding self)
            const otherMembers = members.filter(m => m.userID !== currentUserId);
            if (otherMembers.length === 0) {
                alert('您是群内唯一成员，无法转让群主。请使用"解散群聊"。');
                return;
            }
            setShowTransferModal(true);
        } else {
            // Regular member can just quit
            if (!confirm('确定要退出群聊吗？')) return;
            try {
                await quitGroup(groupclassId);
                alert('已成功退出群聊');
                getCurrentWindow().close();
            } catch (e: any) {
                alert('退出群聊失败: ' + (e.message || e));
            }
        }
        setShowGroupMenu(false);
    };

    // Handler: Transfer Ownership and then Quit (for owner exit)
    const handleTransferAndQuit = async (newOwnerId: string) => {
        if (!groupclassId) return;
        try {
            await changeGroupOwner(groupclassId, newOwnerId);
            await quitGroup(groupclassId);
            alert('已成功转让群主并退出群聊');
            getCurrentWindow().close();
        } catch (e: any) {
            alert('操作失败: ' + (e.message || e));
        }
        setShowTransferModal(false);
    };

    // Handler: Disband Group (Owner only)
    const handleDisbandGroup = async () => {
        if (!groupclassId) return;
        if (!confirm('确定要解散群聊吗？此操作不可恢复！')) return;

        try {
            await dismissGroup(groupclassId);
            // Success - silently close window without alert
            getCurrentWindow().close();
        } catch (e: any) {
            alert('解散群聊失败: ' + (e.message || e));
        }
        setShowGroupMenu(false);
    };

    // Close menu when clicking outside
    useEffect(() => {
        const handleClickOutside = (e: MouseEvent) => {
            if (groupMenuRef.current && !groupMenuRef.current.contains(e.target as Node)) {
                setShowGroupMenu(false);
            }
        };
        if (showGroupMenu) {
            document.addEventListener('mousedown', handleClickOutside);
        }
        return () => document.removeEventListener('mousedown', handleClickOutside);
    }, [showGroupMenu]);

    const handleSendMessage = async () => {
        if (!inputText.trim() || !groupclassId) return;
        try {
            const msg = await sendMessage(groupclassId, inputText);
            if (msg) {
                setMessages(prev => [...prev, msg]);
                setInputText('');
            }
        } catch (e) {
            console.error('Failed to send message', e);
        }
    };

    const handleImageSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
        console.log('ClassChatWindow: Image selected');
        if (e.target.files && e.target.files[0] && groupclassId) {
            try {
                console.log('ClassChatWindow: Sending Image...', e.target.files[0].name);
                const msg = await sendImageMessage(groupclassId, e.target.files[0]);
                console.log('ClassChatWindow: Image sent result:', msg);
                if (msg) {
                    setMessages(prev => [...prev, msg]);
                } else {
                    alert("发送图片失败：SDK返回空消息");
                }
            } catch (err: any) {
                console.error("Failed to send image", err);
                alert("发送图片失败: " + (err.message || JSON.stringify(err)));
            }
        }
        if (e.target) e.target.value = ''; // Reset input
    };

    const handleFileSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
        console.log('ClassChatWindow: File selected');
        if (e.target.files && e.target.files[0] && groupclassId) {
            try {
                console.log('ClassChatWindow: Sending File...', e.target.files[0].name);
                const msg = await sendFileMessage(groupclassId, e.target.files[0]);
                console.log('ClassChatWindow: File sent result:', msg);
                if (msg) {
                    setMessages(prev => [...prev, msg]);
                } else {
                    alert("发送文件失败：SDK返回空消息");
                }
            } catch (err: any) {
                console.error("Failed to send file", err);
                alert("发送文件失败: " + (err.message || JSON.stringify(err)));
            }
        }
        if (e.target) e.target.value = ''; // Reset input
    };

    const handleKeyDown = (e: React.KeyboardEvent) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleSendMessage();
        }
    };

    const renderMessageContent = (msg: any) => {
        // Normalize type check
        const type = msg.type || msg.elements?.[0]?.type;
        const payload = msg.payload || msg.elements?.[0]?.content || msg.elements?.[0]?.payload;

        if (type === 'TIMTextElem' || type === 'TIMText') {
            // Handle both structure nuances if any
            const text = payload?.text || payload;
            return (
                <div className={`p-2.5 px-4 rounded-2xl text-sm font-bold leading-relaxed shadow-sm break-words ${msg.flow === 'out' ? 'bg-sage-500 text-white rounded-tr-sm' : 'bg-white text-ink-600 rounded-tl-sm border border-sage-100'}`}>
                    {typeof text === 'string' ? text : '[文本解析错误]'}
                </div>
            );
        } else if (type === 'TIMImageElem' || type === 'TIMImage') {
            const imageInfoArray = payload?.imageInfoArray || payload?.imageArray; // defensive
            const imgUrl = imageInfoArray?.[0]?.url || imageInfoArray?.[1]?.url;
            return (
                <div className="max-w-[200px] rounded-2xl overflow-hidden border border-sage-200 shadow-sm cursor-pointer hover:shadow-md transition-all" onClick={() => imgUrl && window.open(imgUrl, '_blank')}>
                    {imgUrl ? <img src={imgUrl} alt="Image" className="w-full h-auto" /> : <div className="p-4 bg-sage-50 text-xs text-sage-400">图片加载中...</div>}
                </div>
            );
        } else if (type === 'TIMFileElem' || type === 'TIMFile') {
            return (
                <div className="flex items-center gap-3 p-3 bg-white rounded-2xl border border-sage-200 shadow-sm min-w-[200px] cursor-pointer hover:bg-sage-50 transition-colors" onClick={() => payload?.fileUrl && window.open(payload.fileUrl, '_blank')}>
                    <div className="w-10 h-10 bg-sage-100 rounded-xl flex items-center justify-center text-sage-600">
                        <Paperclip size={20} />
                    </div>
                    <div className="flex flex-col overflow-hidden">
                        <span className="text-sm font-bold text-ink-600 truncate">{payload?.fileName || '未知文件'}</span>
                        <span className="text-[10px] text-ink-400">{payload?.fileSize ? (payload.fileSize / 1024).toFixed(1) + ' KB' : ''}</span>
                    </div>
                </div>
            );
        }

        console.warn('Unknown message type:', msg);
        return (
            <div className="p-2 bg-gray-100 text-gray-500 text-xs rounded border border-gray-200">
                [不支持的消息类型: {type || JSON.stringify(msg)}]
            </div>
        );
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

    return (
        <div className="h-screen w-screen bg-paper flex flex-col overflow-hidden border border-sage-200 selection:bg-sage-100 selection:text-sage-900 shadow-2xl rounded-2xl">
            {/* Title Bar - Enhanced */}
            <div data-tauri-drag-region className="h-12 bg-white/60 backdrop-blur-md flex items-center justify-between px-4 border-b border-sage-100 select-none">
                <div className="flex items-center gap-3 pointer-events-none">
                    <div className="bg-sage-100 p-1.5 rounded-xl text-sage-600 shadow-sm"><MessageCircle size={18} /></div>
                    <div className="flex flex-col">
                        <span className="font-bold text-ink-600 text-sm">班级群 - {groupclassId}</span>
                        <span className="text-[10px] text-ink-400 flex items-center gap-1">
                            {!isReady ? '连接中...' : `在线人数: ${members.length}`}
                        </span>
                    </div>
                </div>
                <div className="flex items-center gap-1">
                    {/* Data Import Trigger */}
                    <button
                        onClick={() => setShowCustomList(true)}
                        className="p-1.5 hover:bg-sage-100 rounded-lg text-sage-400 hover:text-sage-600 transition-colors"
                        title="数据导入"
                    >
                        <Paperclip size={16} />
                    </button>

                    {/* Group Settings Menu */}
                    <div className="relative" ref={groupMenuRef}>
                        <button
                            onClick={() => setShowGroupMenu(!showGroupMenu)}
                            className="p-1.5 hover:bg-sage-100 rounded-lg text-sage-400 hover:text-sage-600 transition-colors"
                            title="群设置"
                        >
                            <MoreHorizontal size={16} />
                        </button>

                        {showGroupMenu && (
                            <div className="absolute right-0 top-8 w-40 bg-white rounded-xl shadow-xl border border-sage-100 py-1.5 z-50 animate-in fade-in slide-in-from-top-2 duration-150 ring-1 ring-black/5">
                                <button
                                    onClick={handleExitGroup}
                                    className="w-full px-4 py-2.5 text-left text-sm text-ink-600 hover:bg-sage-50 flex items-center gap-2 font-medium"
                                >
                                    <LogOut size={14} />
                                    <span>退出群聊</span>
                                </button>
                                {isOwner && (
                                    <button
                                        onClick={handleDisbandGroup}
                                        className="w-full px-4 py-2.5 text-left text-sm text-red-500 hover:bg-red-50 flex items-center gap-2 font-medium"
                                    >
                                        <Trash2 size={14} />
                                        <span>解散群聊</span>
                                    </button>
                                )}
                            </div>
                        )}
                    </div>

                    <button onClick={handleMinimize} className="p-1.5 hover:bg-sage-100 rounded-lg text-sage-400 hover:text-sage-600 transition-colors"><Minus size={16} /></button>
                    <button onClick={handleMaximize} className="p-1.5 hover:bg-sage-100 rounded-lg text-sage-400 hover:text-sage-600 transition-colors">{isMaximized ? <Copy size={16} /> : <Square size={16} />}</button>
                    <button onClick={handleClose} className="p-1.5 hover:bg-red-100 hover:text-red-500 rounded-lg text-sage-400 transition-colors"><X size={16} /></button>
                </div>
            </div>

            <div className="flex-1 flex overflow-hidden">
                {/* Main Chat Area */}
                <div className="flex-1 flex flex-col min-w-0 bg-transparent">
                    {/* Messages */}
                    <div className="flex-1 overflow-y-auto p-4 space-y-4 custom-scrollbar">
                        {messages.map((msg, index) => {
                            const isSelf = msg.flow === 'out';
                            return (
                                <div key={msg.ID || index} className={`flex gap-3 ${isSelf ? 'flex-row-reverse' : ''}`}>
                                    <div className={`w-9 h-9 rounded-full flex items-center justify-center text-xs font-bold border shadow-sm shrink-0 select-none ${isSelf ? 'bg-sage-500 text-white' : 'bg-white text-ink-500 border-sage-100'}`}>
                                        {isSelf ? '我' : (msg.nick || msg.from || '?').substring(0, 1).toUpperCase()}
                                    </div>
                                    <div className={`flex flex-col gap-1 max-w-[70%] ${isSelf ? 'items-end' : ''}`}>
                                        <div className="flex items-center gap-2">
                                            <span className="text-xs text-ink-300">{isSelf ? '' : (msg.nick || msg.from)}</span>
                                            <span className="text-[10px] text-ink-200">{new Date(msg.time * 1000).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}</span>
                                        </div>
                                        {renderMessageContent(msg)}
                                    </div>
                                </div>
                            );
                        })}
                        {messages.length === 0 && (
                            <div className="text-center text-ink-300 mt-10 text-sm">
                                {isReady ? '暂无消息' : '正在加载聊天...'}
                            </div>
                        )}
                    </div>

                    {/* Input ToolBar */}
                    <div className="h-[160px] bg-white/60 backdrop-blur border-t border-sage-100 flex flex-col">
                        <div className="flex items-center gap-2 p-2 px-4">
                            <input type="file" ref={imageInputRef} className="hidden" accept="image/*" onChange={handleImageSelect} />
                            <input type="file" ref={fileInputRef} className="hidden" onChange={handleFileSelect} />

                            <button className="text-sage-400 hover:text-sage-600 p-1 transition-colors"><Smile size={20} /></button>
                            <button className="text-sage-400 hover:text-sage-600 p-1 transition-colors" onClick={() => imageInputRef.current?.click()} title="发送图片"><Image size={20} /></button>
                            <button className="text-sage-400 hover:text-sage-600 p-1 transition-colors" onClick={() => fileInputRef.current?.click()} title="发送文件"><Paperclip size={20} /></button>
                            <button className="text-sage-400 hover:text-sage-600 p-1 transition-colors"><Clock size={20} /></button>
                        </div>
                        <textarea
                            value={inputText}
                            onChange={(e) => setInputText(e.target.value)}
                            onKeyDown={handleKeyDown}
                            className="flex-1 resize-none px-4 py-2 outline-none text-ink-600 text-sm leading-relaxed custom-scrollbar bg-transparent placeholder-ink-300"
                            placeholder=""
                        ></textarea>
                        <div className="p-2 px-4 flex justify-end">
                            <button
                                onClick={handleSendMessage}
                                disabled={!isReady}
                                className={`px-6 py-1.5 rounded-lg text-sm font-bold transition-all shadow-sm ${isReady ? 'bg-sage-100 text-sage-600 hover:bg-sage-200 hover:text-sage-700' : 'bg-gray-100 text-gray-400 cursor-not-allowed'}`}
                            >
                                发送 (S)
                            </button>
                        </div>
                    </div>
                </div>

                {/* Right Sidebar - Member List */}
                <div className="w-48 bg-white/40 border-l border-sage-100 flex flex-col backdrop-blur-sm">
                    <div className="h-10 border-b border-sage-100 flex items-center px-4 text-xs text-ink-400 font-bold uppercase tracking-wider">
                        群成员 ({members.length})
                    </div>

                    <div className="flex-1 overflow-y-auto p-2 space-y-1 custom-scrollbar">
                        {members.map((member) => (
                            <div key={member.userID} className="flex items-center gap-2 p-2 hover:bg-sage-50 rounded-xl cursor-pointer transition-colors group">
                                <div className="w-8 h-8 rounded-full bg-white flex items-center justify-center text-[10px] text-ink-400 border border-sage-100 overflow-hidden shrink-0 group-hover:border-sage-300 transition-colors">
                                    {member.avatar ? (
                                        <img src={member.avatar} className="w-full h-full object-cover" alt="" />
                                    ) : (
                                        (member.nick || member.userID || '?').substring(0, 1).toUpperCase()
                                    )}
                                </div>
                                <span className="text-xs text-ink-600 font-medium truncate">{member.nick || member.userID}</span>
                            </div>
                        ))}
                    </div>
                </div>
            </div>

            {/* Modals */}
            <CustomListModal
                isOpen={showCustomList}
                onClose={() => setShowCustomList(false)}
                classId={groupclassId}
            />

            {/* Transfer Ownership Modal (for owner exit) */}
            {showTransferModal && (
                <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/20 backdrop-blur-sm">
                    <div className="bg-white rounded-2xl shadow-xl w-[400px] p-6 border border-sage-100">
                        <h2 className="text-lg font-bold text-ink-700 mb-4 flex items-center gap-2">
                            <UserCheck size={20} className="text-sage-500" />
                            选择新群主
                        </h2>
                        <p className="text-sm text-ink-400 mb-4">您是群主，退出前需先转让群主身份给其他成员。</p>
                        <div className="max-h-[250px] overflow-y-auto space-y-2 mb-4 custom-scrollbar">
                            {members.filter(m => m.userID !== localStorage.getItem('teacher_unique_id')).map((member) => (
                                <button
                                    key={member.userID}
                                    onClick={() => handleTransferAndQuit(member.userID)}
                                    className="w-full flex items-center gap-3 p-3 rounded-xl hover:bg-sage-50 border border-transparent hover:border-sage-200 transition-all text-left group"
                                >
                                    <div className="w-8 h-8 rounded-full bg-gray-100 flex items-center justify-center text-xs text-gray-500 border border-gray-200">
                                        {member.avatar ? <img src={member.avatar} className="w-full h-full rounded-full" /> : (member.nick || member.userID).substring(0, 1).toUpperCase()}
                                    </div>
                                    <span className="text-sm text-ink-600 font-bold group-hover:text-sage-700">{member.nick || member.userID}</span>
                                    {member.role === 'Admin' && <span className="text-[10px] bg-sky-100 text-sky-600 px-1.5 py-0.5 rounded-full font-bold">管理员</span>}
                                </button>
                            ))}
                        </div>
                        <button
                            onClick={() => setShowTransferModal(false)}
                            className="w-full py-2.5 text-sm font-bold text-ink-500 hover:bg-gray-100 rounded-xl transition-colors"
                        >
                            取消
                        </button>
                    </div>
                </div>
            )}
        </div>
    );
};

export default ClassChatWindow;
