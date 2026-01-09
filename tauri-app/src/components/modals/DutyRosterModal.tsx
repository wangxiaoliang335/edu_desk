import { useState } from 'react';
import { X, Calendar, User, Save, Trash2, Plus } from 'lucide-react';

interface DutyRosterModalProps {
    isOpen: boolean;
    onClose: () => void;
}

interface DutyDay {
    day: string; // "周一"
    cleaners: string[]; // ["张三", "李四"]
    supervisor: string; // "王老师"
}

const DutyRosterModal = ({ isOpen, onClose }: DutyRosterModalProps) => {
    // Mock Data
    const [roster, setRoster] = useState<DutyDay[]>([
        { day: '周一', cleaners: ['张三', '李四', '王五'], supervisor: '赵班长' },
        { day: '周二', cleaners: ['赵六', '钱七'], supervisor: '孙学习' },
        { day: '周三', cleaners: ['周八', '吴九', '郑十'], supervisor: '李纪律' },
        { day: '周四', cleaners: ['王十一', '冯十二'], supervisor: '周卫生' },
        { day: '周五', cleaners: ['陈十三', '褚十四', '卫十五'], supervisor: '吴劳动' },
    ]);

    const [editingDayIndex, setEditingDayIndex] = useState<number | null>(null);
    const [tempCleaners, setTempCleaners] = useState<string>('');
    const [tempSupervisor, setTempSupervisor] = useState<string>('');

    if (!isOpen) return null;

    const handleEdit = (index: number) => {
        setEditingDayIndex(index);
        setTempCleaners(roster[index].cleaners.join(' '));
        setTempSupervisor(roster[index].supervisor);
    };

    const handleSave = () => {
        if (editingDayIndex !== null) {
            const newRoster = [...roster];
            newRoster[editingDayIndex] = {
                ...newRoster[editingDayIndex],
                cleaners: tempCleaners.split(' ').filter(n => n.trim() !== ''),
                supervisor: tempSupervisor
            };
            setRoster(newRoster);
            setEditingDayIndex(null);
        }
    };

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-white rounded-2xl shadow-2xl w-[700px] h-[500px] flex flex-col overflow-hidden border border-gray-100">
                {/* Header */}
                <div className="bg-gradient-to-r from-emerald-500 to-teal-600 p-4 flex items-center justify-between text-white flex-shrink-0">
                    <div className="flex items-center gap-2 font-bold text-lg">
                        <Calendar size={20} />
                        <span>班级值日表</span>
                    </div>
                    <button onClick={onClose} className="p-1 hover:bg-white/20 rounded-full transition-colors">
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-y-auto p-6 bg-gray-50/50">
                    <div className="grid gap-4">
                        {roster.map((day, index) => (
                            <div key={day.day} className={`bg-white rounded-xl border p-4 transition-all ${editingDayIndex === index ? 'ring-2 ring-emerald-500 border-emerald-500 shadow-md' : 'border-gray-200 hover:border-emerald-300'}`}>
                                <div className="flex items-center justify-between mb-2">
                                    <div className="flex items-center gap-2">
                                        <span className="bg-emerald-100 text-emerald-700 px-2 py-1 rounded-md text-sm font-bold">{day.day}</span>
                                        {editingDayIndex !== index && (
                                            <span className="text-sm text-gray-500 flex items-center gap-1">
                                                <User size={12} /> 督导: {day.supervisor}
                                            </span>
                                        )}
                                    </div>
                                    {editingDayIndex === index ? (
                                        <div className="flex gap-2">
                                            <button onClick={() => setEditingDayIndex(null)} className="text-xs text-gray-500 hover:text-gray-700 px-2 py-1">取消</button>
                                            <button onClick={handleSave} className="text-xs bg-emerald-500 text-white px-3 py-1 rounded-md hover:bg-emerald-600 flex items-center gap-1">
                                                <Save size={12} /> 保存
                                            </button>
                                        </div>
                                    ) : (
                                        <button onClick={() => handleEdit(index)} className="text-xs text-blue-600 hover:bg-blue-50 px-2 py-1 rounded transition-colors">编辑</button>
                                    )}
                                </div>

                                {editingDayIndex === index ? (
                                    <div className="space-y-2 mt-2">
                                        <div className="flex items-center gap-2">
                                            <label className="text-xs w-10 text-gray-500">值日生:</label>
                                            <input
                                                className="flex-1 text-sm border rounded px-2 py-1 outline-none focus:border-emerald-500"
                                                value={tempCleaners}
                                                onChange={(e) => setTempCleaners(e.target.value)}
                                                placeholder="使用空格分隔姓名"
                                            />
                                        </div>
                                        <div className="flex items-center gap-2">
                                            <label className="text-xs w-10 text-gray-500">督导:</label>
                                            <input
                                                className="flex-1 text-sm border rounded px-2 py-1 outline-none focus:border-emerald-500"
                                                value={tempSupervisor}
                                                onChange={(e) => setTempSupervisor(e.target.value)}
                                            />
                                        </div>
                                    </div>
                                ) : (
                                    <div className="flex flex-wrap gap-2">
                                        {day.cleaners.map((name, i) => (
                                            <span key={i} className="text-sm bg-gray-100 text-gray-700 px-2 py-0.5 rounded-full border border-gray-200">
                                                {name}
                                            </span>
                                        ))}
                                    </div>
                                )}
                            </div>
                        ))}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default DutyRosterModal;
