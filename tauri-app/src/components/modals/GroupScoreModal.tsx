import { useState } from 'react';
import { X, Users, Award, TrendingUp, Plus, Minus } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface GroupScoreModalProps {
    isOpen: boolean;
    onClose: () => void;
}

interface Group {
    id: number;
    name: string;
    score: number;
    members: string[]; // ["Name1", "Name2"]
}

const GroupScoreModal = ({ isOpen, onClose }: GroupScoreModalProps) => {
    // Mock Data
    const [groups, setGroups] = useState<Group[]>([
        { id: 1, name: '第一组', score: 85, members: ['张三', '李四', '王五', '赵六', '钱七', '孙八'] },
        { id: 2, name: '第二组', score: 92, members: ['周九', '吴十', '郑十一', '王十二', '冯十三', '陈十四'] },
        { id: 3, name: '第三组', score: 78, members: ['褚十五', '卫十六', '蒋十七', '沈十八', '韩十九', '杨二十'] },
        { id: 4, name: '第四组', score: 88, members: ['朱二一', '秦二二', '尤二三', '许二四', '何二五', '吕二六'] },
        { id: 5, name: '第五组', score: 95, members: ['施二七', '张二八', '孔二九', '曹三十', '严三一', '华三二'] },
        { id: 6, name: '第六组', score: 82, members: ['金三三', '魏三四', '陶三五', '姜三六', '戚三七', '谢三八'] },
    ]);
    const { style, handleMouseDown } = useDraggable();

    const handleScoreChange = (id: number, delta: number) => {
        setGroups(groups.map(g => {
            if (g.id === id) {
                return { ...g, score: g.score + delta };
            }
            return g;
        }));
    };

    if (!isOpen) return null;

    return (

        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div
                style={style}
                className="bg-paper/95 backdrop-blur-xl rounded-[2.5rem] shadow-2xl w-[900px] h-[600px] flex flex-col overflow-hidden border border-white/60 ring-1 ring-sage-100/50 animate-in zoom-in-95 duration-200"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="p-5 flex items-center justify-between border-b border-sage-100/50 bg-white/40 backdrop-blur-md cursor-move select-none"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-2xl bg-gradient-to-br from-violet-400 to-violet-600 text-white flex items-center justify-center shadow-lg shadow-violet-500/20">
                            <Award size={20} />
                        </div>
                        <span className="font-bold text-ink-800 text-lg tracking-tight">小组评价</span>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-9 h-9 flex items-center justify-center rounded-full text-sage-400 hover:text-clay-600 hover:bg-clay-50 transition-all duration-300"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-y-auto p-6 bg-white/30 custom-scrollbar">

                    {/* Top Stats / Controls */}
                    <div className="mb-6 flex items-center justify-between">
                        <div className="flex items-center gap-2 text-sage-500 text-sm font-bold bg-white/50 px-4 py-2 rounded-xl backdrop-blur-sm border border-white/50">
                            <Users size={16} /> 共 {groups.length} 个小组
                        </div>
                        <button className="text-xs font-bold bg-white text-ink-600 border border-sage-200 hover:border-sage-400 hover:text-sage-700 px-4 py-2 rounded-xl shadow-sm transition-all hover:shadow-md">
                            重置所有分数
                        </button>
                    </div>

                    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
                        {groups.map((group) => (
                            <div key={group.id} className="bg-white/80 backdrop-blur-sm rounded-2xl border border-white shadow-sm flex flex-col overflow-hidden group hover:shadow-lg hover:scale-[1.02] transition-all duration-300 ring-1 ring-black/5 hover:ring-violet-200">
                                {/* Group Header */}
                                <div className="p-4 border-b border-sage-50 flex items-center justify-between bg-gradient-to-br from-white to-sage-50/30">
                                    <div className="font-bold text-ink-800 flex items-center gap-3">
                                        <div className="w-9 h-9 rounded-xl bg-violet-100 text-violet-600 flex items-center justify-center font-bold text-sm shadow-inner group-hover:bg-violet-500 group-hover:text-white transition-colors duration-300">
                                            {group.id}
                                        </div>
                                        {group.name}
                                    </div>
                                    <div className="flex items-center gap-1 text-3xl font-black text-violet-500 drop-shadow-sm">
                                        {group.score}
                                        <span className="text-xs font-bold text-sage-400 mt-3">分</span>
                                    </div>
                                </div>

                                {/* Members */}
                                <div className="p-4 flex-1 bg-white/40">
                                    <div className="flex flex-wrap gap-2">
                                        {group.members.map((m, i) => (
                                            <span key={i} className="text-xs bg-white border border-sage-100 text-ink-600 px-2.5 py-1 rounded-lg font-medium text-center shadow-sm">
                                                {m}
                                            </span>
                                        ))}
                                    </div>
                                </div>

                                {/* Actions */}
                                <div className="p-3 bg-sage-50/50 border-t border-sage-100/50 flex items-center justify-between gap-3 backdrop-blur-md">
                                    <button
                                        onClick={() => handleScoreChange(group.id, -1)}
                                        className="flex-1 py-2 rounded-xl border border-clay-200 text-clay-600 hover:bg-clay-50 hover:border-clay-300 flex items-center justify-center gap-1.5 transition-all font-bold text-xs bg-white hover:shadow-sm"
                                    >
                                        <Minus size={14} strokeWidth={2.5} /> 扣分
                                    </button>
                                    <button
                                        onClick={() => handleScoreChange(group.id, 1)}
                                        className="flex-1 py-2 rounded-xl bg-violet-600 text-white hover:bg-violet-700 shadow-lg shadow-violet-200 flex items-center justify-center gap-1.5 transition-all font-bold text-xs hover:-translate-y-0.5"
                                    >
                                        <Plus size={14} strokeWidth={2.5} /> 加分
                                    </button>
                                </div>
                            </div>
                        ))}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default GroupScoreModal;
