import { X, Bell, User, Users, Clock } from "lucide-react";
import { useState } from "react";

export interface NotificationItem {
    id: number;
    sender_id: number;
    sender_name: string;
    receiver_id: string;
    unique_group_id: string; // Group ID
    group_name: string;
    content: string;
    content_text: number; // 3 seems to be Invite?
    is_read: number;
    is_agreed: number; // 0=Pending, 1=Agreed, 2=Rejected
    remark?: string;
    created_at: string;
    updated_at: string;
}

interface Props {
    isOpen: boolean;
    onClose: () => void;
    notifications: NotificationItem[];
}

const NotificationCenterModal = ({ isOpen, onClose, notifications }: Props) => {
    if (!isOpen) return null;

    // Filter to show pending items at top
    const sorted = [...notifications].sort((a, b) => {
        // Sort by date desc
        return new Date(b.created_at).getTime() - new Date(a.created_at).getTime();
    });

    return (
        <div className="fixed inset-0 z-[100] flex items-start justify-end p-4 pt-16 bg-transparent pointer-events-none font-sans">
            {/* Modal Container - Pointer events auto to capture clicks */}
            <div className="bg-paper/95 backdrop-blur-xl pointer-events-auto rounded-[2rem] shadow-2xl w-[380px] max-h-[600px] flex flex-col border border-white/50 ring-1 ring-sage-100/50 overflow-hidden animate-in slide-in-from-right-5 duration-300">
                {/* Header */}
                <div className="px-6 py-4 border-b border-sage-100/50 bg-white/30 backdrop-blur-md flex items-center justify-between">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 bg-gradient-to-br from-red-400 to-red-500 text-white rounded-xl flex items-center justify-center shadow-lg shadow-red-500/20">
                            <Bell size={18} />
                        </div>
                        <div>
                            <h3 className="font-bold text-ink-800 text-lg">消息中心</h3>
                            <span className="text-xs font-bold text-sage-500 bg-sage-50 px-2 py-0.5 rounded-full border border-sage-100">{sorted.length} 条通知</span>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-8 h-8 flex items-center justify-center hover:bg-clay-50 rounded-full text-sage-400 hover:text-clay-600 transition-colors"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-y-auto p-4 bg-white/30 custom-scrollbar">
                    {sorted.length === 0 ? (
                        <div className="h-64 flex flex-col items-center justify-center text-sage-300 gap-3">
                            <div className="w-16 h-16 rounded-full bg-sage-50 flex items-center justify-center border border-sage-100">
                                <Bell size={32} className="opacity-50" />
                            </div>
                            <span className="text-sm font-bold">暂无新消息</span>
                        </div>
                    ) : (
                        <div className="space-y-3">
                            {sorted.map(item => (
                                <div key={item.id} className="bg-white/80 backdrop-blur-md p-4 rounded-2xl border border-white/60 shadow-sm hover:shadow-md transition-all group hover:border-sage-200">
                                    <div className="flex items-start justify-between gap-4">
                                        {/* Icon */}
                                        <div className="mt-1">
                                            {item.content_text === 3 ? ( // 3 = Group Invite
                                                <div className="w-10 h-10 bg-indigo-50 text-indigo-500 rounded-xl flex items-center justify-center border border-indigo-100 shadow-sm">
                                                    <Users size={20} />
                                                </div>
                                            ) : (
                                                <div className="w-10 h-10 bg-sage-50 text-sage-600 rounded-xl flex items-center justify-center border border-sage-100 shadow-sm">
                                                    <User size={20} />
                                                </div>
                                            )}
                                        </div>

                                        {/* Content */}
                                        <div className="flex-1 min-w-0">
                                            <div className="flex items-center justify-between mb-1">
                                                <span className="font-bold text-ink-800 text-sm truncate pr-2">
                                                    {/* Display Group Name if available, else Sender */}
                                                    {item.group_name || item.sender_name}
                                                </span>
                                                <span className="text-[10px] text-ink-400 font-medium flex-shrink-0 flex items-center gap-1 bg-white/50 px-1.5 py-0.5 rounded-md border border-black/5">
                                                    <Clock size={10} />
                                                    {/* Format date: Remove seconds? Or just show time if today? For now split space */}
                                                    {item.created_at.split(' ')[0]}
                                                </span>
                                            </div>

                                            <p className="text-xs text-ink-500 mb-2 leading-relaxed bg-sage-50/50 p-2 rounded-lg border border-sage-50">
                                                <span className="font-bold text-ink-700 mr-1">{item.sender_name}</span>
                                                {item.content}
                                            </p>

                                            {/* Action Buttons Removed - Default Agreed */}
                                        </div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default NotificationCenterModal;
