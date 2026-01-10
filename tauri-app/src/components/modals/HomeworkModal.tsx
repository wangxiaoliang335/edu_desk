import { useState } from 'react';
import { X, BookOpen, Calendar, Send } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface HomeworkModalProps {
    isOpen: boolean;
    onClose: () => void;
    onPublish: (data: { subject: string; content: string; date: string }) => void;
}

const SUBJECTS = ["语文", "数学", "英语", "物理", "化学", "生物", "历史", "地理", "政治"];

const HomeworkModal = ({ isOpen, onClose, onPublish }: HomeworkModalProps) => {
    const [subject, setSubject] = useState(SUBJECTS[0]);
    const [content, setContent] = useState("");

    // Default to today
    const today = new Date().toISOString().split('T')[0];
    const [date, setDate] = useState(today);
    const { style, handleMouseDown } = useDraggable();

    if (!isOpen) return null;

    const handleSubmit = () => {
        if (!content.trim()) return;
        onPublish({ subject, content, date });
        onClose();
        setContent(""); // Reset
    };

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-2xl w-[500px] overflow-hidden border border-gray-100 flex flex-col"
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
                <div className="p-6 space-y-5">
                    <div className="grid grid-cols-2 gap-4">
                        <div className="space-y-1.5">
                            <label className="text-xs font-semibold text-gray-500 uppercase tracking-wide">科目</label>
                            <div className="relative">
                                <select
                                    value={subject}
                                    onChange={(e) => setSubject(e.target.value)}
                                    className="w-full text-sm font-medium text-gray-700 bg-gray-50 border border-gray-200 rounded-lg p-2.5 outline-none focus:ring-2 focus:ring-green-500/20 focus:border-green-500 appearance-none"
                                >
                                    {SUBJECTS.map(sub => <option key={sub} value={sub}>{sub}</option>)}
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
                            rows={6}
                            className="w-full text-sm text-gray-700 bg-gray-50 border border-gray-200 rounded-lg p-3 outline-none focus:ring-2 focus:ring-green-500/20 focus:border-green-500 resize-none placeholder:text-gray-400"
                            placeholder="请输入作业的具体内容..."
                        ></textarea>
                    </div>
                </div>

                {/* Footer */}
                <div className="p-4 bg-gray-50 border-t border-gray-100 flex justify-end gap-3">
                    <button
                        onClick={onClose}
                        className="px-4 py-2 text-sm font-medium text-gray-600 hover:bg-gray-200 rounded-lg transition-colors"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleSubmit}
                        disabled={!content.trim()}
                        className={`px-6 py-2 text-sm font-bold text-white rounded-lg shadow-sm flex items-center gap-2 transition-all ${content.trim() ? 'bg-green-600 hover:bg-green-700 hover:shadow-green-200 hover:-translate-y-0.5' : 'bg-gray-300 cursor-not-allowed'}`}
                    >
                        <Send size={16} /> 发布作业
                    </button>
                </div>
            </div>
        </div>
    );
};

export default HomeworkModal;
