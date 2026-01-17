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
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[480px] max-h-[600px] overflow-hidden border border-white/50 ring-1 ring-sage-100/50 flex flex-col transition-all"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="px-6 py-4 flex items-center justify-between cursor-move select-none border-b border-sage-100/50 bg-white/30 backdrop-blur-md relative z-10"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 bg-gradient-to-br from-sage-400 to-sage-600 text-white rounded-xl flex items-center justify-center shadow-lg shadow-sage-500/20">
                            {showHistory ? <History size={20} /> : <Bell size={20} />}
                        </div>
                        <div>
                            <h3 className="font-bold text-ink-800 text-base tracking-tight">
                                {showHistory ? '发送历史记录' : '发送通知消息'}
                            </h3>
                            <p className="text-xs text-ink-400 font-medium truncate max-w-[200px]">
                                {displayName}
                            </p>
                        </div>
                    </div>

                    <div className="flex items-center gap-2">
                        {!showHistory ? (
                            <button
                                onClick={() => setShowHistory(true)}
                                className="p-2 text-sage-400 hover:text-sage-600 hover:bg-sage-50 rounded-lg transition-all"
                                title="查看历史"
                            >
                                <History size={20} />
                            </button>
                        ) : (
                            <button
                                onClick={() => setShowHistory(false)}
                                className="p-2 text-sage-400 hover:text-sage-600 hover:bg-sage-50 rounded-lg transition-all"
                                title="返回发送"
                            >
                                <ChevronLeft size={20} />
                            </button>
                        )}
                        <button
                            onClick={onClose}
                            className="p-2 text-sage-400 hover:text-clay-600 hover:bg-clay-50 rounded-lg transition-colors"
                        >
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {showHistory ? (
                    /* History View */
                    <div className="flex-1 overflow-y-auto p-4 space-y-3 bg-white/40 custom-scrollbar">
                        {notices.length > 0 ? (
                            notices.map(notice => (
                                <div key={notice.id} className="bg-white/90 backdrop-blur-sm rounded-xl p-4 shadow-sm border border-white/60 hover:border-sage-200 group relative hover:shadow-md transition-all">
                                    <div className="flex items-start justify-between gap-3">
                                        <p className="text-ink-700 text-sm leading-relaxed whitespace-pre-wrap">{notice.content}</p>
                                        <button
                                            onClick={() => handleDelete(notice.id)}
                                            className="text-sage-300 hover:text-clay-600 p-1.5 hover:bg-clay-50 rounded-lg opacity-0 group-hover:opacity-100 transition-all flex-shrink-0"
                                            title="删除"
                                        >
                                            <Trash2 size={16} />
                                        </button>
                                    </div>
                                    <div className="flex items-center gap-2 mt-3 pt-3 border-t border-sage-50">
                                        <div className="flex items-center gap-1.5 px-2 py-0.5 bg-sage-50 text-sage-600 rounded text-[10px] font-bold border border-sage-100">
                                            <Check size={10} strokeWidth={3} />
                                            <span>已发送</span>
                                        </div>
                                        <span className="text-[10px] text-ink-400 font-medium font-mono">
                                            {new Date(notice.timestamp).toLocaleString()}
                                        </span>
                                    </div>
                                </div>
                            ))
                        ) : (
                            <div className="flex flex-col items-center justify-center py-12 text-sage-300 gap-3">
                                <div className="w-16 h-16 bg-sage-50 rounded-full flex items-center justify-center border border-sage-100">
                                    <History size={24} className="opacity-50" />
                                </div>
                                <p className="text-sm font-bold text-ink-400">暂无发送历史</p>
                            </div>
                        )}
                    </div>
                ) : (
                    /* Send View */
                    <div className="bg-white/40 flex flex-col h-full">
                        <div className="p-6 flex-1 pb-0">
                            <label className="block text-xs font-bold text-ink-400 mb-2 uppercase tracking-wide px-1">
                                消息内容
                            </label>
                            <div className="relative group">
                                <textarea
                                    value={newNotice}
                                    onChange={e => setNewNotice(e.target.value)}
                                    placeholder="请输入需要发送给全班的通知内容..."
                                    className="w-full h-48 bg-white text-ink-800 border border-sage-200 rounded-2xl px-5 py-4 text-sm focus:ring-4 focus:ring-sage-100 focus:border-sage-400 transition-all outline-none resize-none placeholder:text-sage-300 leading-relaxed scrollbar-thin shadow-inner"
                                    autoFocus
                                />
                                <div className="absolute bottom-3 right-3 text-[10px] text-sage-400 font-bold bg-white/90 px-2 py-1 rounded-md border border-sage-100 shadow-sm pointer-events-none">
                                    {newNotice.length} 字
                                </div>
                            </div>

                            <div className="mt-4 flex items-start gap-3 text-xs text-ink-500 bg-sage-50/50 p-4 rounded-xl border border-sage-100/50">
                                <div className="mt-0.5 text-sage-500 flex-shrink-0">
                                    <Clock size={16} />
                                </div>
                                <p className="leading-relaxed">消息将即时发送到班级群，所有在线成员均可收到。</p>
                            </div>
                        </div>

                        <div className="p-6 flex items-center justify-end gap-3 bg-white/50 border-t border-white/50 mt-auto backdrop-blur-sm">
                            <button
                                onClick={onClose}
                                className="px-6 py-3 text-sm font-bold text-ink-500 hover:text-ink-700 hover:bg-white border border-transparent hover:border-sage-200 rounded-2xl transition-all"
                            >
                                取消
                            </button>
                            <button
                                onClick={handleSend}
                                disabled={!newNotice.trim() || sendStatus !== 'idle'}
                                className={`px-8 py-3 text-sm font-bold text-white rounded-2xl flex items-center gap-2 shadow-lg transition-all transform active:scale-95
                                    ${!newNotice.trim() && sendStatus === 'idle'
                                        ? 'bg-gray-300 shadow-none cursor-not-allowed text-white/80'
                                        : sendStatus === 'success'
                                            ? 'bg-green-500 hover:bg-green-600 shadow-green-500/30'
                                            : 'bg-gradient-to-r from-sage-500 to-sage-600 hover:from-sage-600 hover:to-sage-700 hover:-translate-y-0.5 shadow-sage-500/30'}`}
                            >
                                {sendStatus === 'sending' ? (
                                    <>
                                        <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                                        <span>发送中...</span>
                                    </>
                                ) : sendStatus === 'success' ? (
                                    <>
                                        <Check size={18} strokeWidth={3} />
                                        <span>已发送</span>
                                    </>
                                ) : (
                                    <>
                                        <span>立即发送</span>
                                        <div className="w-px h-3 bg-white/20 mx-1"></div>
                                        <Bell size={16} className="opacity-90" />
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
