import { useState, useEffect } from 'react';
import { X, BookOpen, Send, Clock, Trash2 } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessageWS } from '../../utils/websocket';

interface HomeworkModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
    groupId?: string;
    groupName?: string;
    teachSubjects?: string[]; // Teacher's subjects from group members API
    onPublished?: (latest: PublishedHomework, all: PublishedHomework[]) => void;
}

export interface PublishedHomework {
    id: string;
    subject: string;
    content: string;
    date: string;
    publishedAt: string;
}


// Store published homework per class in localStorage
const getStorageKey = (classId: string) => `homework_history_${classId}`;

const loadPublishedHomework = (classId: string): PublishedHomework[] => {
    try {
        const data = localStorage.getItem(getStorageKey(classId));
        return data ? JSON.parse(data) : [];
    } catch {
        return [];
    }
};

const savePublishedHomework = (classId: string, homework: PublishedHomework[]) => {
    localStorage.setItem(getStorageKey(classId), JSON.stringify(homework));
};

const HomeworkModal = ({ isOpen, onClose, classId, groupId, groupName, teachSubjects, onPublished }: HomeworkModalProps) => {
    const [subject, setSubject] = useState("");
    const [content, setContent] = useState("");
    const [sending, setSending] = useState(false);
    const [publishedList, setPublishedList] = useState<PublishedHomework[]>([]);

    // Use teacher's subjects from Class Info; if empty, disable publishing
    const subjects = (teachSubjects && teachSubjects.length > 0) ? teachSubjects : [];

    // Default to today
    const today = new Date().toISOString().split('T')[0];
    const [date, setDate] = useState(today);
    const { style, handleMouseDown } = useDraggable();

    // Load published homework when modal opens
    useEffect(() => {
        if (isOpen && classId) {
            setPublishedList(loadPublishedHomework(classId));
        }
    }, [isOpen, classId]);

    // Set default subject when modal opens
    useEffect(() => {
        if (isOpen && subjects.length > 0 && !subject) {
            setSubject(subjects[0]);
        }
    }, [isOpen, subjects]);

    // Listen for WebSocket homework responses
    useEffect(() => {
        if (!isOpen) return;

        const handleWSMessage = (e: CustomEvent) => {
            try {
                const data = JSON.parse(e.detail);
                if (data.type === 'homework' && data.status === 'success') {
                    console.log('[HomeworkModal] Homework published successfully:', data.message);
                }
            } catch (err) {
                // Not JSON or not for us
            }
        };

        window.addEventListener('ws-message', handleWSMessage as EventListener);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage as EventListener);
        };
    }, [isOpen]);

    if (!isOpen) return null;

    const handleSubmit = async () => {
        if (!content.trim()) {
            alert("请输入作业内容");
            return;
        }
        if (!subject) {
            alert("请先在班级信息中设置任教科目");
            return;
        }
        if (!classId) {
            alert("班级ID缺失，无法发布作业");
            return;
        }
        if (!groupId) {
            alert("群组ID缺失，无法发布作业");
            return;
        }

        setSending(true);

        try {
            // Get user info
            const userInfoStr = localStorage.getItem('user_info');
            let senderName = "老师";
            let senderId = "";
            let schoolId = "";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    senderName = u.name || u.strName || u.data?.name || "老师";
                    senderId = u.teacher_unique_id || u.data?.teacher_unique_id || "";
                    schoolId = u.school_id || u.data?.school_id || u.schoolId || "";
                } catch (e) { }
            }

            // Build homework message
            const homeworkObj: Record<string, string> = {
                type: "homework",
                class_id: classId,
                subject: subject,
                content: content.trim(),
                date: date,
                sender_name: senderName,
            };

            if (groupId) {
                homeworkObj.group_id = groupId;
            }
            if (groupName) {
                homeworkObj.group_name = groupName;
            }
            if (senderId) {
                homeworkObj.sender_id = senderId;
            }
            if (schoolId) {
                homeworkObj.school_id = schoolId;
            }

            // Format: to:<group_id>:<JSON> (recommended format)
            const wsMessage = `to:${groupId}:${JSON.stringify(homeworkObj)}`;

            console.log('[HomeworkModal] Sending:', wsMessage);
            sendMessageWS(wsMessage);

            // Save to published list
            const newHomework: PublishedHomework = {
                id: Date.now().toString(),
                subject,
                content: content.trim(),
                date,
                publishedAt: new Date().toLocaleString()
            };
            const updatedList = [newHomework, ...publishedList];
            setPublishedList(updatedList);
            savePublishedHomework(classId, updatedList);
            if (onPublished) onPublished(newHomework, updatedList);

            // Reset form but keep modal open
            setContent("");
        } catch (e) {
            console.error("Failed to publish homework", e);
            alert("发布失败，请重试");
        } finally {
            setSending(false);
        }
    };

    const handleDeleteHomework = (id: string) => {
        if (!classId) return;
        const updated = publishedList.filter(h => h.id !== id);
        setPublishedList(updated);
        savePublishedHomework(classId, updated);
    };

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-2xl w-[550px] max-h-[90vh] overflow-hidden border border-gray-100 flex flex-col"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-gradient-to-r from-green-500 to-emerald-600 p-4 flex items-center justify-between text-white cursor-move select-none"
                >
                    <div className="flex items-center gap-2 font-bold text-lg">
                        <BookOpen size={20} />
                        <span>发布作业</span>
                    </div>
                    <button onClick={onClose} className="p-1 hover:bg-white/20 rounded-full transition-colors">
                        <X size={20} />
                    </button>
                </div>

                {/* Form */}
                <div className="p-5 space-y-4">
                    <div className="grid grid-cols-2 gap-4">
                        <div className="space-y-1.5">
                            <label className="text-xs font-semibold text-gray-500 uppercase tracking-wide">科目</label>
                            <div className="relative">
                                <select
                                    value={subject}
                                    onChange={(e) => setSubject(e.target.value)}
                                    disabled={subjects.length === 0}
                                    className={`w-full text-sm font-medium rounded-lg p-2.5 outline-none appearance-none ${
                                        subjects.length === 0
                                            ? 'bg-gray-100 text-gray-400 border border-gray-200 cursor-not-allowed'
                                            : 'text-gray-700 bg-gray-50 border border-gray-200 focus:ring-2 focus:ring-green-500/20 focus:border-green-500'
                                    }`}
                                >
                                    {subjects.length === 0 ? (
                                        <option value="">请先在班级信息中添加任教科目</option>
                                    ) : (
                                        subjects.map(sub => <option key={sub} value={sub}>{sub}</option>)
                                    )}
                                </select>
                                <div className="absolute right-3 top-3 pointer-events-none text-gray-400">▼</div>
                            </div>
                        </div>
                        <div className="space-y-1.5">
                            <label className="text-xs font-semibold text-gray-500 uppercase tracking-wide">日期</label>
                            <input
                                type="date"
                                value={date}
                                onChange={(e) => setDate(e.target.value)}
                                className="w-full text-sm font-medium text-gray-700 bg-gray-50 border border-gray-200 rounded-lg p-2.5 outline-none focus:ring-2 focus:ring-green-500/20 focus:border-green-500"
                            />
                        </div>
                    </div>

                    <div className="space-y-1.5">
                        <label className="text-xs font-semibold text-gray-500 uppercase tracking-wide">作业内容</label>
                        <textarea
                            value={content}
                            onChange={(e) => setContent(e.target.value)}
                            rows={4}
                            className="w-full text-sm text-gray-700 bg-gray-50 border border-gray-200 rounded-lg p-3 outline-none focus:ring-2 focus:ring-green-500/20 focus:border-green-500 resize-none placeholder:text-gray-400"
                            placeholder="请输入作业的具体内容..."
                        ></textarea>
                    </div>

                    <div className="flex justify-end">
                        <button
                            onClick={handleSubmit}
                            disabled={!content.trim() || sending}
                            className={`px-5 py-2 text-sm font-bold text-white rounded-lg shadow-sm flex items-center gap-2 transition-all ${content.trim() && !sending ? 'bg-green-600 hover:bg-green-700 hover:shadow-green-200 hover:-translate-y-0.5' : 'bg-gray-300 cursor-not-allowed'}`}
                        >
                            {sending ? (
                                <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                            ) : (
                                <Send size={16} />
                            )}
                            发布作业
                        </button>
                    </div>
                </div>

                {/* Published Homework List */}
                {publishedList.length > 0 && (
                    <div className="border-t border-gray-100 bg-gray-50/50 p-4 max-h-[200px] overflow-y-auto">
                        <h4 className="text-xs font-bold text-gray-500 uppercase tracking-wide mb-3">已发布的作业</h4>
                        <div className="space-y-2">
                            {publishedList.map(hw => (
                                <div key={hw.id} className="bg-white p-3 rounded-lg border border-gray-100 shadow-sm group relative">
                                    <div className="flex items-start justify-between">
                                        <div className="flex-1 min-w-0">
                                            <div className="flex items-center gap-2 mb-1">
                                                <span className="px-2 py-0.5 bg-green-100 text-green-700 text-xs font-bold rounded">{hw.subject}</span>
                                                <span className="text-xs text-gray-400">{hw.date}</span>
                                            </div>
                                            <p className="text-sm text-gray-700 line-clamp-2">{hw.content}</p>
                                            <div className="text-[10px] text-gray-400 mt-1 flex items-center gap-1">
                                                <Clock size={10} />
                                                {hw.publishedAt}
                                            </div>
                                        </div>
                                        <button
                                            onClick={() => handleDeleteHomework(hw.id)}
                                            className="p-1 text-gray-300 hover:text-red-500 opacity-0 group-hover:opacity-100 transition-all"
                                            title="删除"
                                        >
                                            <Trash2 size={14} />
                                        </button>
                                    </div>
                                </div>
                            ))}
                        </div>
                    </div>
                )}

                {/* Footer */}
                <div className="p-3 bg-gray-50 border-t border-gray-100 flex justify-end">
                    <button
                        onClick={onClose}
                        className="px-4 py-2 text-sm font-medium text-gray-600 hover:bg-gray-200 rounded-lg transition-colors"
                    >
                        关闭
                    </button>
                </div>
            </div>
        </div>
    );
};

export default HomeworkModal;
