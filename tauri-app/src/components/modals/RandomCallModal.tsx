import { useState, useEffect, useRef } from 'react';
import { X, Shuffle, RotateCcw } from 'lucide-react';

interface RandomCallModalProps {
    isOpen: boolean;
    onClose: () => void;
    students?: string[]; // List of student names
}

const MOCK_STUDENTS = [
    "张伟", "王伟", "王芳", "李伟", "王秀英", "李秀英", "李娜", "张秀英", "刘伟", "张敏",
    "李静", "张丽", "王静", "王丽", "李强", "张静", "李敏", "王敏", "王磊", "李军",
    "刘洋", "王勇", "张勇", "王艳", "李杰", "张磊", "王强", "王军", "张杰", "李娟"
];

const RandomCallModal = ({ isOpen, onClose, students = MOCK_STUDENTS }: RandomCallModalProps) => {
    const [currentName, setCurrentName] = useState("---");
    const [isRunning, setIsRunning] = useState(false);
    const [history, setHistory] = useState<string[]>([]);
    const intervalRef = useRef<any>(null);

    useEffect(() => {
        if (!isOpen) {
            setIsRunning(false);
            if (intervalRef.current) clearInterval(intervalRef.current);
            setCurrentName("---");
        }
    }, [isOpen]);

    const toggleRollCall = () => {
        if (isRunning) {
            // Stop
            if (intervalRef.current) clearInterval(intervalRef.current);
            setIsRunning(false);
            // Add to history
            if (currentName !== "---") {
                setHistory(prev => [currentName, ...prev].slice(0, 5));
            }
        } else {
            // Start
            setIsRunning(true);
            intervalRef.current = setInterval(() => {
                const randomName = students[Math.floor(Math.random() * students.length)];
                setCurrentName(randomName);
            }, 80); // Fast cycle
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-white rounded-2xl shadow-2xl w-[400px] overflow-hidden border border-gray-100 flex flex-col">
                {/* Header */}
                <div className="bg-gradient-to-r from-blue-500 to-indigo-600 p-4 flex items-center justify-between text-white">
                    <div className="flex items-center gap-2 font-bold text-lg">
                        <Shuffle size={20} />
                        <span>随机点名</span>
                    </div>
                    <button onClick={onClose} className="p-1 hover:bg-white/20 rounded-full transition-colors">
                        <X size={20} />
                    </button>
                </div>

                {/* Main Content */}
                <div className="p-8 flex flex-col items-center justify-center bg-gray-50/50">
                    <div className="w-48 h-48 rounded-full bg-white border-4 border-blue-100 shadow-inner flex items-center justify-center mb-8 relative">
                        {/* Decorative rings */}
                        <div className={`absolute inset-0 rounded-full border-2 border-blue-500/20 ${isRunning ? 'animate-ping' : ''}`}></div>
                        <div className={`absolute inset-2 rounded-full border border-blue-500/10 ${isRunning ? 'animate-pulse' : ''}`}></div>

                        <span className={`text-5xl font-bold bg-gradient-to-br from-gray-800 to-gray-600 bg-clip-text text-transparent transition-all duration-100 ${isRunning ? 'scale-110 blur-[0.5px]' : 'scale-100'}`}>
                            {currentName}
                        </span>
                    </div>

                    <button
                        onClick={toggleRollCall}
                        className={`w-full py-3 rounded-xl font-bold text-white shadow-md transition-all active:scale-95 flex items-center justify-center gap-2 ${isRunning
                            ? 'bg-red-500 hover:bg-red-600 shadow-red-200'
                            : 'bg-blue-600 hover:bg-blue-700 shadow-blue-200'}`}
                    >
                        {isRunning ? (
                            <>停止点名</>
                        ) : (
                            <>开始点名</>
                        )}
                    </button>
                </div>

                {/* History */}
                {history.length > 0 && (
                    <div className="bg-white border-t border-gray-100 p-4">
                        <div className="flex items-center gap-2 text-xs text-gray-500 mb-2 font-medium">
                            <RotateCcw size={12} />
                            最近点名
                        </div>
                        <div className="flex gap-2 flex-wrap">
                            {history.map((name, i) => (
                                <span key={i} className="px-2 py-1 bg-gray-100 text-gray-600 text-xs rounded-md">
                                    {name}
                                </span>
                            ))}
                        </div>
                    </div>
                )}
            </div>
        </div>
    );
};

export default RandomCallModal;
