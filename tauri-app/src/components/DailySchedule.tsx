import { useState, useEffect, useRef } from 'react';
import { createPortal } from 'react-dom';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentTerm } from '../utils/term';
import { Clock, Bookmark, ClipboardCheck } from 'lucide-react';

interface ScheduleItem {
    time: string;       // formatted time for display
    rawTime: string;    // original time string for filtering
    subject: string;
    isHighlight?: boolean;
    isCurrent?: boolean;
}

interface Props {
    classId?: string;
    onPrepareClass?: (subject: string, time: string) => void;
    onPostEvaluation?: (subject: string) => void;
}

const getStorageKey = (classId: string) => `daily_schedule_${classId}`;

const formatTime = (time: string): string => {
    // Normalize: replace Chinese full-width colon (：) with regular colon (:)
    const normalized = time.replace(/：/g, ':');

    // Extract only time range like "7:50-8:30" from strings like "第一节\n7:50-8:30"
    const rangeMatch = normalized.match(/(\d{1,2}:\d{2})\s*-\s*(\d{1,2}:\d{2})/);
    if (rangeMatch) {
        let start = rangeMatch[1];
        let end = rangeMatch[2];
        if (/^0\d:/.test(start)) start = start.substring(1);
        if (/^0\d:/.test(end)) end = end.substring(1);
        return `${start}-${end}`;
    }

    // Try single time
    const singleMatch = normalized.match(/(\d{1,2}:\d{2})/);
    if (singleMatch) {
        let t = singleMatch[1];
        if (/^0\d:/.test(t)) t = t.substring(1);
        return t;
    }

    return '';
};

const isSpecialSubject = (subject: string): boolean => {
    const specials = ['早读', '午休', '眼保健操', '课间操', '班会', '大课间', '课服', '晚自习'];
    return specials.some(s => subject.includes(s));
};

// Get subject background color
const getSubjectStyle = (subject: string, isCurrent: boolean, isHighlight: boolean) => {
    if (isCurrent) return 'bg-gradient-to-br from-sage-500 to-sage-600 text-white shadow-lg shadow-sage-200/50 border-transparent';
    if (isHighlight) return 'bg-gradient-to-br from-clay-500 to-clay-600 text-white border-transparent';

    if (subject.includes('语文')) return 'bg-red-50 text-red-600 border-red-100';
    if (subject.includes('数学')) return 'bg-blue-50 text-blue-600 border-blue-100';
    if (subject.includes('英语')) return 'bg-amber-50 text-amber-600 border-amber-100';
    if (subject.includes('物理')) return 'bg-indigo-50 text-indigo-600 border-indigo-100';
    if (subject.includes('化学')) return 'bg-purple-50 text-purple-600 border-purple-100';
    if (subject.includes('生物')) return 'bg-green-50 text-green-600 border-green-100';
    if (subject.includes('历史')) return 'bg-yellow-50 text-yellow-700 border-yellow-100';
    if (subject.includes('地理')) return 'bg-teal-50 text-teal-600 border-teal-100';
    if (subject.includes('政治')) return 'bg-rose-50 text-rose-600 border-rose-100';
    if (subject.includes('体育')) return 'bg-emerald-50 text-emerald-600 border-emerald-100';
    return 'bg-paper text-ink-500 border-sage-100';
};

