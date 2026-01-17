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
            className="flex flex-col bg-white rounded-3xl border border-sage-200 shadow-[0_8px_30px_rgb(0,0,0,0.06)] overflow-hidden w-[900px] h-[700px] text-ink-600 font-sans"
            style={draggable.style}
        >
            {/* Header */}
            <div
                className="h-16 bg-white border-b border-sage-50 flex items-center justify-between px-8 select-none cursor-move"
                onMouseDown={draggable.handleMouseDown}
            >
                <div className="flex items-center gap-3">
                    <span className="text-2xl pt-1">🗓️</span>
                    <span className="text-xl font-bold text-ink-800 tracking-tight">
                        校历日程
                    </span>
                    <span className="bg-sage-50 text-sage-600 px-3 py-1 rounded-lg text-sm font-semibold border border-sage-100">
                        {date.getFullYear()}
                    </span>
                </div>
                <div className="flex items-center gap-4 text-sm text-ink-400 font-medium">
                    <input
                        type="file"
                        ref={fileInputRef}
                        onChange={handleFileChange}
                        accept=".xlsx, .xls"
                        className="hidden"
                    />
                    <button
                        onClick={handleImportClick}
                        className="flex items-center gap-2 hover:bg-sage-50 px-4 py-2 rounded-xl transition-colors text-ink-600 font-bold"
                    >
                        <Upload size={18} />
                        <span>导入数据</span>
                    </button>

                    <div className="h-6 w-px bg-sage-100"></div>

                    <span className="text-ink-600">{format(date, 'yyyy年MM月dd日', { locale: zhCN })}</span>

                    <button
                        onClick={() => {
                            if (onClose) onClose();
                            else window.close();
                        }}
                        className="hover:bg-clay-50 hover:text-clay-600 p-2 rounded-full transition-colors"
                    >
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" viewBox="0 0 20 20" fill="currentColor">
                            <path fillRule="evenodd" d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z" clipRule="evenodd" />
                        </svg>
                    </button>
                </div>
            </div>

            {/* Split View Content */}
            <div className="flex-1 overflow-hidden flex bg-paper">
                {/* Left: Calendar */}
                <div className="flex-1 p-8 overflow-auto custom-scrollbar">
                    <div className="bg-white rounded-3xl shadow-sm border border-sage-50 p-6">
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
                <div className="w-80 bg-white border-l border-sage-50 p-6 flex flex-col gap-6">
                    {/* Events List */}
                    <div className="flex-1 overflow-y-auto pr-2">
                        <h3 className="text-sm font-bold text-ink-400 mb-4 flex items-center gap-2 uppercase tracking-wider">
                            <span className="w-1.5 h-1.5 bg-sage-400 rounded-full"></span>
                            今日日程
                        </h3>
                        {selectedEvents.filter(e => e.type !== 'remark').length > 0 ? (
                            <div className="flex flex-col gap-3">
                                {selectedEvents.filter(e => e.type !== 'remark').map(evt => (
                                    <div key={evt.id} className="group flex items-start gap-4 p-4 bg-paper rounded-2xl border border-transparent hover:border-sage-200 transition-all">
                                        <div className={`mt-2 w-2 h-2 rounded-full shrink-0 ${evt.type === 'holiday' ? 'bg-clay-500' :
                                            evt.type === 'exam' ? 'bg-sage-600' :
                                                'bg-amber-400'
                                            }`} />
                                        <div className="flex-1">
                                            <div className="text-base font-bold text-ink-800">{evt.title}</div>
                                            {evt.description && <div className="text-xs text-ink-400 mt-1">{evt.description}</div>}
                                        </div>
                                    </div>
                                ))}
                            </div>
                        ) : (
                            <div className="py-12 text-center text-ink-300 text-sm border-2 border-dashed border-sage-50 rounded-2xl">
                                今日暂无日程安排
                            </div>
                        )}
                    </div>

                    {/* Remarks Section */}
                    <div className="shrink-0 h-1/2 flex flex-col pt-4 border-t border-sage-50">
                        <h3 className="text-sm font-bold text-ink-400 mb-4 flex items-center gap-2 justify-between uppercase tracking-wider">
                            <div className="flex items-center gap-2">
                                <span className="w-1.5 h-1.5 bg-clay-400 rounded-full"></span>
                                备注提醒
                            </div>
                            <button
                                onClick={saveRemark}
                                className="text-xs bg-ink-800 text-white px-3 py-1.5 rounded-lg hover:bg-ink-600 transition-colors flex items-center gap-1.5 active:scale-95 font-bold"
                            >
                                <Save size={14} />
                                保存
                            </button>
                        </h3>
                        <div className="flex-1 relative group">
                            <textarea
                                value={remarkText}
                                onChange={handleRemarkChange}
                                className="w-full h-full resize-none bg-paper p-4 rounded-2xl border-2 border-transparent focus:border-sage-200 text-ink-600 text-sm focus:outline-none transition-all custom-scrollbar placeholder:text-ink-300 font-medium"
                                placeholder={`输入 ${format(date, 'MM月dd日')} 的备忘信息...`}
                            />
                            <div className="absolute bottom-4 right-4 pointer-events-none opacity-0 group-focus-within:opacity-100 transition-opacity">
                                <FileText size={20} className="text-sage-300" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default SchoolCalendar;
