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
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl w-[550px] max-h-[90vh] overflow-hidden border border-white/50 ring-1 ring-sage-100/50 flex flex-col"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-white/30 px-8 py-5 flex items-center justify-between flex-shrink-0 cursor-move border-b border-sage-100/50 backdrop-blur-md relative z-20"
                >
                    <div className="flex items-center gap-4">
                        <div className="w-12 h-12 bg-gradient-to-br from-sage-400 to-sage-600 text-white rounded-2xl flex items-center justify-center shadow-lg shadow-sage-500/20">
                            <BookOpen size={24} />
                        </div>
                        <div>
                            <h2 className="text-2xl font-bold text-ink-800 tracking-tight">发布作业</h2>
                            <p className="text-sm font-medium text-ink-400 mt-0.5 tracking-wide">Publish Homework</p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-10 h-10 flex items-center justify-center hover:bg-clay-50 text-sage-400 hover:text-clay-600 rounded-full transition-all duration-300"
                    >
                        <X size={24} />
                    </button>
                </div>

                {/* Form */}
                <div className="p-8 space-y-6 bg-white/40 overflow-y-auto custom-scrollbar">
                    <div className="grid grid-cols-2 gap-6">
                        <div className="space-y-2">
                            <label className="text-xs font-bold text-ink-500 uppercase tracking-wide px-1">科目</label>
                            <div className="relative group">
                                <select
                                    value={subject}
                                    onChange={(e) => setSubject(e.target.value)}
                                    disabled={subjects.length === 0}
                                    className={`w-full text-sm font-bold rounded-2xl p-4 outline-none appearance-none transition-all shadow-sm ${subjects.length === 0
                                            ? 'bg-gray-100 text-gray-400 border border-gray-200 cursor-not-allowed'
                                            : 'text-ink-800 bg-white border border-sage-200 focus:ring-4 focus:ring-sage-100 focus:border-sage-400 hover:border-sage-300'
                                        }`}
                                >
                                    {subjects.length === 0 ? (
                                        <option value="">请先在班级信息中添加任教科目</option>
                                    ) : (
                                        subjects.map(sub => <option key={sub} value={sub}>{sub}</option>)
                                    )}
                                </select>
                                <div className="absolute right-4 top-1/2 -translate-y-1/2 pointer-events-none text-sage-400 group-hover:text-sage-600 transition-colors">▼</div>
                            </div>
                        </div>
                        <div className="space-y-2">
                            <label className="text-xs font-bold text-ink-500 uppercase tracking-wide px-1">日期</label>
                            <input
                                type="date"
                                value={date}
                                onChange={(e) => setDate(e.target.value)}
                                className="w-full text-sm font-bold text-ink-800 bg-white border border-sage-200 rounded-2xl p-4 outline-none focus:ring-4 focus:ring-sage-100 focus:border-sage-400 hover:border-sage-300 transition-all shadow-sm"
                            />
                        </div>
                    </div>

                    <div className="space-y-2">
                        <label className="text-xs font-bold text-ink-500 uppercase tracking-wide px-1">作业内容</label>
                        <textarea
                            value={content}
                            onChange={(e) => setContent(e.target.value)}
                            rows={4}
                            className="w-full text-sm text-ink-700 bg-white border border-sage-200 rounded-2xl p-4 outline-none focus:ring-4 focus:ring-sage-100 focus:border-sage-400 resize-none placeholder:text-sage-300 transition-all shadow-inner leading-relaxed"
                            placeholder="请输入作业的具体内容..."
                        ></textarea>
                    </div>

                    <div className="flex justify-end pt-2">
                        <button
                            onClick={handleSubmit}
                            disabled={!content.trim() || sending}
                            className={`px-8 py-3 text-sm font-bold text-white rounded-2xl shadow-lg flex items-center gap-2 transition-all active:scale-95 transform ${content.trim() && !sending ? 'bg-sage-600 hover:bg-sage-700 shadow-sage-500/20 hover:-translate-y-0.5' : 'bg-gray-300 cursor-not-allowed'}`}
                        >
                            {sending ? (
                                <div className="w-5 h-5 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                            ) : (
                                <Send size={18} />
                            )}
                            发布作业
                        </button>
                    </div>

                    {/* Published Homework List */}
                    {publishedList.length > 0 && (
                        <div className="border-t border-sage-100/50 pt-6 mt-2">
                            <h4 className="text-xs font-bold text-ink-400 uppercase tracking-wide mb-4 px-1 flex items-center gap-2">
                                <Clock size={12} />
                                已发布的作业
                            </h4>
                            <div className="space-y-3 max-h-[220px] overflow-y-auto custom-scrollbar pr-1">
                                {publishedList.map(hw => (
                                    <div key={hw.id} className="bg-white/80 backdrop-blur-sm p-4 rounded-2xl border border-white/60 shadow-sm group relative hover:border-sage-200 transition-all hover:shadow-md ring-1 ring-sage-50/50">
                                        <div className="flex items-start justify-between gap-3">
                                            <div className="flex-1 min-w-0">
                                                <div className="flex items-center gap-2 mb-1.5">
                                                    <span className="px-2.5 py-0.5 bg-sage-100 text-sage-700 text-[10px] font-bold rounded-full border border-sage-200">{hw.subject}</span>
                                                    <span className="text-[10px] font-bold text-ink-400 bg-white px-2 py-0.5 rounded-full border border-sage-100">{hw.date}</span>
                                                </div>
                                                <p className="text-sm font-medium text-ink-700 line-clamp-2 leading-relaxed">{hw.content}</p>
                                                <div className="text-[10px] text-ink-400 mt-2 flex items-center gap-1 font-mono opacity-80">
                                                    <Clock size={10} />
                                                    {hw.publishedAt}
                                                </div>
                                            </div>
                                            <button
                                                onClick={() => handleDeleteHomework(hw.id)}
                                                className="p-2 text-sage-300 hover:text-clay-600 hover:bg-clay-50 rounded-lg opacity-0 group-hover:opacity-100 transition-all flex-shrink-0"
                                                title="删除"
                                            >
                                                <Trash2 size={16} />
                                            </button>
                                        </div>
                                    </div>
                                ))}
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default HomeworkModal;
