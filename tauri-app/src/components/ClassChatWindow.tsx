import { useEffect, useState } from 'react';
import { useParams } from 'react-router-dom';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { Minus, X, Square, Copy, MessageCircle, MoreHorizontal, Image, Paperclip, Smile, Clock } from 'lucide-react';
import { sendMessage, getMessageList, loginTIM, isSDKReady, getGroupMemberList } from '../utils/tim';

const ClassChatWindow = () => {
    const { groupclassId } = useParams();
    const [isMaximized, setIsMaximized] = useState(false);
    const [messages, setMessages] = useState<any[]>([]);
    const [inputText, setInputText] = useState('');
    const [isReady, setIsReady] = useState(false);

    const [members, setMembers] = useState<any[]>([]);

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
        if (!groupclassId) return;
        const list = await getGroupMemberList(groupclassId);
        setMembers(list);
    };

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

    const handleKeyDown = (e: React.KeyboardEvent) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleSendMessage();
        }
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
        <div className="h-screen w-screen bg-[#f3f9fe] flex flex-col overflow-hidden border border-gray-300 selection:bg-blue-100 selection:text-blue-900 shadow-xl">
            {/* Title Bar - Enhanced */}
            <div data-tauri-drag-region className="h-12 bg-gradient-to-r from-blue-50 to-white flex items-center justify-between px-4 border-b border-gray-200 select-none">
                <div className="flex items-center gap-3">
                    <div className="bg-blue-100 p-1.5 rounded-lg text-blue-600"><MessageCircle size={18} /></div>
                    <div className="flex flex-col">
                        <span className="font-bold text-gray-800 text-sm">班级群 - {groupclassId}</span>
                        <span className="text-[10px] text-gray-500 flex items-center gap-1">
                            {!isReady ? '连接中...' : `在线人数: ${members.length}`}
                        </span>
                    </div>
                </div>
                <div className="flex items-center gap-1">
                    <button onClick={handleMinimize} className="p-1.5 hover:bg-gray-200 rounded-md text-gray-500"><Minus size={16} /></button>
                    <button onClick={handleMaximize} className="p-1.5 hover:bg-gray-200 rounded-md text-gray-500">{isMaximized ? <Copy size={16} /> : <Square size={16} />}</button>
                    <button onClick={handleClose} className="p-1.5 hover:bg-red-500 hover:text-white rounded-md text-gray-500"><X size={16} /></button>
                </div>
            </div>

            <div className="flex-1 flex overflow-hidden">
                {/* Main Chat Area */}
                <div className="flex-1 flex flex-col min-w-0 bg-[#f5f6f7]">
                    {/* Messages */}
                    <div className="flex-1 overflow-y-auto p-4 space-y-4 custom-scrollbar">
                        {messages.map((msg, index) => {
                            const isSelf = msg.flow === 'out';
                            return (
                                <div key={msg.ID || index} className={`flex gap-3 ${isSelf ? 'flex-row-reverse' : ''}`}>
                                    <div className={`w-9 h-9 rounded-full flex items-center justify-center text-xs font-bold border shadow-sm shrink-0 select-none ${isSelf ? 'bg-blue-600 text-white' : 'bg-white text-gray-600 border-gray-200'}`}>
                                        {isSelf ? '我' : (msg.nick || msg.from || '?').substring(0, 1).toUpperCase()}
                                    </div>
                                    <div className={`flex flex-col gap-1 max-w-[70%] ${isSelf ? 'items-end' : ''}`}>
                                        <div className="flex items-center gap-2">
                                            <span className="text-xs text-gray-400">{isSelf ? '' : (msg.nick || msg.from)}</span>
                                            <span className="text-[10px] text-gray-300">{new Date(msg.time * 1000).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}</span>
                                        </div>
                                        <div className={`p-2.5 px-4 rounded-2xl text-sm leading-relaxed shadow-sm break-words ${isSelf ? 'bg-blue-600 text-white rounded-tr-sm' : 'bg-white text-gray-800 rounded-tl-sm border border-gray-100'}`}>
                                            {msg.payload?.text || '[不支持的消息类型]'}
                                        </div>
                                    </div>
                                </div>
                            );
                        })}
                        {messages.length === 0 && (
                            <div className="text-center text-gray-400 mt-10 text-sm">
                                {isReady ? '暂无消息' : '正在加载聊天...'}
                            </div>
                        )}
                    </div>

                    {/* Input ToolBar */}
                    <div className="h-[160px] bg-white border-t border-gray-200 flex flex-col">
                        <div className="flex items-center gap-2 p-2 px-4">
                            <button className="text-gray-500 hover:text-gray-700 p-1"><Smile size={20} /></button>
                            <button className="text-gray-500 hover:text-gray-700 p-1"><Image size={20} /></button>
                            <button className="text-gray-500 hover:text-gray-700 p-1"><Paperclip size={20} /></button>
                            <button className="text-gray-500 hover:text-gray-700 p-1"><Clock size={20} /></button>
                        </div>
                        <textarea
                            value={inputText}
                            onChange={(e) => setInputText(e.target.value)}
                            onKeyDown={handleKeyDown}
                            className="flex-1 resize-none px-4 py-2 outline-none text-gray-800 text-sm leading-relaxed custom-scrollbar"
                            placeholder=""
                        ></textarea>
                        <div className="p-2 px-4 flex justify-end">
                            <button
                                onClick={handleSendMessage}
                                disabled={!isReady}
                                className={`px-6 py-1.5 rounded text-sm transition-colors border ${isReady ? 'bg-blue-50 text-blue-600 border-blue-200 hover:bg-blue-100' : 'bg-gray-100 text-gray-400 border-gray-200 cursor-not-allowed'}`}
                            >
                                发送 (S)
                            </button>
                        </div>
                    </div>
                </div>

                {/* Right Sidebar - Member List */}
                <div className="w-48 bg-white border-l border-gray-200 flex flex-col">
                    <div className="h-10 border-b border-gray-100 flex items-center px-4 text-xs text-gray-500 font-medium">
                        群成员 ({members.length})
                    </div>
                    <div className="flex-1 overflow-y-auto p-2 space-y-1 custom-scrollbar">
                        {members.map((member) => (
                            <div key={member.userID} className="flex items-center gap-2 p-1.5 hover:bg-gray-50 rounded cursor-pointer">
                                <div className="w-6 h-6 rounded-full bg-gray-100 flex items-center justify-center text-[10px] text-gray-500 border border-gray-200">
                                    {member.avatar ? <img src={member.avatar} className="w-full h-full rounded-full" /> : (member.nick || member.userID).substring(0, 1).toUpperCase()}
                                </div>
                                <span className="text-xs text-gray-700 truncate">{member.nick || member.userID}</span>
                            </div>
                        ))}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default ClassChatWindow;
