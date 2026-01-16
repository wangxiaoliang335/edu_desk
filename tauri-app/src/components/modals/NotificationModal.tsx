import { useState, useEffect } from 'react';
import { X, Clock, Trash2, History, ChevronLeft, Bell, Check } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessageWS } from '../../utils/websocket';

interface Props {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
    groupId?: string;
    groupName?: string;
    className?: string;
}

interface Notice {
    id: string;
    content: string;
    senderName: string;
    timestamp: number;
}

const MAX_NOTICES = 100;

const getStorageKey = (classId: string) => `notification_history_${classId}`;

const loadNotices = (classId: string): Notice[] => {
    try {
        const data = localStorage.getItem(getStorageKey(classId));
        return data ? JSON.parse(data) : [];
    } catch {
        return [];
    }
};

const saveNotices = (classId: string, notices: Notice[]) => {
    const trimmed = notices.slice(0, MAX_NOTICES);
    localStorage.setItem(getStorageKey(classId), JSON.stringify(trimmed));
};

const NotificationModal = ({ isOpen, onClose, classId, groupId, groupName, className }: Props) => {
    const { style, handleMouseDown } = useDraggable();
    const [showHistory, setShowHistory] = useState(false);
    const [notices, setNotices] = useState<Notice[]>([]);
    const [newNotice, setNewNotice] = useState("");
    const [sendStatus, setSendStatus] = useState<'idle' | 'sending' | 'success'>('idle');

    useEffect(() => {
        if (isOpen && classId) {
            setNotices(loadNotices(classId));
            setShowHistory(false);
            setSendStatus('idle');
        }
    }, [isOpen, classId]);

    const handleSend = async () => {
        if (!newNotice.trim()) return;
        if (!classId) {
            alert("班级ID缺失，无法发送通知");
            return;
        }

        setSendStatus('sending');

        try {
            const userInfoStr = localStorage.getItem('user_info');
            let senderName = "老师";
            let senderId = "";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    senderName = u.name || u.strName || u.data?.name || "老师";
                    senderId = u.teacher_unique_id || u.data?.teacher_unique_id || "";
                } catch (e) { }
            }

            const notificationObj: Record<string, string> = {
                type: "notification",
                class_id: classId,
                content: newNotice.trim(),
                content_text: "notification",
                sender_name: senderName,
            };

            if (senderId) notificationObj.sender_id = senderId;
            if (groupId) {
                notificationObj.group_id = groupId;
                notificationObj.unique_group_id = groupId;
            }
            if (groupName) notificationObj.group_name = groupName;

            let wsMessage = groupId
                ? `to:${groupId}:${JSON.stringify(notificationObj)}`
                : JSON.stringify(notificationObj);

            console.log('[NotificationModal] Sending:', wsMessage);
            sendMessageWS(wsMessage);

            // Save to history
            const newNoticeObj: Notice = {
                id: Date.now().toString(),
                content: newNotice.trim(),
                senderName,
                timestamp: Date.now()
            };
            const updatedNotices = [newNoticeObj, ...notices];
            setNotices(updatedNotices);
            saveNotices(classId, updatedNotices);

            setSendStatus('success');

            // Wait a bit to show success state
            setTimeout(() => {
                setNewNotice("");
                setSendStatus('idle');
                // Auto-close removed
            }, 1500);

        } catch (e) {
            console.error("Failed to send notice", e);
            alert("发送失败，请重试");
            setSendStatus('idle');
        }
    };

    const handleDelete = (id: string) => {
        if (!classId) return;
        const updated = notices.filter(n => n.id !== id);
        setNotices(updated);
        saveNotices(classId, updated);
    };

    if (!isOpen) return null;

    const displayName = className || groupName || classId || "班级";

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-slate-900/50 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-white/90 backdrop-blur-xl rounded-2xl shadow-2xl w-[480px] max-h-[600px] overflow-hidden border border-white/60 ring-1 ring-black/5 flex flex-col transition-all"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="px-6 py-4 flex items-center justify-between cursor-move select-none border-b border-gray-100 bg-white/50 relative z-10"
                >
                    <div className="flex items-center gap-3">
                        <div className="p-2 bg-blue-50 text-blue-600 rounded-lg shadow-sm border border-blue-100">
                            {showHistory ? <History size={20} /> : <Bell size={20} />}
                        </div>
                        <div>
                            <h3 className="font-bold text-slate-800 text-base tracking-tight">
                                {showHistory ? '发送历史记录' : '发送通知消息'}
                            </h3>
                            <p className="text-xs text-slate-500 font-medium truncate max-w-[200px]">
                                {displayName}
                            </p>
                        </div>
                    </div>

                    <div className="flex items-center gap-2">
                        {!showHistory ? (
                            <button
                                onClick={() => setShowHistory(true)}
                                className="p-2 text-slate-400 hover:text-blue-600 hover:bg-blue-50 rounded-lg transition-all"
                                title="查看历史"
                            >
                                <History size={18} />
                            </button>
                        ) : (
                            <button
                                onClick={() => setShowHistory(false)}
                                className="p-2 text-slate-400 hover:text-blue-600 hover:bg-blue-50 rounded-lg transition-all"
                                title="返回发送"
                            >
                                <ChevronLeft size={18} />
                            </button>
                        )}
                        <button
                            onClick={onClose}
                            className="p-2 text-slate-400 hover:text-red-500 hover:bg-red-50 rounded-lg transition-colors"
                        >
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {showHistory ? (
                    /* History View */
                    <div className="flex-1 overflow-y-auto p-4 space-y-3 bg-slate-50/50 custom-scrollbar">
                        {notices.length > 0 ? (
                            notices.map(notice => (
                                <div key={notice.id} className="bg-white rounded-xl p-4 shadow-sm border border-gray-100 group relative hover:shadow-md transition-all">
                                    <div className="flex items-start justify-between gap-2">
                                        <p className="text-slate-700 text-sm leading-relaxed whitespace-pre-wrap">{notice.content}</p>
                                        <button
                                            onClick={() => handleDelete(notice.id)}
                                            className="text-slate-300 hover:text-red-500 p-1.5 hover:bg-red-50 rounded-lg opacity-0 group-hover:opacity-100 transition-all flex-shrink-0"
                                            title="删除"
                                        >
                                            <Trash2 size={14} />
                                        </button>
                                    </div>
                                    <div className="flex items-center gap-1.5 mt-3 pt-3 border-t border-gray-50">
                                        <div className="flex items-center gap-1 px-2 py-0.5 bg-blue-50 text-blue-600 rounded text-[10px] font-bold">
                                            <Clock size={10} />
                                            <span>已发送</span>
                                        </div>
                                        <span className="text-[10px] text-slate-400 font-medium font-mono">
                                            {new Date(notice.timestamp).toLocaleString()}
                                        </span>
                                    </div>
                                </div>
                            ))
                        ) : (
                            <div className="flex flex-col items-center justify-center py-12 text-slate-400 gap-3">
                                <div className="w-16 h-16 bg-slate-100 rounded-full flex items-center justify-center">
                                    <History size={24} className="opacity-50" />
                                </div>
                                <p className="text-sm font-medium">暂无发送历史</p>
                            </div>
                        )}
                    </div>
                ) : (
                    /* Send View */
                    <div className="bg-white flex flex-col h-full">
                        <div className="p-5 flex-1 pb-0">
                            <label className="block text-xs font-bold text-slate-500 mb-2 uppercase tracking-wide">
                                消息内容
                            </label>
                            <div className="relative">
                                <textarea
                                    value={newNotice}
                                    onChange={e => setNewNotice(e.target.value)}
                                    placeholder="请输入需要发送给全班的通知内容..."
                                    className="w-full h-48 bg-slate-50 text-slate-800 border border-slate-200 rounded-xl px-4 py-3 text-sm focus:bg-white focus:border-blue-500 focus:ring-4 focus:ring-blue-500/10 transition-all outline-none resize-none placeholder:text-slate-400 leading-relaxed scrollbar-thin"
                                    autoFocus
                                />
                                <div className="absolute bottom-3 right-3 text-[10px] text-slate-400 font-medium bg-slate-50/80 px-1.5 py-0.5 rounded border border-slate-100">
                                    {newNotice.length} 字
                                </div>
                            </div>

                            <div className="mt-4 flex items-start gap-2 text-xs text-slate-400 bg-blue-50/50 p-3 rounded-lg border border-blue-100/50">
                                <div className="mt-0.5 text-blue-500 flex-shrink-0">
                                    <Clock size={14} />
                                </div>
                                <p>消息将即时发送到班级群，所有在线成员均可收到。</p>
                            </div>
                        </div>

                        <div className="p-5 flex items-center justify-end gap-3 bg-slate-50 border-t border-slate-100 mt-auto">
                            <button
                                onClick={onClose}
                                className="px-5 py-2.5 text-sm font-bold text-slate-500 hover:text-slate-700 hover:bg-slate-200/50 rounded-xl transition-colors"
                            >
                                取消
                            </button>
                            <button
                                onClick={handleSend}
                                disabled={!newNotice.trim() || sendStatus !== 'idle'}
                                className={`px-8 py-2.5 text-sm font-bold text-white rounded-xl flex items-center gap-2 shadow-lg transition-all transform active:scale-95
                                    ${!newNotice.trim() && sendStatus === 'idle'
                                        ? 'bg-slate-300 shadow-none cursor-not-allowed text-slate-500'
                                        : sendStatus === 'success'
                                            ? 'bg-green-500 hover:bg-green-600 shadow-green-500/30'
                                            : 'bg-gradient-to-r from-blue-500 to-indigo-600 hover:from-blue-600 hover:to-indigo-700 hover:-translate-y-0.5 shadow-blue-500/30'}`}
                            >
                                {sendStatus === 'sending' ? (
                                    <>
                                        <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                                        <span>发送中...</span>
                                    </>
                                ) : sendStatus === 'success' ? (
                                    <>
                                        <Check size={18} />
                                        <span>已发送</span>
                                    </>
                                ) : (
                                    <>
                                        <span>立即发送</span>
                                        <div className="w-px h-3 bg-white/20 mx-0.5"></div>
                                        <Clock size={14} className="opacity-80" />
                                    </>
                                )}
                            </button>
                        </div>
                    </div>
                )}
            </div>
        </div>
    );
};

export default NotificationModal;
