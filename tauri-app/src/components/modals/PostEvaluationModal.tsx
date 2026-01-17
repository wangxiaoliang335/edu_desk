import { useState } from 'react';
import { X, ClipboardCheck, Send } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessageWS } from '../../utils/websocket';

interface Props {
    isOpen: boolean;
    onClose: () => void;
    subject: string;
    classId?: string;
    groupId?: string;
}

const PostEvaluationModal = ({ isOpen, onClose, subject, classId, groupId }: Props) => {
    const { style, handleMouseDown } = useDraggable();
    const [content, setContent] = useState("");
    const [sending, setSending] = useState(false);

    const handleSend = async () => {
        if (!content.trim()) {
            alert("请输入课后评价内容");
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

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    senderName = u.name || u.strName || u.data?.name || "老师";
                    senderId = u.teacher_unique_id || u.data?.teacher_unique_id || "";
                } catch (e) { }
            }

            const messageObj: Record<string, string> = {
                type: "post_class_evaluation",
                class_id: classId,
                subject: subject,
                content: content.trim(),
                date: new Date().toISOString().split('T')[0],
                sender_name: senderName,
            };

            if (senderId) messageObj.sender_id = senderId;

            const wsMessage = `to:${groupId}:${JSON.stringify(messageObj)}`;

            console.log('[PostEvaluationModal] Sending:', wsMessage);
            sendMessageWS(wsMessage);

            setContent("");
            onClose();
        } catch (e) {
            console.error("Failed to send post-class evaluation", e);
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
                        <div className="w-10 h-10 rounded-2xl bg-gradient-to-br from-emerald-400 to-emerald-600 text-white flex items-center justify-center shadow-lg shadow-emerald-500/20">
                            <ClipboardCheck size={20} />
                        </div>
                        <span className="font-bold text-ink-800 text-lg tracking-tight">课后评价</span>
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
                        <span className="px-3 py-1 bg-emerald-100 text-emerald-700 rounded-lg font-bold text-xs shadow-sm border border-emerald-200">
                            {subject}
                        </span>
                        <span className="text-xs text-sage-400 font-medium">请填写本节课的教学评价</span>
                    </div>
                </div>

                {/* Content */}
                <div className="px-6 py-3 bg-white/30 backdrop-blur-sm flex-1">
                    <textarea
                        value={content}
                        onChange={e => setContent(e.target.value)}
                        placeholder="请输入课后评价内容，例如：课堂纪律良好，同学们积极发言..."
                        className="w-full bg-white/60 text-ink-700 border border-sage-200 rounded-2xl px-5 py-4 text-sm focus:bg-white focus:ring-2 focus:ring-emerald-100 focus:border-emerald-400 transition-all outline-none resize-none h-48 placeholder:text-sage-300 shadow-inner"
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
                            : 'bg-emerald-500 hover:bg-emerald-600 shadow-emerald-200'
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

export default PostEvaluationModal;
