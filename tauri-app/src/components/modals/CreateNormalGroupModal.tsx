import { useState, useEffect } from 'react';
import { X, Users } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';

interface CreateNormalGroupModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo?: any;
    onSuccess?: () => void;
}

const CreateNormalGroupModal = ({ isOpen, onClose, userInfo, onSuccess }: CreateNormalGroupModalProps) => {
    const [groupName, setGroupName] = useState('');
    const [isLoading, setIsLoading] = useState(false);

    // Reset state when modal opens
    useEffect(() => {
        if (isOpen) {
            setGroupName('');
            setIsLoading(false);
        }
    }, [isOpen]);

    if (!isOpen) return null;

    const handleCreate = async () => {
        if (!groupName.trim()) {
            return;
        }

        setIsLoading(true);
        try {
            // Retrieve current user ID
            const userId = userInfo?.teacher_unique_id || userInfo?.id || localStorage.getItem('unique_id') || localStorage.getItem('userid');
            if (!userId) {
                alert("无法获取用户信息，请重新登录");
                setIsLoading(false);
                return;
            }

            // Call backend command to create group via TIM REST API
            await invoke('create_group_tim', {
                ownerId: userId,
                groupName: groupName.trim(),
                groupType: 'Public'
            });

            onSuccess?.();
            onClose();
        } catch (error) {
            console.error("Failed to create group:", error);
            alert(`创建失败: ${error}`);
        } finally {
            setIsLoading(false);
        }
    };

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-white rounded-xl shadow-2xl w-[400px] flex flex-col overflow-hidden animate-in zoom-in-95 duration-200">

                {/* Header */}
                <div className="bg-gradient-to-r from-blue-500 to-blue-600 p-4 flex items-center justify-between text-white shadow-md">
                    <div className="flex items-center gap-2">
                        <Users className="w-5 h-5" />
                        <span className="font-bold text-lg">创建普通群</span>
                    </div>
                    <button
                        onClick={onClose}
                        className="text-white/80 hover:text-white hover:bg-white/20 rounded-full p-1 transition-colors"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-6 flex flex-col gap-6">
                    <div className="flex flex-col gap-2">
                        <label className="text-gray-700 font-medium text-sm">群组名称</label>
                        <input
                            type="text"
                            value={groupName}
                            onChange={(e) => setGroupName(e.target.value)}
                            placeholder="请输入群组名称..."
                            className="w-full p-3 rounded-lg border border-gray-300 bg-gray-50 text-gray-900 outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent transition-all placeholder:text-gray-400"
                            maxLength={30}
                            autoFocus
                        />
                        <p className="text-xs text-gray-500 text-right">{groupName.length}/30</p>
                    </div>
                </div>

                {/* Footer */}
                <div className="p-4 bg-gray-50 border-t border-gray-100 flex justify-end gap-3">
                    <button
                        onClick={onClose}
                        className="px-5 py-2.5 text-gray-600 bg-white border border-gray-300 rounded-lg hover:bg-gray-50 font-medium transition-colors shadow-sm"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleCreate}
                        disabled={isLoading || !groupName.trim()}
                        className="px-5 py-2.5 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-lg hover:from-blue-600 hover:to-blue-700 font-medium shadow-md disabled:opacity-50 disabled:cursor-not-allowed flex items-center gap-2 transition-all"
                    >
                        {isLoading ? (
                            <>
                                <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin" />
                                <span>创建中...</span>
                            </>
                        ) : (
                            <span>确认创建</span>
                        )}
                    </button>
                </div>

            </div>
        </div>
    );
};

export default CreateNormalGroupModal;
