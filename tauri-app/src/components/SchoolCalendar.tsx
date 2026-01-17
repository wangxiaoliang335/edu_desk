import { useState, useEffect, useRef } from 'react';
import Calendar from 'react-calendar';
import { format } from 'date-fns';
import { zhCN } from 'date-fns/locale';
import {
    SchoolEvent,
    loadEvents,
    saveEvents,
    getEventsForDate,
    isHoliday,
    formatDateKey
} from '../utils/schoolEvents';
import * as XLSX from 'xlsx';
import '../styles/calendar.css';
import { useDraggable } from '../hooks/useDraggable';
import { Upload, Save, FileText } from 'lucide-react';

interface SchoolCalendarProps {
    onClose?: () => void;
}

const SchoolCalendar = ({ onClose }: SchoolCalendarProps) => {
    const [date, setDate] = useState<Date>(new Date());
    const [events, setEvents] = useState<SchoolEvent[]>([]);
    const [remarkText, setRemarkText] = useState('');
    const draggable = useDraggable();
    const fileInputRef = useRef<HTMLInputElement>(null);

    useEffect(() => {
        const loaded = loadEvents();
        setEvents(loaded);
    }, []);

    useEffect(() => {
        // Load remark for current date when date selection changes
        const currentEvents = getEventsForDate(date, events);
        const remarkEvent = currentEvents.find(e => e.type === 'remark');
        setRemarkText(remarkEvent ? remarkEvent.title : '');
    }, [date, events]);

    const handleDateChange = (value: any) => {
        if (value instanceof Date) {
            setDate(value);
        }
    };

    const handleRemarkChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
        setRemarkText(e.target.value);
    };

    const saveRemark = () => {
        const dateKey = formatDateKey(date);
        const newEvents = events.filter(e => !(e.date === dateKey && e.type === 'remark'));

        if (remarkText.trim()) {
            newEvents.push({
                id: `remark-${dateKey}-${Date.now()}`,
                date: dateKey,
                title: remarkText,
                type: 'remark',
                description: '备注'
            });
        }

        setEvents(newEvents);
        saveEvents(newEvents);
        // Optional: Show toast
    };

    const handleImportClick = () => {
        fileInputRef.current?.click();
    };

    const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (evt) => {
            try {
                const bstr = evt.target?.result;
                const wb = XLSX.read(bstr, { type: 'binary' });
                const wsname = wb.SheetNames[0];
                const ws = wb.Sheets[wsname];
                const data = XLSX.utils.sheet_to_json<any>(ws);

                // Expected format: { date: '2026-01-01', content: 'Holiday', type: 'holiday' }
                // Or simplified: Just 'Date' and 'Content' columns
                const importedEvents: SchoolEvent[] = [];

                data.forEach((row: any) => {
                    // Try to parse date
                    let dateStr = row['日期'] || row['Date'] || row['date'];
                    const content = row['内容'] || row['Content'] || row['content'] || row['Remark'];
                    let type = row['类型'] || row['Type'] || row['type'] || 'activity';

                    if (dateStr && content) {
                        // Basic cleanup or parsing if needed. Assuming YYYY-MM-DD or readable format
                        // Excel dates might be numbers, need handling if raw values

                        // For simplicity, assuming user provides string 'YYYY-MM-DD'
                        // If Excel date number, conversion needed: 
                        // new Date(Math.round((excelDate - 25569)*86400*1000))

                        importedEvents.push({
                            id: `import-${Date.now()}-${Math.random()}`,
                            date: String(dateStr).trim(),
                            title: String(content).trim(),
                            type: type as any
                        });
                    }
                });

                if (importedEvents.length > 0) {
                    const newEventList = [...events, ...importedEvents];
                    setEvents(newEventList);
                    saveEvents(newEventList);
                    alert(`成功导入 ${importedEvents.length} 条记录`);
                } else {
                    alert('未解析到有效数据，请检查 Excel 格式 (列名: 日期, 内容)');
                }

            } catch (err) {
                console.error('Import failed', err);
                alert('导入失败，请检查文件格式');
            }
        };
        reader.readAsBinaryString(file);

        // Reset input
        e.target.value = '';
    };

    // Custom rendering for calendar tiles (days)
    const tileContent = ({ date, view }: { date: Date; view: string }) => {
        if (view === 'month') {
            const dayEvents = getEventsForDate(date, events);
            // Filter out empty remarks to avoid dot clutter if needed, or show differently
            const visibleEvents = dayEvents.filter(e => e.type !== 'remark');
            const hasRemark = dayEvents.some(e => e.type === 'remark');

            return (
                <div className="flex flex-col items-start w-full overflow-hidden">
                    {visibleEvents.map((event) => (
                        <div key={event.id} className={`calendar-event-text event-${event.type}`}>
                            {event.title}
                        </div>
                    ))}
                    {hasRemark && visibleEvents.length === 0 && (
                        <div className="absolute top-1 right-1">
                            <div className="w-2 h-2 bg-gray-400 rounded-full"></div>
                        </div>
                    )}
                </div>
            );
        }
        return null;
    };

    // Custom classes for tiles
    const tileClassName = ({ date, view }: { date: Date; view: string }) => {
        if (view === 'month') {
            if (isHoliday(date, events)) {
                return 'bg-red-50 text-red-700 font-medium'; // Holiday highlight
            }
            if (date.getDay() === 0 || date.getDay() === 6) {
                return 'text-red-500';
            }
        }
        return '';
    };

    const selectedEvents = getEventsForDate(date, events);

    return (
        <div
            className="flex flex-col bg-white/80 backdrop-blur-md rounded-2xl border border-gray-200 shadow-2xl overflow-hidden w-[900px] h-[700px] text-gray-800"
            style={draggable.style}
        >
            {/* Header */}
            <div
                className="h-14 bg-white/90 border-b border-gray-100 flex items-center justify-between px-6 select-none cursor-move"
                onMouseDown={draggable.handleMouseDown}
            >
                <div className="flex items-center gap-3">
                    <span className="text-2xl pt-1">🗓️</span>
                    <span className="text-lg font-bold bg-gradient-to-r from-blue-600 to-indigo-600 bg-clip-text text-transparent">
                        校历系统
                    </span>
                    <span className="bg-blue-50 text-blue-600 px-2 py-0.5 rounded text-xs border border-blue-100">
                        {date.getFullYear()}年
                    </span>
                </div>
                <div className="flex items-center gap-4 text-sm text-gray-500">
                    <input
                        type="file"
                        ref={fileInputRef}
                        onChange={handleFileChange}
                        accept=".xlsx, .xls"
                        className="hidden"
                    />
                    <button
                        onClick={handleImportClick}
                        className="flex items-center gap-1 hover:bg-gray-100 px-3 py-1.5 rounded-lg transition-colors text-gray-600"
                    >
                        <Upload size={16} />
                        <span>导入校历</span>
                    </button>

                    <div className="h-4 w-px bg-gray-300"></div>

                    <span className="font-medium mr-2">{format(date, 'yyyy年MM月dd日', { locale: zhCN })}</span>

                    <button
                        onClick={() => {
                            if (onClose) onClose();
                            else window.close();
                        }}
                        className="hover:bg-red-50 hover:text-red-600 p-1.5 rounded-full transition-colors"
                    >
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor">
                            <path fillRule="evenodd" d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z" clipRule="evenodd" />
                        </svg>
                    </button>
                </div>
            </div>

            {/* Split View Content */}
            <div className="flex-1 overflow-hidden flex">
                {/* Left: Calendar */}
                <div className="flex-1 p-6 overflow-auto bg-gradient-to-br from-white to-blue-50/30 overflow-y-auto custom-scrollbar">
                    <div className="bg-white rounded-xl shadow-sm border border-gray-100 p-4">
                        <Calendar
                            onChange={handleDateChange}
                            value={date}
                            locale="zh-CN"
                            tileContent={tileContent}
                            tileClassName={tileClassName}
                            view="month"
                            maxDetail="month"
                            minDetail="year"
                            prev2Label={null}
                            next2Label={null}
                        />
                    </div>
                </div>

                {/* Right: Info Panel */}
                <div className="w-80 bg-white/60 border-l border-gray-100 p-5 flex flex-col gap-6 backdrop-blur-sm">
                    {/* Events List */}
                    <div className="flex-1 overflow-y-auto">
                        <h3 className="text-sm font-bold text-gray-700 mb-3 flex items-center gap-2">
                            <span className="w-1 h-4 bg-blue-500 rounded-full"></span>
                            当日事件
                        </h3>
                        {selectedEvents.filter(e => e.type !== 'remark').length > 0 ? (
                            <div className="flex flex-col gap-2">
                                {selectedEvents.filter(e => e.type !== 'remark').map(evt => (
                                    <div key={evt.id} className="group flex items-start gap-3 p-3 bg-white rounded-xl border border-gray-100 shadow-sm hover:shadow-md transition-all">
                                        <div className={`mt-1.5 w-2 h-2 rounded-full shrink-0 ${evt.type === 'holiday' ? 'bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.5)]' :
                                                evt.type === 'exam' ? 'bg-indigo-500 shadow-[0_0_8px_rgba(99,102,241,0.5)]' :
                                                    'bg-amber-500'
                                            }`} />
                                        <div className="flex-1">
                                            <div className="text-sm font-bold text-gray-800">{evt.title}</div>
                                            {evt.description && <div className="text-xs text-gray-500 mt-0.5">{evt.description}</div>}
                                        </div>
                                    </div>
                                ))}
                            </div>
                        ) : (
                            <div className="py-8 text-center text-gray-400 text-xs border border-dashed border-gray-200 rounded-xl bg-gray-50/50">
                                暂无特定日程
                            </div>
                        )}
                    </div>

                    {/* Remarks Section */}
                    <div className="shrink-0 h-1/2 flex flex-col">
                        <h3 className="text-sm font-bold text-gray-700 mb-3 flex items-center gap-2 justify-between">
                            <div className="flex items-center gap-2">
                                <span className="w-1 h-4 bg-orange-400 rounded-full"></span>
                                备注 / 备忘
                            </div>
                            <button
                                onClick={saveRemark}
                                className="text-xs bg-gray-900 text-white px-2 py-1 rounded-md hover:bg-black transition-colors flex items-center gap-1 active:scale-95"
                            >
                                <Save size={12} />
                                保存
                            </button>
                        </h3>
                        <div className="flex-1 relative group">
                            <textarea
                                value={remarkText}
                                onChange={handleRemarkChange}
                                className="w-full h-full resize-none bg-white p-3 rounded-xl border border-gray-200 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all custom-scrollbar placeholder:text-gray-300"
                                placeholder={`在此输入 ${format(date, 'MM月dd日')} 的备注信息...`}
                            />
                            <div className="absolute bottom-3 right-3 pointer-events-none opacity-0 group-focus-within:opacity-100 transition-opacity">
                                <FileText size={48} className="text-gray-100" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default SchoolCalendar;
