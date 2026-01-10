import { useState, useEffect } from 'react';
import { X, Bell, Send, Clock, Trash2 } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessage, addMessageListener } from '../../utils/tim';

interface Props {
    isOpen: boolean;
    onClose: () => void;
    groupId?: string;
}

interface Notice {
    id: string;
    content: string;
    date: string;
    sender: string;
    isNew?: boolean;
}

const NotificationModal = ({ isOpen, onClose, groupId }: Props) => {
    const { style, handleMouseDown } = useDraggable();
    const [notices, setNotices] = useState<Notice[]>([
        {
            id: "1",
            content: "请各位同学注意，本周五下午将进行期中考试模拟，请提前做好准备。考试范围包括第一章至第三章。",
            date: "2023-11-15 14:30",
            sender: "张老师",
            isNew: true
        },
        {
            id: "2",
            content: "下周一班会课将进行“文明礼仪”主题教育，请班长提前准备好PPT。",
            date: "2023-11-14 09:00",
            sender: "李老师"
        }
    ]);
    const [newNotice, setNewNotice] = useState("");
    const [sending, setSending] = useState(false);

    // Focus input & Listen for notices
    useEffect(() => {
        if (isOpen) {
            // fetch notices (mock or api)
        }

        const removeListener = addMessageListener((event) => {
            // Check if event is for this group
            // event.data matches TIM message structure
            const newMsgs = event.data || [];
            newMsgs.forEach((msg: any) => {
                if (msg.to === groupId || !groupId) { // Simple filter
                    const newNotice: Notice = {
                        id: msg.ID || Date.now().toString(),
                        content: msg.payload?.text || "[非文本消息]",
                        date: new Date(msg.time * 1000).toLocaleString(),
                        sender: msg.from || "未知",
                        isNew: true
                    };
                    setNotices(prev => [newNotice, ...prev]);
                }
            });
        });

        return () => {
            removeListener();
        };
    }, [isOpen, groupId]);

    const handleSend = async () => {
        if (!newNotice.trim()) return;
        setSending(true);

        try {
            // If we have a real groupId, try to send via TIM
            if (groupId) {
                const msgText = `[班级通知] ${newNotice}`;
                await sendMessage(groupId, msgText);
            }

            // Optimistic UI update
            const notice: Notice = {
                id: Date.now().toString(),
                content: newNotice,
                date: new Date().toLocaleString(),
                sender: "我"
            };
            setNotices([notice, ...notices]);
            setNewNotice("");
        } catch (e) {
            console.error("Failed to send notice", e);
            alert("发送失败，请重试");
        } finally {
            setSending(false);
        }
    };

    const handleDelete = (id: string) => {
        if (confirm("确定删除这条通知吗？")) {
            setNotices(notices.filter(n => n.id !== id));
        }
    }

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm p-4">
            <div
                className="bg-white rounded-xl shadow-2xl w-full max-w-lg flex flex-col overflow-hidden animate-in fade-in zoom-in-95 duration-200"
                style={{ ...style, height: '650px' }}
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="flex items-center justify-between p-4 border-b border-gray-100 bg-gray-50/50 cursor-move select-none"
                >
                    <h3 className="font-bold text-gray-800 text-lg flex items-center gap-2">
                        <span className="bg-red-100 text-red-600 p-1 rounded-md"><Bell size={18} /></span>
                        班级通知
                    </h3>
                    <button
                        onClick={onClose}
                        className="p-1.5 hover:bg-gray-200 rounded-full text-gray-500 transition-colors"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-hidden flex flex-col bg-gray-50/30">
                    {/* Notice List */}
                    <div className="flex-1 overflow-y-auto p-4 space-y-4">
                        {notices.length > 0 ? (
                            notices.map(notice => (
                                <div key={notice.id} className={`bg-white p-4 rounded-xl border shadow-sm relative group transition-all hover:shadow-md ${notice.isNew ? 'border-red-200 ring-4 ring-red-50' : 'border-gray-100'}`}>
                                    {notice.isNew && (
                                        <span className="absolute -top-1.5 -right-1.5 bg-red-500 text-white text-[10px] px-1.5 py-0.5 rounded-full shadow-sm animate-pulse">NEW</span>
                                    )}

                                    <div className="flex justify-between items-start mb-2">
                                        <div className="flex items-center gap-2">
                                            <div className="w-8 h-8 rounded-full bg-blue-100 text-blue-600 flex items-center justify-center font-bold text-xs">
                                                {notice.sender[0]}
                                            </div>
                                            <div>
                                                <span className="font-bold text-gray-800 text-sm block">{notice.sender}</span>
                                                <span className="text-[10px] text-gray-400 flex items-center gap-1">
                                                    <Clock size={10} /> {notice.date}
                                                </span>
                                            </div>
                                        </div>
                                        <button
                                            onClick={() => handleDelete(notice.id)}
                                            className="text-gray-300 hover:text-red-500 opacity-0 group-hover:opacity-100 transition-all p-1"
                                        >
                                            <Trash2 size={14} />
                                        </button>
                                    </div>

                                    <p className="text-sm text-gray-600 leading-relaxed whitespace-pre-wrap pl-10">
                                        {notice.content}
                                    </p>
                                </div>
                            ))
                        ) : (
                            <div className="h-full flex flex-col items-center justify-center text-gray-400 opacity-70">
                                <Bell size={48} className="mb-2 opacity-50" />
                                <p className="text-sm">暂无通知</p>
                            </div>
                        )}
                    </div>

                    {/* Input Area */}
                    <div className="p-4 bg-white border-t border-gray-100 shadow-[0_-4px_20px_rgba(0,0,0,0.02)]">
                        <div className="relative">
                            <textarea
                                value={newNotice}
                                onChange={e => setNewNotice(e.target.value)}
                                placeholder="发布新通知..."
                                className="w-full bg-gray-50 border border-gray-200 rounded-xl px-4 py-3 text-sm focus:ring-2 focus:ring-blue-100 focus:border-blue-300 focus:bg-white transition-all outline-none resize-none h-24"
                            />
                            <div className="absolute bottom-3 right-3 flex items-center gap-2">
                                <span className="text-xs text-gray-400">{newNotice.length} 字</span>
                                <button
                                    onClick={handleSend}
                                    disabled={!newNotice.trim() || sending}
                                    className={`p-2 rounded-lg transition-all flex items-center gap-2 ${!newNotice.trim() || sending ? 'bg-gray-100 text-gray-400 cursor-not-allowed' : 'bg-blue-600 text-white hover:bg-blue-700 shadow-md hover:shadow-lg active:scale-95'}`}
                                >
                                    {sending ? (
                                        <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                                    ) : (
                                        <Send size={16} />
                                    )}
                                    <span className="text-xs font-bold">发布</span>
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default NotificationModal;