const DailySchedule = ({ classId, onPrepareClass, onPostEvaluation }: Props) => {
    const [scheduleItems, setScheduleItems] = useState<ScheduleItem[]>([]);
    const [loading, setLoading] = useState(false);
    const [activeMenuIndex, setActiveMenuIndex] = useState<number | null>(null);
    const [menuPosition, setMenuPosition] = useState<{ top: number; left: number } | null>(null);
    const menuRef = useRef<HTMLDivElement>(null);
    const scrollRef = useRef<HTMLDivElement>(null);
    const cardRefs = useRef<(HTMLDivElement | null)[]>([]);

    useEffect(() => {
        if (!classId) return;

        const fetchSchedule = async () => {
            setLoading(true);
            try {
                const userInfoStr = localStorage.getItem('user_info');
                let token = "";
                if (userInfoStr) {
                    try {
                        const u = JSON.parse(userInfoStr);
                        token = u.token || "";
                    } catch { }
                }

                const term = getCurrentTerm();
                console.log('[DailySchedule] Fetching get_course_schedule', { classId, term });
                const resp = await invoke<string>('get_course_schedule', {
                    classId,
                    term,
                    token
                });
                const data = JSON.parse(resp);

                if (data.code === 200 && data.data) {
                    if (data.data.schedule && data.data.cells) {
                        const { times } = data.data.schedule;
                        const cells = data.data.cells;

                        const today = new Date().getDay();
                        const dayMap: Record<number, number> = { 0: 6, 1: 0, 2: 1, 3: 2, 4: 3, 5: 4, 6: 5 };
                        const todayColIndex = dayMap[today];

                        const now = new Date();
                        const currentMinutes = now.getHours() * 60 + now.getMinutes();

                        const items: ScheduleItem[] = [];
                        times.forEach((time: string, rowIndex: number) => {
                            const cell = cells.find((c: any) => c.row_index === rowIndex && c.col_index === todayColIndex);
                            let subject = cell ? (cell.course_name || cell.subject || "") : "";

                            // Extract special subject names from time field (e.g. "早读\n7:00-7:40" -> subject="早读")
                            const specialMatch = time.match(/(早读|午休|眼保健操|课间操|班会|大课间|课服\d*|晚自习\d*)/);
                            if (!subject && specialMatch) {
                                subject = specialMatch[1];
                            }

                            // Normalize Chinese colon for time matching
                            const normalizedTime = time.replace(/：/g, ':');
                            const timeMatch = normalizedTime.match(/(\d{1,2}):(\d{2})/);
                            let isCurrent = false;
                            if (timeMatch) {
                                const itemMinutes = parseInt(timeMatch[1]) * 60 + parseInt(timeMatch[2]);
                                isCurrent = currentMinutes >= itemMinutes && currentMinutes < itemMinutes + 45;
                            }

                            items.push({
                                time: formatTime(time),
                                rawTime: time,
                                subject: subject,
                                isHighlight: isSpecialSubject(subject) || isSpecialSubject(time),
                                isCurrent
                            });
                        });

                        const filtered = items.filter(i => i.rawTime);
                        setScheduleItems(filtered);
                        localStorage.setItem(getStorageKey(classId), JSON.stringify(filtered));

                        // Scroll to current class
                        setTimeout(() => {
                            const currentIndex = filtered.findIndex(i => i.isCurrent);
                            if (currentIndex > 0 && scrollRef.current) {
                                const scrollAmount = Math.max(0, (currentIndex - 1) * 90);
                                scrollRef.current.scrollLeft = scrollAmount;
                            }
                        }, 100);
                    }
                }
            } catch (e) {
                console.error('[DailySchedule] Failed to fetch:', e);
                try {
                    const cached = localStorage.getItem(getStorageKey(classId));
                    if (cached) {
                        setScheduleItems(JSON.parse(cached));
                    }
                } catch { }
            } finally {
                setLoading(false);
            }
        };

        fetchSchedule();
    }, [classId]);

    useEffect(() => {
        const handleRefresh = (e: Event) => {
            const detail = (e as CustomEvent).detail;
            if (detail?.classId && detail.classId !== classId) return;
            // Re-run fetch for latest schedule after import
            if (classId) {
                const term = getCurrentTerm();
                console.log('[DailySchedule] Refresh requested', { classId, term });
                invoke<string>('get_course_schedule', {
                    classId,
                    term,
                    token: localStorage.getItem('token') || ''
                }).then(res => {
                    try {
                        const data = JSON.parse(res);
                        if (data.code === 200 && data.data) {
                            if (data.data.schedule && data.data.cells) {
                                const { times } = data.data.schedule;
                                const cells = data.data.cells;

                                const today = new Date().getDay();
                                const dayMap: Record<number, number> = { 0: 6, 1: 0, 2: 1, 3: 2, 4: 3, 5: 4, 6: 5 };
                                const todayColIndex = dayMap[today];

                                const now = new Date();
                                const currentMinutes = now.getHours() * 60 + now.getMinutes();

                                const items: ScheduleItem[] = [];
                                times.forEach((time: string, rowIndex: number) => {
                                    const cell = cells.find((c: any) => c.row_index === rowIndex && c.col_index === todayColIndex);
                                    let subject = cell ? (cell.course_name || cell.subject || "") : "";

                                    const specialMatch = time.match(/(早读|午休|眼保健操|课间操|班会|大课间|课服\d*|晚自习\d*)/);
                                    if (!subject && specialMatch) {
                                        subject = specialMatch[1];
                                    }

                                    const normalizedTime = time.replace(/：/g, ':');
                                    const timeMatch = normalizedTime.match(/(\d{1,2}):(\d{2})/);
                                    let isCurrent = false;
                                    if (timeMatch) {
                                        const itemMinutes = parseInt(timeMatch[1]) * 60 + parseInt(timeMatch[2]);
                                        isCurrent = currentMinutes >= itemMinutes && currentMinutes < itemMinutes + 45;
                                    }

                                    items.push({
                                        time: formatTime(time),
                                        rawTime: time,
                                        subject: subject,
                                        isHighlight: isSpecialSubject(subject) || isSpecialSubject(time),
                                        isCurrent
                                    });
                                });

                                const filtered = items.filter(i => i.rawTime);
                                setScheduleItems(filtered);
                                localStorage.setItem(getStorageKey(classId), JSON.stringify(filtered));
                            }
                        }
                    } catch { }
                }).catch(() => { });
            }
        };
        window.addEventListener('refresh-daily-schedule', handleRefresh as EventListener);
        return () => window.removeEventListener('refresh-daily-schedule', handleRefresh as EventListener);
    }, [classId]);

    useEffect(() => {
        const handleClickOutside = (e: MouseEvent) => {
            if (menuRef.current && !menuRef.current.contains(e.target as Node)) {
                setActiveMenuIndex(null);
            }
        };
        document.addEventListener('mousedown', handleClickOutside);
        return () => document.removeEventListener('mousedown', handleClickOutside);
    }, []);

    // Handle mouse wheel for horizontal scrolling
    const handleWheel = (e: React.WheelEvent<HTMLDivElement>) => {
        if (scrollRef.current) {
            e.preventDefault();
            scrollRef.current.scrollLeft += e.deltaY;
        }
    };

    const handleCardClick = (index: number, item: ScheduleItem) => {
        if (item.subject && !item.isHighlight) {
            if (activeMenuIndex === index) {
                setActiveMenuIndex(null);
                setMenuPosition(null);
            } else {
                const cardEl = cardRefs.current[index];
                if (cardEl) {
                    const rect = cardEl.getBoundingClientRect();
                    setMenuPosition({
                        top: rect.bottom + 8,
                        left: rect.left + rect.width / 2
                    });
                }
                setActiveMenuIndex(index);
            }
        }
    };

    const handleMenuAction = (action: 'prepare' | 'evaluate', item: ScheduleItem) => {
        setActiveMenuIndex(null);
        if (action === 'prepare' && onPrepareClass) {
            onPrepareClass(item.subject, item.time);
        } else if (action === 'evaluate' && onPostEvaluation) {
            onPostEvaluation(item.subject);
        }
    };

    if (loading) {
        return (
            <div className="h-full flex items-center justify-center">
                <div className="w-6 h-6 border-2 border-sage-200 border-t-sage-500 rounded-full animate-spin"></div>
            </div>
        );
    }

    if (scheduleItems.length === 0) {
        return (
            <div className="h-full flex flex-col items-center justify-center text-center py-6">
                <Clock size={28} className="text-sage-200 mb-2" />
                <p className="text-ink-300 text-sm">今日无课程安排</p>
            </div>
        );
    }

    return (
        <div className="h-full flex flex-col">
            {/* Horizontal Timeline */}
            <div
                ref={scrollRef}
                onWheel={handleWheel}
                className="flex-1 flex items-center gap-2 overflow-x-auto pb-2 scroll-smooth timeline-scroll"
            >
                <style>{`
                    .timeline-scroll::-webkit-scrollbar { height: 4px; }
                    .timeline-scroll::-webkit-scrollbar-track { background: transparent; }
                    .timeline-scroll::-webkit-scrollbar-thumb { background: #e2e8f0; border-radius: 4px; }
                    .timeline-scroll::-webkit-scrollbar-thumb:hover { background: #cbd5e1; }
                `}</style>


                {scheduleItems.map((item, index) => {
                    const isSpecial = item.isHighlight;
                    const cardStyle = getSubjectStyle(item.subject, item.isCurrent || false, isSpecial || false);
                    const hasSubject = item.subject && !isSpecial;

                    return (
                        <div
                            key={index}
                            ref={el => { cardRefs.current[index] = el; }}
                            className="relative flex-shrink-0"
                        >
                            {/* Card */}
                            <div
                                onClick={() => handleCardClick(index, item)}
                                className={`
                                    w-24 h-24 rounded-xl flex flex-col items-center justify-center gap-1 transition-all
                                    border ${cardStyle}
                                    ${hasSubject ? 'cursor-pointer hover:scale-105' : ''}
                                    ${item.isCurrent ? 'scale-105' : ''}
                                `}
                            >
                                {/* Subject - big and prominent */}
                                <span className="text-lg font-bold text-center px-1 leading-tight">
                                    {item.subject || '-'}
                                </span>

                                {/* Time - smaller below */}
                                <span className={`text-xs font-medium ${item.isCurrent || isSpecial ? 'opacity-70' : 'text-gray-400'
                                    }`}>
                                    {item.time}
                                </span>

                                {/* Current Badge */}
                                {item.isCurrent && (
                                    <span className="text-[10px] bg-white/20 px-2 py-0.5 rounded-full font-medium">
                                        进行中
                                    </span>
                                )}
                            </div>

                            {/* Connection Line (except last) */}
                            {index < scheduleItems.length - 1 && (
                                <div className="absolute top-1/2 -right-1 w-2 h-0.5 bg-gray-200 -translate-y-1/2"></div>
                            )}
                        </div>
                    );
                })}

                {/* Dropdown Menu - Rendered in Portal to avoid clipping */}
                {activeMenuIndex !== null && menuPosition && createPortal(
                    <div
                        ref={menuRef}
                        className="fixed bg-white rounded-xl shadow-2xl border border-gray-100 py-1.5 z-[9999] min-w-[120px] animate-in fade-in zoom-in-95 duration-100"
                        style={{
                            top: menuPosition.top,
                            left: menuPosition.left,
                            transform: 'translateX(-50%)'
                        }}
                    >
                        {scheduleItems[activeMenuIndex] && (
                            <>
                                <button
                                    onClick={() => handleMenuAction('prepare', scheduleItems[activeMenuIndex])}
                                    className="w-full px-4 py-2.5 text-sm text-gray-700 hover:bg-blue-50 hover:text-blue-600 flex items-center gap-2 transition-colors"
                                >
                                    <Bookmark size={14} />
                                    课前准备
                                </button>
                                <button
                                    onClick={() => handleMenuAction('evaluate', scheduleItems[activeMenuIndex])}
                                    className="w-full px-4 py-2.5 text-sm text-gray-700 hover:bg-green-50 hover:text-green-600 flex items-center gap-2 transition-colors"
                                >
                                    <ClipboardCheck size={14} />
                                    课后评价
                                </button>
                            </>
                        )}
                    </div>,
                    document.body
                )}
            </div>
        </div>
    );
};

export default DailySchedule;
