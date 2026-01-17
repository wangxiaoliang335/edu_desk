import { useState } from 'react';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';
import { Folder, GraduationCap, Calendar, User } from 'lucide-react';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

// Define Dock Items types - reusing same types for compatibility
export type TaskbarItemType = 'folder' | 'class' | 'schedule' | 'user';

interface TaskbarProps {
    activeItem: TaskbarItemType | null;
    onItemClick: (item: TaskbarItemType) => void;
}

const Taskbar = ({ activeItem, onItemClick }: TaskbarProps) => {
    const [hoveredItem, setHoveredItem] = useState<TaskbarItemType | null>(null);

    const taskbarItems = [
        { id: 'folder', label: '资源云盘', icon: Folder, color: 'text-blue-600' },
        { id: 'class', label: '班级管理', icon: GraduationCap, color: 'text-emerald-600' },
        { id: 'schedule', label: '课程表', icon: Calendar, color: 'text-orange-600' },
        { id: 'user', label: '个人中心', icon: User, color: 'text-purple-600' },
    ] as const;

    return (
        <div className="fixed bottom-6 left-0 right-0 z-50 flex justify-center pointer-events-none">
            {/* Organic Floating Dock */}
            <div className="pointer-events-auto h-16 px-4 bg-white/90 backdrop-blur-2xl border border-sage-200 shadow-[0_10px_40px_-10px_rgba(0,0,0,0.1)] rounded-full flex items-center gap-3 transition-all duration-300">

                {/* Start Menu Placeholder */}
                <div className="w-12 h-12 flex items-center justify-center rounded-full hover:bg-sage-50 transition-colors cursor-pointer group active:scale-95">
                    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" className="group-hover:opacity-80 transition-opacity">
                        <path d="M4 4H11V11H4V4Z" fill="#6b9074" />
                        <path d="M4 13H11V20H4V13Z" fill="#6b9074" />
                        <path d="M13 4H20V11H13V4Z" fill="#6b9074" />
                        <path d="M13 13H20V20H13V13Z" fill="#6b9074" />
                    </svg>
                </div>

                <div className="w-[1px] h-8 bg-sage-200 mx-2"></div>

                {taskbarItems.map((item) => {
                    const Icon = item.icon;
                    const isActive = activeItem === item.id;
                    const isHovered = hoveredItem === item.id;

                    return (
                        <button
                            key={item.id}
                            onClick={() => onItemClick(item.id as TaskbarItemType)}
                            onMouseEnter={() => setHoveredItem(item.id as TaskbarItemType)}
                            onMouseLeave={() => setHoveredItem(null)}
                            className={cn(
                                "group relative w-12 h-12 flex items-center justify-center rounded-2xl transition-all duration-300",
                                isActive ? "bg-sage-100 text-sage-700" : "hover:bg-sage-50 text-ink-400"
                            )}
                            title={item.label}
                        >
                            {/* Icon */}
                            <Icon
                                size={24}
                                className={cn(
                                    "transition-transform duration-300",
                                    isActive ? "scale-110" : "group-hover:scale-110",
                                    isActive ? "text-sage-700" : "text-ink-500"
                                )}
                                strokeWidth={isActive ? 2.5 : 2}
                            />

                            {/* Tooltip (Simple) */}
                            {isHovered && !isActive && (
                                <div className="absolute -top-10 bg-ink-800 text-white text-xs px-2 py-1 rounded-md opacity-0 animate-in fade-in zoom-in-95 duration-200 pointer-events-none whitespace-nowrap">
                                    {item.label}
                                </div>
                            )}
                        </button>
                    );
                })}
            </div>
        </div>
    );
};

export default Taskbar;
