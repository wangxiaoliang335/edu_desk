export type EventType = 'holiday' | 'exam' | 'activity' | 'meeting' | 'remark';

export interface SchoolEvent {
    id: string;
    date: string; // YYYY-MM-DD
    title: string;
    type: EventType;
    description?: string;
}

const STORAGE_KEY = 'school_calendar_events';

// Default initial events
const DEFAULT_EVENTS: SchoolEvent[] = [
    {
        id: '1',
        date: '2026-01-01',
        title: '元旦',
        type: 'holiday',
        description: '元旦放假'
    },
    {
        id: '2',
        date: '2026-02-14',
        title: '情人节',
        type: 'activity',
        description: '情人节活动'
    },
    {
        id: '3',
        date: '2026-06-07',
        title: '高考',
        type: 'exam',
        description: '全国统一高考'
    },
    {
        id: '4',
        date: '2026-09-10',
        title: '教师节',
        type: 'activity',
        description: '庆祝教师节'
    }
];

// Helper to format date as YYYY-MM-DD
export const formatDateKey = (date: Date): string => {
    const y = date.getFullYear();
    const m = String(date.getMonth() + 1).padStart(2, '0');
    const d = String(date.getDate()).padStart(2, '0');
    return `${y}-${m}-${d}`;
};

export const loadEvents = (): SchoolEvent[] => {
    try {
        const stored = localStorage.getItem(STORAGE_KEY);
        if (stored) {
            return JSON.parse(stored);
        }
    } catch (e) {
        console.error('Failed to load events', e);
    }
    return DEFAULT_EVENTS;
};

export const saveEvents = (events: SchoolEvent[]) => {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(events));
    } catch (e) {
        console.error('Failed to save events', e);
    }
};

export const getEventsForDate = (date: Date, allEvents: SchoolEvent[]): SchoolEvent[] => {
    const key = formatDateKey(date);
    return allEvents.filter(event => event.date === key);
};

export const isHoliday = (date: Date, allEvents: SchoolEvent[]): boolean => {
    const key = formatDateKey(date);
    return allEvents.some(event => event.type === 'holiday' && event.date === key);
};
