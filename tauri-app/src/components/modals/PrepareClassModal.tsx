import { useState, useEffect } from 'react';
import { X, Bookmark, Send } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { useWebSocket } from '../../context/WebSocketContext';

interface Props {
    isOpen: boolean;
    onClose: () => void;
    subject: string;
    time: string;
    classId?: string;
    groupId?: string;
}

const getCacheKey = (classId: string, subject: string, time: string) =>
    `prepare_class_${classId}_${subject}_${time}`;

const PrepareClassModal = ({ isOpen, onClose, subject, time, classId, groupId }: Props) => {
    const { sendMessage } = useWebSocket();
    const { style, handleMouseDown } = useDraggable();
    const [content, setContent] = useState("");
    const [sending, setSending] = useState(false);

    useEffect(() => {
        if (isOpen && classId) {
            const cached = localStorage.getItem(getCacheKey(classId, subject, time));
            if (cached) {
                setContent(cached);
            } else {
                setContent("");
            }
        }
    }, [isOpen, classId, subject, time]);

    const handleSend = async () => {
        if (!content.trim()) {
            alert("请输入课前准备内容");
            return;
        }
        if (!classId) {
            alert("班级ID缺失，无法发送");
            return;
        }
        if (!groupId) {
            alert("群组ID缺失，无法发送");
            return;
        }

        setSending(true);

        try {
            const userInfoStr = localStorage.getItem('user_info');
            let senderName = "老师";
            let senderId = "";
            let schoolId = "";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    senderName = u.name || u.strName || u.data?.name || "老师";
                    senderId = u.teacher_unique_id || u.data?.teacher_unique_id || "";
                    schoolId = u.school_id || u.data?.school_id || "";
                } catch (e) { }
            }

            // Format date as YYYY-MM-DD
            const today = new Date();
            const dateStr = today.toISOString().split('T')[0];

            // Format time as HH:MM (extract start time only)
            let timeStr = time;
            const timeMatch = time.match(/(\d{1,2}):(\d{2})/);
            if (timeMatch) {
                const hour = timeMatch[1].padStart(2, '0');
                const minute = timeMatch[2];
                timeStr = `${hour}:${minute}`;
            }

            // Build message object according to server protocol
            const messageObj: Record<string, string> = {
                type: "prepare_class",
                class_id: classId ? (classId.endsWith('01') ? classId.slice(0, -2) : classId) : "",
                subject: subject.trim(),
                content: content.trim(),
                date: dateStr,
                time: timeStr,
                sender_name: senderName,
            };

            // Optional fields
            if (senderId) messageObj.sender_id = senderId;
            if (schoolId) messageObj.school_id = schoolId;

            // Format: to:<group_id>:<JSON message>
            const wsMessage = `to:${groupId}:${JSON.stringify(messageObj)}`;

            console.log('[PrepareClassModal] Sending:', wsMessage);
            sendMessage(wsMessage);

            localStorage.setItem(getCacheKey(classId, subject, time), content.trim());
            onClose();
        } catch (e) {
            console.error("Failed to send prepare class content", e);
            alert("发送失败，请重试");
        } finally {
            setSending(false);
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[480px] overflow-hidden border border-white/60 ring-1 ring-sage-100/50 flex flex-col animate-in zoom-in-95 duration-200"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="p-5 flex items-center justify-between border-b border-sage-100/50 bg-white/40 backdrop-blur-md cursor-move select-none"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-2xl bg-gradient-to-br from-amber-400 to-amber-600 text-white flex items-center justify-center shadow-lg shadow-amber-500/20">
                            <Bookmark size={20} />
                        </div>
                        <span className="font-bold text-ink-800 text-lg tracking-tight">课前准备</span>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-9 h-9 flex items-center justify-center rounded-full text-sage-400 hover:text-clay-600 hover:bg-clay-50 transition-all duration-300"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Subject Tag */}
                <div className="px-6 pt-5 pb-2 bg-white/30 backdrop-blur-sm">
                    <div className="flex items-center gap-2 text-sm">
                        <span className="px-3 py-1 bg-amber-50 text-amber-700 rounded-lg font-bold text-xs shadow-sm border border-amber-100">
                            {subject}
                        </span>
                        <span className="text-sage-300">|</span>
                        <span className="text-sage-500 font-bold">{time}</span>
                    </div>
                </div>

                {/* Content */}
                <div className="px-6 py-3 bg-white/30 backdrop-blur-sm flex-1">
                    <label className="block text-sage-600 text-sm font-bold mb-2 ml-1">内容</label>
                    <textarea
                        value={content}
                        onChange={e => setContent(e.target.value)}
                        placeholder="请输入课前准备内容..."
                        className="w-full bg-white/60 text-ink-700 border border-sage-200 rounded-2xl px-5 py-4 text-sm focus:bg-white focus:ring-4 focus:ring-amber-100/50 focus:border-amber-300 transition-all outline-none resize-none h-48 placeholder:text-sage-300 shadow-inner"
                    />
                </div>

                {/* Footer */}
                <div className="px-6 py-5 flex justify-end gap-3 bg-white/40 border-t border-sage-100/50 backdrop-blur-md">
                    <button
                        onClick={onClose}
                        className="px-5 py-2.5 text-sm font-bold text-sage-500 hover:bg-white hover:text-sage-700 rounded-xl border border-transparent hover:border-sage-200 transition-all"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleSend}
                        disabled={!content.trim() || sending}
                        className={`px-6 py-2.5 text-sm font-bold text-white rounded-xl flex items-center gap-2 transition-all shadow-lg hover:-translate-y-0.5 ${!content.trim() || sending
                            ? 'bg-sage-300 cursor-not-allowed shadow-none'
                            : 'bg-amber-500 hover:bg-amber-600 shadow-amber-200'
                            }`}
                    >
                        {sending && <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>}
                        <Send size={16} />
                        确定发送
                    </button>
                </div>
            </div>
        </div>
    );
};

export default PrepareClassModal;
