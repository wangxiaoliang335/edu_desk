import { useState } from 'react';
import { X, Calendar, Download, RefreshCw } from 'lucide-react';

interface CourseScheduleModalProps {
    isOpen: boolean;
    onClose: () => void;
}

// Mock Schedule Data
const SCHEDULE_DATA = [
    { time: '08:00 - 08:45', mon: '语文', tue: '英语', wed: '数学', thu: '语文', fri: '英语' },
    { time: '08:55 - 09:40', mon: '数学', tue: '语文', wed: '英语', thu: '数学', fri: '物理' },
    { time: '09:50 - 10:35', mon: '英语', tue: '物理', wed: '语文', thu: '英语', fri: '化学' },
    { time: '10:50 - 11:35', mon: '物理', tue: '化学', wed: '物理', thu: '化学', fri: '生物' },
    { time: 'lunch', label: '午休' },
    { time: '14:00 - 14:45', mon: '化学', tue: '生物', wed: '化学', thu: '生物', fri: '历史' },
    { time: '14:55 - 15:40', mon: '生物', tue: '历史', wed: '生物', thu: '历史', fri: '地理' },
    { time: '15:50 - 16:35', mon: '历史', tue: '地理', wed: '历史', thu: '地理', fri: '班会' },
    { time: '16:45 - 17:30', mon: '地理', tue: '体育', wed: '体育', thu: '美术', fri: '自习' },
];

const CourseScheduleModal = ({ isOpen, onClose }: CourseScheduleModalProps) => {
    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-white rounded-2xl shadow-2xl w-[900px] h-[650px] flex flex-col overflow-hidden border border-gray-100">
                {/* Header */}
                <div className="bg-gradient-to-r from-blue-500 to-indigo-600 p-4 flex items-center justify-between text-white flex-shrink-0">
                    <div className="flex items-center gap-2 font-bold text-lg">
                        <Calendar size={20} />
                        <span>班级课程表</span>
                        <span className="text-sm font-normal opacity-80 bg-white/20 px-2 py-0.5 rounded ml-2">2023-2024 第二学期</span>
                    </div>
                    <div className="flex items-center gap-2">
                        <button className="p-1.5 hover:bg-white/20 rounded-lg transition-colors text-white/90">
                            <RefreshCw size={18} />
                        </button>
                        <button className="p-1.5 hover:bg-white/20 rounded-lg transition-colors text-white/90">
                            <Download size={18} />
                        </button>
                        <div className="w-[1px] h-6 bg-white/30 mx-1"></div>
                        <button onClick={onClose} className="p-1 hover:bg-white/20 rounded-full transition-colors">
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-auto p-4 bg-gray-50/50">
                    <div className="bg-white rounded-xl border border-gray-200 shadow-sm overflow-hidden">
                        <div className="grid grid-cols-6 text-sm">
                            {/* Header Row */}
                            <div className="bg-gray-50 p-3 font-bold text-gray-500 text-center border-b border-r border-gray-200">时间</div>
                            {['周一', '周二', '周三', '周四', '周五'].map(day => (
                                <div key={day} className="bg-gray-50 p-3 font-bold text-gray-700 text-center border-b border-gray-200 border-r last:border-r-0">
                                    {day}
                                </div>
                            ))}

                            {/* Schedule Rows */}
                            {SCHEDULE_DATA.map((row, index) => {
                                if (row.time === 'lunch') {
                                    return (
                                        <div key={index} className="col-span-6 bg-orange-50/50 p-2 text-center text-orange-400 font-medium text-xs border-b border-gray-200">
                                            {row.label}
                                        </div>
                                    );
                                }
                                return (
                                    <>
                                        {/* Time Column */}
                                        <div className="p-4 flex items-center justify-center text-gray-500 font-medium border-b border-r border-gray-100 text-xs bg-gray-50/30">
                                            {row.time}
                                        </div>
                                        {/* Course Columns */}
                                        {[row.mon, row.tue, row.wed, row.thu, row.fri].map((course, i) => (
                                            <div key={i} className="p-2 border-b border-r border-gray-100 last:border-r-0 h-20 relative group hover:bg-blue-50/50 transition-colors">
                                                <div className={`
                                                    h-full w-full rounded-lg flex flex-col items-center justify-center gap-1 cursor-pointer transition-all hover:shadow-sm
                                                    ${course === '数学' ? 'bg-blue-100 text-blue-700' :
                                                        course === '语文' ? 'bg-red-100 text-red-700' :
                                                            course === '英语' ? 'bg-orange-100 text-orange-700' :
                                                                course === '物理' ? 'bg-indigo-100 text-indigo-700' :
                                                                    course === '化学' ? 'bg-purple-100 text-purple-700' :
                                                                        course === '体育' ? 'bg-green-100 text-green-700' :
                                                                            'bg-gray-100 text-gray-600'}
                                                `}>
                                                    <span className="font-bold">{course}</span>
                                                    <span className="text-[10px] opacity-70">A-302</span>
                                                </div>
                                            </div>
                                        ))}
                                    </>
                                );
                            })}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default CourseScheduleModal;
